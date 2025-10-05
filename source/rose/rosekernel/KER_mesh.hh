#ifndef KER_MESH_HH
#define KER_MESH_HH

#include "KER_mesh.h"

#include "LIB_math_vector_types.hh"
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
	return rose::Span<int>(KER_mesh_poly_offsets(mesh), mesh->totpoly);
}

ROSE_INLINE rose::MutableSpan<int> KER_mesh_poly_offsets_for_write_span(Mesh *mesh) {
	return rose::MutableSpan<int>(KER_mesh_poly_offsets_for_write(mesh), mesh->totpoly);
}

ROSE_INLINE rose::Span<float3> KER_mesh_vert_normals_span(const Mesh *mesh) {
	return rose::Span<float3>(reinterpret_cast<const float3 *>(KER_mesh_vert_normals_ensure(mesh)), mesh->totvert);
}

ROSE_INLINE rose::MutableSpan<float3> KER_mesh_vert_normals_for_write_span(Mesh *mesh) {
	return rose::MutableSpan<float3>(reinterpret_cast<float3 *>(KER_mesh_vert_normals_for_write(mesh)), mesh->totvert);
}

#endif // KER_MESH_HH
