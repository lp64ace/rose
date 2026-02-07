#include "MEM_guardedalloc.h"

#include "LIB_array_utils.hh"
#include "LIB_offset_indices.hh"
#include "LIB_thread.h"

#include "KER_lib_id.h"
#include "KER_mesh_types.hh"
#include "KER_mesh.hh"

#include "atomic_ops.h"

/* -------------------------------------------------------------------- */
/** \name Mesh Runtime Struct Utils
 * \{ */

void KER_mesh_runtime_clear_batch_cache(Mesh *mesh) {
	KER_mesh_batch_cache_free(mesh);
}

void KER_mesh_runtime_init_data(Mesh *mesh) {
	mesh->runtime = MEM_new<rose::kernel::MeshRuntime>("rose::kernel::MeshRuntime");
}

void KER_mesh_runtime_free_data(Mesh *mesh) {
	if (mesh->runtime) {
		KER_mesh_runtime_clear_cache(mesh);
		KER_mesh_runtime_clear_batch_cache(mesh);

		MEM_delete<rose::kernel::MeshRuntime>(mesh->runtime);
	}
}

void KER_mesh_runtime_clear_cache(Mesh *mesh) {
	if (mesh->runtime->mesh_eval != nullptr) {
		KER_id_free(nullptr, mesh->runtime->mesh_eval);
		mesh->runtime->mesh_eval = nullptr;
	}
	KER_mesh_runtime_clear_geometry(mesh);
	KER_mesh_clear_derived_normals(mesh);
}

void KER_mesh_runtime_clear_geometry(Mesh *mesh) {
	mesh->runtime->bounds_cache.tag_dirty();
	mesh->runtime->looptris_cache.tag_dirty();
	mesh->runtime->looptri_polys_cache.tag_dirty();
}

void KER_mesh_positions_changed(Mesh *mesh) {
	KER_mesh_normals_tag_dirty(mesh);
	KER_mesh_batch_cache_tag_dirty(mesh, KER_MESH_BATCH_DIRTY_ALL);

	mesh->runtime->bounds_cache.tag_dirty();
	mesh->runtime->looptris_cache.tag_dirty();
}

void KER_mesh_positions_changed_uniformly(Mesh *mesh) {
	/* The normals and triangulation didn't change, since all verts moved by the same amount. */
	mesh->runtime->bounds_cache.tag_dirty();
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Mesh Runtime Cache Utils
 * \{ */

namespace rose::kernel::mesh {

static Array<int> create_reverse_offsets(const Span<int> indices, const int items_num) {
	Array<int> offsets(items_num + 1, 0);
	offset_indices::build_reverse_offsets(indices, offsets);
	return offsets;
}

static void sort_small_groups(const OffsetIndices<int> groups, const int grain_size, MutableSpan<int> indices) {
	threading::parallel_for(groups.index_range(), grain_size, [&](const IndexRange range) {
		for (const int64_t index : range) {
			MutableSpan<int> group = indices.slice(groups[index]);
			std::sort(group.begin(), group.end());
		}
	});
}

/* A version of #reverse_indices_in_groups that stores face indices instead of corner indices. */
static void reverse_group_indices_in_groups(const OffsetIndices<int> groups, const Span<int> group_to_elem, const OffsetIndices<int> offsets, MutableSpan<int> results) {
	int *counts = static_cast<int *>(MEM_callocN(sizeof(int) * offsets.size(), __func__));
	ROSE_SCOPED_DEFER([&]() { MEM_freeN(counts); })
	rose::threading::parallel_for(groups.index_range(), 1024, [&](const IndexRange range) {
		for (const size_t face : range) {
			for (const int elem : group_to_elem.slice(groups[face])) {
				const int index_in_group = atomic_fetch_and_add_int32(&counts[elem], 1);
				results[offsets[elem][index_in_group]] = int(face);
			}
		}
	});
	sort_small_groups(offsets, 1024, results);
}

void build_vert_to_face_indices(const rose::OffsetIndices<int> faces, const rose::Span<int> corner_verts, const OffsetIndices<int> offsets, MutableSpan<int> face_indices) {
	reverse_group_indices_in_groups(faces, corner_verts, offsets, face_indices);
}

}  // namespace rose::kernel::mesh

rose::OffsetIndices<int> KER_mesh_vert_to_face_map_offsets(const Mesh *mesh) {
	const rose::Span<int> corner_verts = KER_mesh_corner_verts_span(mesh);
	mesh->runtime->vert_to_face_offset_cache.ensure([&](rose::Array<int> &r_data) {
		r_data = rose::Array<int>(mesh->totvert + 1, 0);
		rose::offset_indices::build_reverse_offsets(corner_verts, r_data);
	});
	return rose::OffsetIndices<int>(mesh->runtime->vert_to_face_offset_cache.data());
}

rose::GroupedSpan<int> KER_mesh_vert_to_face_map_span(const Mesh *mesh) {
	const rose::OffsetIndices<int> offsets = KER_mesh_vert_to_face_map_offsets(mesh);
	mesh->runtime->vert_to_face_map_cache.ensure([&](rose::Array<int> &r_data) {
		r_data.reinitialize(mesh->totloop);

		const rose::OffsetIndices polys = KER_mesh_poly_offsets_span(mesh);
		const rose::Span<int> corner_verts = KER_mesh_corner_verts_span(mesh);

		rose::kernel::mesh::build_vert_to_face_indices(polys, corner_verts, offsets, r_data);
	});
	return rose::GroupedSpan<int>(offsets, mesh->runtime->vert_to_face_map_cache.data());
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Mesh Geometry Evaluation
 * \{ */

std::optional<rose::Bounds<float3>> KER_mesh_evaluated_geometry_bounds(Mesh *mesh) {
	if (mesh->totvert == 0) {
		return std::nullopt;
	}

	mesh->runtime->bounds_cache.ensure([&](rose::Bounds<float3> &r_bounds) {
		const rose::Span<float3> vertices = KER_mesh_vert_positions_span(mesh);

		r_bounds = *rose::bounds::min_max(vertices);
	});
	return mesh->runtime->bounds_cache.data();
}

/** \} */
