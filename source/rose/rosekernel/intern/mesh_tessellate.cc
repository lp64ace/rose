#include "MEM_guardedalloc.h"

#include "LIB_enumerable_thread_specific.hh"
#include "LIB_math_geom.h"
#include "LIB_math_matrix.h"
#include "LIB_math_matrix.hh"
#include "LIB_math_vector.h"
#include "LIB_math_vector.hh"
#include "LIB_memarena.h"
#include "LIB_offset_span.hh"
#include "LIB_offset_indices.hh"
#include "LIB_polyfill_2d.h"
#include "LIB_span.hh"
#include "LIB_task.h"
#include "LIB_task.hh"

#include "KER_mesh_types.hh"
#include "KER_mesh.h"

#define MESH_FACE_TESSELLATE_THREADED_LIMIT 4096

namespace rose::kernel::mesh {

ROSE_INLINE void mesh_calc_tessellation_for_face_impl(const Span<int> corner_verts, const OffsetIndices<int> polys, const Span<float3> positions, int poly_index, MLoopTri *mlt, MemArena **pf_arena_p, const bool face_normal, const float normal_precalc[3]) {
	const size_t loop_start = polys[poly_index].start();
	const size_t loop_length = polys[poly_index].size();

	auto create_tri = [&](size_t i1, size_t i2, size_t i3) {
		mlt->tri[0] = loop_start + i1;
		mlt->tri[1] = loop_start + i2;
		mlt->tri[2] = loop_start + i3;
	};

	switch (loop_length) {
		case 3: {
			create_tri(0, 1, 2);
		} break;
		case 4: {
			create_tri(0, 1, 2);
			MLoopTri *mlt_a = mlt++;
			create_tri(0, 2, 3);
			MLoopTri *mlt_b = mlt++;
			if (is_quad_flip_v3_first_third_fast(positions[corner_verts[mlt_a->tri[0]]], positions[corner_verts[mlt_a->tri[1]]], positions[corner_verts[mlt_a->tri[2]]], positions[corner_verts[mlt_b->tri[2]]])) {
				mlt_a->tri[2] = mlt_b->tri[2];
				mlt_b->tri[0] = mlt_a->tri[1];
			}
		} break;
		default: {
			float axis_mat[3][3];

			if (face_normal == false) {
				float normal[3];
				const float *co_curr, *co_prev;

				zero_v3(normal);

				co_prev = positions[corner_verts[loop_start + loop_length - 1]];
				for (size_t i = 0; i < loop_length; i++) {
					co_curr = positions[corner_verts[loop_start + i]];
					add_newell_cross_v3_v3v3(normal, co_prev, co_curr);
					co_prev = co_curr;
				}
				if (normalize_v3(normal) == 0.0f) {
					normal[2] = 1.0f;
				}
				axis_dominant_v3_to_m3_negate(axis_mat, normal);
			}
			else {
				axis_dominant_v3_to_m3_negate(axis_mat, normal_precalc);
			}

			const size_t tri_length = loop_length - 2;

			MemArena *pf_arena = *pf_arena_p;
			if (pf_arena == NULL) {
				pf_arena = *pf_arena_p = LIB_memory_arena_create(16 * 1024, __func__);
			}

			unsigned int(*tris)[3] = static_cast<unsigned int(*)[3]>(LIB_memory_arena_malloc(pf_arena, sizeof(*tris) * tri_length));
			float(*vrts)[2] = static_cast<float(*)[2]>(LIB_memory_arena_malloc(pf_arena, sizeof(*vrts) * loop_length));

			for (size_t i = 0; i < loop_length; i++) {
				/** Store the projected 3D verts onto the 2D plane defined by the normal into #vrts. */
				mul_v2_m3v3(vrts[i], axis_mat, positions[corner_verts[loop_start + i]]);
			}

			LIB_polyfill_calc_arena(vrts, loop_length, 1, tris, pf_arena);

			for (size_t i = 0; i < tri_length; i++, mlt++) {
				const unsigned int *tri = tris[i];
				create_tri(tri[0], tri[1], tri[2]);
			}

			LIB_memory_arena_clear(pf_arena);
		} break;
	}
}

ROSE_STATIC void mesh_calc_tessellation_for_face(const Span<int> corner_verts, const OffsetIndices<int> polys, const Span<float3> positions, int poly_index, MLoopTri *mlt, MemArena **pf_arena_p) {
	mesh_calc_tessellation_for_face_impl(corner_verts, polys, positions, poly_index, mlt, pf_arena_p, false, NULL);
}

ROSE_STATIC void mesh_calc_tessellation_for_face_with_normal(const Span<int> corner_verts, const OffsetIndices<int> polys, const Span<float3> positions, int poly_index, MLoopTri *mlt, MemArena **pf_arena_p, const float normal_precalc[3]) {
	mesh_calc_tessellation_for_face_impl(corner_verts, polys, positions, poly_index, mlt, pf_arena_p, true, normal_precalc);
}

ROSE_STATIC void mesh_recalc_looptri__single_threaded(const Span<int> corner_verts, const OffsetIndices<int> polys, const Span<float3> positions, MLoopTri *mlt, const float (*poly_normals)[3]) {
	MemArena *pf_arena = NULL;

	int tri_index = 0;

	if (poly_normals != NULL) {
		for (const size_t i : polys.index_range()) {
			mesh_calc_tessellation_for_face_with_normal(corner_verts, polys, positions, (int)i, &mlt[tri_index], &pf_arena, poly_normals[i]);
			tri_index += (int)polys[i].size() - 2;
		}
	}
	else {
		for (const size_t i : polys.index_range()) {
			mesh_calc_tessellation_for_face(corner_verts, polys, positions, (int)i, &mlt[tri_index], &pf_arena);
			tri_index += (int)polys[i].size() - 2;
		}
	}

	if (pf_arena) {
		LIB_memory_arena_destroy(pf_arena);
	}
	ROSE_assert(tri_index == poly_to_tri_count((int)polys.size(), (int)corner_verts.size()));
}

struct TessellationUserData {
	Span<int> corner_verts;
	OffsetIndices<int> polys;
	Span<float3> positions;
	Span<float3> poly_normals;

	MutableSpan<MLoopTri> mlooptri;
};

struct TesselationUserTLS {
	MemArena *pf_arena;
};

ROSE_STATIC void mesh_calc_tessellation_for_face_fn(void *userdata, const int index, const TaskParallelTLS *tls_v) {
	const TessellationUserData *data = static_cast<const TessellationUserData *>(userdata);

	TesselationUserTLS *tls = static_cast<TesselationUserTLS *>(tls_v->userdata_chunk);
	int i = (int)poly_to_tri_count(index, data->polys[index].start());
	mesh_calc_tessellation_for_face_impl(data->corner_verts, data->polys, data->positions, i, &data->mlooptri[index], &tls->pf_arena, false, NULL);
}

ROSE_STATIC void mesh_calc_tessellation_for_face_with_normal_fn(void *userdata, const int index, const TaskParallelTLS *tls_v) {
	const TessellationUserData *data = static_cast<const TessellationUserData *>(userdata);

	TesselationUserTLS *tls = static_cast<TesselationUserTLS *>(tls_v->userdata_chunk);
	int i = (int)poly_to_tri_count(index, data->polys[index].start());
	mesh_calc_tessellation_for_face_impl(data->corner_verts, data->polys, data->positions, i, &data->mlooptri[index], &tls->pf_arena, true, data->poly_normals[index]);
}

ROSE_STATIC void mesh_calc_tessellation_for_face_free_fn(const void *userdata, void *tls_v) {
	TesselationUserTLS *tls_data = static_cast<TesselationUserTLS *>(tls_v);

	if (tls_data->pf_arena) {
		LIB_memory_arena_destroy(tls_data->pf_arena);
	}
}

ROSE_STATIC void looptris_calc_all(const Span<float3> positions, const OffsetIndices<int> polys, const Span<int> corner_verts, const Span<float3> poly_normals, MutableSpan<MLoopTri> looptris) {
	if (corner_verts.size() < MESH_FACE_TESSELLATE_THREADED_LIMIT) {
		mesh_recalc_looptri__single_threaded(corner_verts, polys, positions, looptris.data(), reinterpret_cast<const float(*)[3]>(poly_normals.data()));
	}
	else {
		struct TesselationUserTLS tls_data_dummy = {nullptr};
		struct TessellationUserData data {};
		data.corner_verts = corner_verts;
		data.polys = polys;
		data.positions = positions;
		data.mlooptri = looptris;
		data.poly_normals = poly_normals;

		TaskParallelSettings settings;
		LIB_parallel_range_settings_defaults(&settings);

		settings.userdata_chunk = &tls_data_dummy;
		settings.userdata_chunk_size = sizeof(tls_data_dummy);
		settings.func_free = mesh_calc_tessellation_for_face_free_fn;

		LIB_task_parallel_range(0, polys.size(), &data, poly_normals.is_empty() ? mesh_calc_tessellation_for_face_fn : mesh_calc_tessellation_for_face_with_normal_fn, &settings);
	}
}

ROSE_STATIC void looptris_calc(const Span<float3> vert_positions, const OffsetIndices<int> polys, const Span<int> corner_verts, MutableSpan<MLoopTri> looptris) {
	looptris_calc_all(vert_positions, polys, corner_verts, {}, looptris);
}

ROSE_STATIC void looptris_calc_poly_indices(const OffsetIndices<int> polys, MutableSpan<int> looptri_polys) {
	threading::parallel_for(polys.index_range(), 1024, [&](const IndexRange range) {
		for (const size_t i : range) {
			const IndexRange poly = polys[i];
			const int start = poly_to_tri_count((int)i, (int)poly.start());
			const int length = ME_POLY_TRI_TOT((int)poly.size());
			looptri_polys.slice(start, length).fill(i);
		}
	});
}

ROSE_STATIC void looptris_calc_with_normals(const Span<float3> vert_positions, const OffsetIndices<int> polys, const Span<int> corner_verts, const Span<float3> poly_normals, MutableSpan<MLoopTri> looptris) {
	ROSE_assert(!poly_normals.is_empty() || polys.size() == 0);
	looptris_calc_all(vert_positions, polys, corner_verts, poly_normals, looptris);
}

}  // namespace rose::kernel::mesh

const MLoopTri *KER_mesh_looptris(const struct Mesh *mesh) {
	mesh->runtime->looptris_cache.ensure([&](rose::Array<MLoopTri> &r_data) {
		const rose::Span<float3> positions = rose::Span<float3>(
			reinterpret_cast<const float3 *>(KER_mesh_vert_positions(mesh)),
			mesh->totvert
		);
		const rose::OffsetIndices<int> polys = rose::Span<int>(
			KER_mesh_poly_offsets(mesh),
			mesh->totpoly + 1
		);
		const rose::Span<int> corner_verts = rose::Span<int>(
			KER_mesh_corner_verts(mesh),
			mesh->totloop
		);

		r_data.reinitialize(poly_to_tri_count((int)polys.size(), (int)corner_verts.size()));

		if (KER_mesh_poly_normals_are_dirty(mesh)) {
			rose::kernel::mesh::looptris_calc(positions, polys, corner_verts, r_data);
		}
		else {
			const rose::Span<float3> poly_normals = rose::Span<float3>(
				reinterpret_cast<const float3 *>(KER_mesh_poly_normals_ensure(mesh)),
				mesh->totpoly
			);
			rose::kernel::mesh::looptris_calc_with_normals(positions, polys, corner_verts, poly_normals, r_data);
		}
	});
	/** Using .data().data() does not seem very nice but since we need to cast it to C pointer we just use the compiler optimizations! */
	const rose::Array<MLoopTri>& looptris = mesh->runtime->looptris_cache.data();
	return looptris.data();
}

const int *KER_mesh_looptri_polys(const Mesh *mesh) {
	mesh->runtime->looptri_polys_cache.ensure([&](rose::Array<int> &r_data) {
		const rose::OffsetIndices<int> polys = rose::Span<int>(
			KER_mesh_poly_offsets(mesh),
			mesh->totpoly + 1
		);
		r_data.reinitialize(poly_to_tri_count(polys.size(), mesh->totloop));
		rose::kernel::mesh::looptris_calc_poly_indices(polys, r_data);
	});
	/** Using .data().data() does not seem very nice but since we need to cast it to C pointer we just use the compiler optimizations! */
	const rose::Array<int>& looptri_polys = mesh->runtime->looptri_polys_cache.data();
	return looptri_polys.data();
}
