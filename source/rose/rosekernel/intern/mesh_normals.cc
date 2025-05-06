#include "MEM_guardedalloc.h"

#include "LIB_math_geom.h"
#include "LIB_math_matrix.h"
#include "LIB_math_matrix.hh"
#include "LIB_math_vector.h"
#include "LIB_math_vector.hh"
#include "LIB_task.hh"
#include "LIB_offset_span.hh"
#include "LIB_offset_indices.hh"

#include "KER_mesh_types.hh"
#include "KER_mesh.h"

#include "atomic_ops.h"

/* -------------------------------------------------------------------- */
/** \name Private Utility Functions
 * \{ */

/**
 * A thread-safe version of #add_v3_v3 that uses a spin-lock.
 *
 * \note Avoid using this when the chance of contention is high.
 */
static void add_v3_v3_atomic(float r[3], const float a[3]) {
#define FLT_EQ_NONAN(_fa, _fb) (*((const uint32_t *)&_fa) == *((const uint32_t *)&_fb))

	float virtual_lock = r[0];
	while (true) {
		/* This loops until following conditions are met:
		 * - `r[0]` has same value as virtual_lock (i.e. it did not change since last try).
		 * - `r[0]` was not `FLT_MAX`, i.e. it was not locked by another thread. */
		const float test_lock = atomic_cas_float(&r[0], virtual_lock, FLT_MAX);
		if (_ATOMIC_LIKELY(FLT_EQ_NONAN(test_lock, virtual_lock) && (test_lock != FLT_MAX))) {
			break;
		}
		virtual_lock = test_lock;
	}
	virtual_lock += a[0];
	r[1] += a[1];
	r[2] += a[2];

	/* Second atomic operation to 'release'
	 * our lock on that vector and set its first scalar value. */
	/* Note that we do not need to loop here, since we 'locked' `r[0]`,
	 * nobody should have changed it in the mean time. */
	virtual_lock = atomic_cas_float(&r[0], FLT_MAX, virtual_lock);
	ROSE_assert(virtual_lock == FLT_MAX);

#undef FLT_EQ_NONAN
}

/* \} */

namespace rose::kernel::mesh {

ROSE_STATIC float3 normal_calc_ngon(const Span<float3> vert_positions, const Span<int> poly_verts) {
	float3 normal(0);

	/* Newell's Method */
	const float *v_prev = vert_positions[poly_verts.last()];
	for (const int i : poly_verts.index_range()) {
		const float *v_curr = vert_positions[poly_verts[i]];
		add_newell_cross_v3_v3v3(normal, v_prev, v_curr);
		v_prev = v_curr;
	}

	if (normalize_v3(normal) == 0.0f) {
		normal[2] = 1.0f; /* other axis set to 0.0 */
	}

	return normal;
}

ROSE_STATIC float3 poly_normal_calc(const Span<float3> vert_positions, const Span<int> poly_verts) {
	if (poly_verts.size() > 4) {
		return normal_calc_ngon(vert_positions, poly_verts);
	}
	if (poly_verts.size() == 3) {
		return math::normal_tri(vert_positions[poly_verts[0]], vert_positions[poly_verts[1]], vert_positions[poly_verts[2]]);
	}
	if (poly_verts.size() == 4) {
		float3 normal;
		normal_quad_v3(normal, vert_positions[poly_verts[0]], vert_positions[poly_verts[1]], vert_positions[poly_verts[2]], vert_positions[poly_verts[3]]);
		return normal;
	}
	/* horrible, two sided face! */
	return float3(0);
}

void normals_calc_polys(const Span<float3> positions, const OffsetIndices<int> polys, const Span<int> corner_verts, MutableSpan<float3> poly_normals) {
	ROSE_assert(polys.size() == poly_normals.size());
	threading::parallel_for(polys.index_range(), 1024, [&](const IndexRange range) {
		for (const int i : range) {
			poly_normals[i] = poly_normal_calc(positions, corner_verts.slice(polys[i]));
		}
	});
}

void normals_calc_poly_vert(const Span<float3> positions, const OffsetIndices<int> polys, const Span<int> corner_verts, MutableSpan<float3> poly_normals, MutableSpan<float3> vert_normals) {
	/* Zero the vertex normal array for accumulation. */
	memset(vert_normals.data(), 0, vert_normals.as_span().size_in_bytes());

	/* Compute poly normals, accumulating them into vertex normals. */
	{
		threading::parallel_for(polys.index_range(), 1024, [&](const IndexRange range) {
			for (const int poly_i : range) {
				const Span<int> poly_verts = corner_verts.slice(polys[poly_i]);

				float3 &pnor = poly_normals[poly_i];

				const int i_end = poly_verts.size() - 1;

				/* Polygon Normal and edge-vector. */
				/* Inline version of #poly_normal_calc, also does edge-vectors. */
				{
					zero_v3(pnor);
					/* Newell's Method */
					const float *v_curr = positions[poly_verts[i_end]];
					for (int i_next = 0; i_next <= i_end; i_next++) {
						const float *v_next = positions[poly_verts[i_next]];
						add_newell_cross_v3_v3v3(pnor, v_curr, v_next);
						v_curr = v_next;
					}
					if (normalize_v3(pnor) == 0.0f) {
						pnor[2] = 1.0f; /* Other axes set to zero. */
					}
				}

				/* Accumulate angle weighted face normal into the vertex normal. */
				/* Inline version of #accumulate_vertex_normals_poly_v3. */
				{
					float edvec_prev[3], edvec_next[3], edvec_end[3];
					const float *v_curr = positions[poly_verts[i_end]];
					sub_v3_v3v3(edvec_prev, positions[poly_verts[i_end - 1]], v_curr);
					normalize_v3(edvec_prev);
					copy_v3_v3(edvec_end, edvec_prev);

					for (int i_next = 0, i_curr = i_end; i_next <= i_end; i_curr = i_next++) {
						const float *v_next = positions[poly_verts[i_next]];

						/* Skip an extra normalization by reusing the first calculated edge. */
						if (i_next != i_end) {
							sub_v3_v3v3(edvec_next, v_curr, v_next);
							normalize_v3(edvec_next);
						}
						else {
							copy_v3_v3(edvec_next, edvec_end);
						}

						/* Calculate angle between the two poly edges incident on this vertex. */
						const float fac = saacos(-dot_v3v3(edvec_prev, edvec_next));
						const float vnor_add[3] = {pnor[0] * fac, pnor[1] * fac, pnor[2] * fac};

						float *vnor = vert_normals[poly_verts[i_curr]];
						add_v3_v3_atomic(vnor, vnor_add);
						v_curr = v_next;
						copy_v3_v3(edvec_prev, edvec_next);
					}
				}
			}
		});
	}

	/* Normalize and validate computed vertex normals. */
	{
		threading::parallel_for(positions.index_range(), 1024, [&](const IndexRange range) {
			for (const int vert_i : range) {
				float *no = vert_normals[vert_i];

				if (normalize_v3(no) == 0.0f) {
					/* Following Mesh convention; we use vertex coordinate itself for normal in this
					 * case. */
					normalize_v3_v3(no, positions[vert_i]);
				}
			}
		});
	}
}

}  // namespace rose::kernel::mesh

void KER_mesh_normals_tag_dirty(Mesh *mesh) {
	mesh->runtime->vert_normals_dirty = true;
	mesh->runtime->poly_normals_dirty = true;
}

bool KER_mesh_vertex_normals_are_dirty(const Mesh *mesh) {
	return mesh->runtime->vert_normals_dirty;
}

bool KER_mesh_poly_normals_are_dirty(const Mesh *mesh) {
	return mesh->runtime->poly_normals_dirty;
}

float (*KER_mesh_vert_normals_for_write(struct Mesh *mesh))[3] {
	mesh->runtime->vert_normals.reinitialize(mesh->totvert);
	return reinterpret_cast<float(*)[3]>(mesh->runtime->vert_normals.data());
}
float (*KER_mesh_poly_normals_for_write(struct Mesh *mesh))[3] {
	mesh->runtime->poly_normals.reinitialize(mesh->totpoly);
	return reinterpret_cast<float(*)[3]>(mesh->runtime->poly_normals.data());
}

const float (*KER_mesh_vert_normals_ensure(const struct Mesh *mesh))[3] {
	if (!mesh->runtime->vert_normals_dirty) {
		ROSE_assert(mesh->runtime->vert_normals.size() == mesh->totvert);
		return reinterpret_cast<const float(*)[3]>(mesh->runtime->vert_normals.data());
	}

	std::lock_guard lock{mesh->runtime->normals_mutex};
	if (!mesh->runtime->vert_normals_dirty) {
		ROSE_assert(mesh->runtime->vert_normals.size() == mesh->totvert);
		return reinterpret_cast<const float(*)[3]>(mesh->runtime->vert_normals.data());
	}

	rose::threading::isolate_task([&]() {
		const rose::Span<float3> positions = rose::Span<float3>(
			reinterpret_cast<const float3 *>(KER_mesh_vert_positions(mesh)),
			mesh->totvert
		);
		const rose::OffsetIndices<int> polys = rose::Span<int>(
			reinterpret_cast<const int *>(KER_mesh_poly_offsets(mesh)),
			mesh->totpoly + 1
		);
		const rose::Span<int> corner_verts = rose::Span<int>(
			reinterpret_cast<const int *>(KER_mesh_corner_verts(mesh)),
			mesh->totloop
		);

		mesh->runtime->vert_normals.reinitialize(positions.size());
		mesh->runtime->poly_normals.reinitialize(polys.size());
		rose::kernel::mesh::normals_calc_poly_vert(positions, polys, corner_verts, mesh->runtime->poly_normals, mesh->runtime->vert_normals);
		mesh->runtime->vert_normals_dirty = false;
		mesh->runtime->poly_normals_dirty = false;
	});

	return reinterpret_cast<const float(*)[3]>(mesh->runtime->vert_normals.data());
}
const float (*KER_mesh_poly_normals_ensure(const struct Mesh *mesh))[3] {
	using namespace rose;
	if (!mesh->runtime->poly_normals_dirty) {
		ROSE_assert(mesh->runtime->poly_normals.size() == mesh->totpoly);
		return reinterpret_cast<const float(*)[3]>(mesh->runtime->poly_normals.data());
	}

	std::lock_guard lock{mesh->runtime->normals_mutex};
	if (!mesh->runtime->poly_normals_dirty) {
		ROSE_assert(mesh->runtime->poly_normals.size() == mesh->totpoly);
		return reinterpret_cast<const float(*)[3]>(mesh->runtime->poly_normals.data());
	}

	/* Isolate task because a mutex is locked and computing normals is multi-threaded. */
	threading::isolate_task([&]() {
		const rose::Span<float3> positions = rose::Span<float3>(
			reinterpret_cast<const float3 *>(KER_mesh_vert_positions(mesh)),
			mesh->totvert
		);
		const rose::OffsetIndices<int> polys = rose::Span<int>(
			reinterpret_cast<const int *>(KER_mesh_poly_offsets(mesh)),
			mesh->totpoly + 1
		);
		const rose::Span<int> corner_verts = rose::Span<int>(
			reinterpret_cast<const int *>(KER_mesh_corner_verts(mesh)),
			mesh->totloop
		);

		mesh->runtime->poly_normals.reinitialize(polys.size());
		kernel::mesh::normals_calc_polys(positions, polys, corner_verts, mesh->runtime->poly_normals);

		mesh->runtime->poly_normals_dirty = false;
	});

	return reinterpret_cast<const float(*)[3]>(mesh->runtime->poly_normals.data());
}

void KER_mesh_clear_derived_normals(Mesh *mesh) {
	mesh->runtime->vert_normals.clear_and_shrink();
	mesh->runtime->poly_normals.clear_and_shrink();

	mesh->runtime->vert_normals_dirty = true;
	mesh->runtime->poly_normals_dirty = true;
}
