#ifndef KER_MESH_HH
#define KER_MESH_HH

#include "KER_mesh.h"

#include "LIB_math_vector_types.hh"
#include "LIB_offset_indices.hh"
#include "LIB_span.hh"

ROSE_INLINE rose::Span<float3> KER_mesh_vert_positions_span(const Mesh *mesh) {
	return rose::Span<float3>(reinterpret_cast<const float3 *>(KER_mesh_vert_positions(mesh)), mesh->totvert);
}

ROSE_INLINE rose::MutableSpan<float3> KER_mesh_vert_positions_for_write_span(Mesh *mesh) {
	return rose::MutableSpan<float3>(reinterpret_cast<float3 *>(KER_mesh_vert_positions_for_write(mesh)), mesh->totvert);
}

ROSE_INLINE rose::Span<int2> KER_mesh_edges_span(const Mesh *mesh) {
	return rose::Span<int2>(reinterpret_cast<const int2 *>(KER_mesh_edges(mesh)), mesh->totedge);
}

ROSE_INLINE rose::MutableSpan<int2> KER_mesh_edges_for_write_span(Mesh *mesh) {
	return rose::MutableSpan<int2>(reinterpret_cast<int2 *>(KER_mesh_edges_for_write(mesh)), mesh->totedge);
}

ROSE_INLINE rose::Span<int> KER_mesh_corner_verts_span(const Mesh *mesh) {
	return rose::Span<int>(KER_mesh_corner_verts(mesh), mesh->totloop);
}

ROSE_INLINE rose::MutableSpan<int> KER_mesh_corner_verts_for_write_span(Mesh *mesh) {
	return rose::MutableSpan<int>(KER_mesh_corner_verts_for_write(mesh), mesh->totloop);
}

ROSE_INLINE rose::Span<int> KER_mesh_corner_edges_span(const Mesh *mesh) {
	return rose::Span<int>(KER_mesh_corner_edges(mesh), mesh->totloop);
}

ROSE_INLINE rose::MutableSpan<int> KER_mesh_corner_edges_for_write_span(Mesh *mesh) {
	return rose::MutableSpan<int>(KER_mesh_corner_edges_for_write(mesh), mesh->totloop);
}

ROSE_INLINE rose::Span<int> KER_mesh_poly_offsets_span(const Mesh *mesh) {
	return rose::Span<int>(KER_mesh_poly_offsets(mesh), mesh->totpoly + 1);
}

ROSE_INLINE rose::MutableSpan<int> KER_mesh_poly_offsets_for_write_span(Mesh *mesh) {
	return rose::MutableSpan<int>(KER_mesh_poly_offsets_for_write(mesh), mesh->totpoly + 1);
}

ROSE_INLINE rose::Span<float3> KER_mesh_vert_normals_span(const Mesh *mesh) {
	return rose::Span<float3>(reinterpret_cast<const float3 *>(KER_mesh_vert_normals_ensure(mesh)), mesh->totvert);
}

ROSE_INLINE rose::Span<float3> KER_mesh_poly_normals_span(const Mesh *mesh) {
	return rose::Span<float3>(reinterpret_cast<const float3 *>(KER_mesh_poly_normals_ensure(mesh)), mesh->totpoly);
}

ROSE_INLINE rose::Span<float3> KER_mesh_corner_normals_span(const Mesh *mesh) {
	return rose::Span<float3>(reinterpret_cast<const float3 *>(KER_mesh_corner_normals_ensure(mesh)), mesh->totloop);
}

ROSE_INLINE rose::Span<MDeformVert> KER_mesh_deform_verts_span(const Mesh *mesh) {
	return rose::Span<MDeformVert>(KER_mesh_deform_verts(mesh), mesh->totvert);
}

ROSE_INLINE rose::MutableSpan<MDeformVert> KER_mesh_deform_verts_for_write_span(Mesh *mesh) {
	return rose::MutableSpan<MDeformVert>(KER_mesh_deform_verts_for_write(mesh), mesh->totvert);
}

rose::GroupedSpan<int> KER_mesh_vert_to_face_map_span(const Mesh *mesh);

/* -------------------------------------------------------------------- */
/** \name Topology Queries
 * \{ */

namespace rose::kernel::mesh {

/**
 * Return the index of the edge's vertex that is not the \a vert.
 */
ROSE_INLINE int edge_other_vert(const int2 edge, const int vert) {
	ROSE_assert(ELEM(vert, edge[0], edge[1]));
	ROSE_assert(edge[0] >= 0);
	ROSE_assert(edge[1] >= 0);
	/* Order is important to avoid overflow. */
	return (edge[0] - vert) + edge[1];
}

/**
 * Find the index of the previous corner in the face, looping to the end if necessary.
 * The indices are into the entire corners array, not just the face's corners.
 */
ROSE_INLINE int face_corner_prev(const rose::IndexRange face, const int corner) {
	return corner - 1 + (corner == face.start()) * face.size();
}

/**
 * Find the index of the next corner in the face, looping to the start if necessary.
 * The indices are into the entire corners array, not just the face's corners.
 */
ROSE_INLINE int face_corner_next(const rose::IndexRange face, const int corner) {
	if (corner == face.last()) {
		return face.start();
	}
	return corner + 1;
}

/**
 * Find the index of the corner in the face that uses the given vertex.
 * The index is into the entire corners array, not just the face's corners.
 */
ROSE_INLINE int face_find_corner_from_vert(const rose::IndexRange face, const rose::Span<int> corner_verts, const int vert) {
	return face[corner_verts.slice(face).first_index(vert)];
}

/**
 * Return the vertex indices on either side of the given vertex, ordered based on the winding
 * direction of the face. The vertex must be in the face.
 */
ROSE_INLINE int2 face_find_adjacent_verts(const rose::IndexRange face, const rose::Span<int> corner_verts, const int vert) {
	const int corner = face_find_corner_from_vert(face, corner_verts, vert);
	return {corner_verts[face_corner_prev(face, corner)], corner_verts[face_corner_next(face, corner)]};
}

}  // namespace rose::kernel::mesh

/** \} */

#endif // KER_MESH_HH
