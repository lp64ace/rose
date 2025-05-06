#include "MEM_guardedalloc.h"

#include "LIB_math_vector.h"
#include "LIB_math_vector.hh"
#include "LIB_utildefines.h"

#include "RM_include.h"

RMesh *RM_preset_cube_create(const float dim[3]) {
	RMesh *mesh = RM_mesh_create();

	RMVert *verts[4];

	// -X
	do {
		int vert_i = 0;
		verts[vert_i++] = RM_vert_create(mesh, float3(-dim[0], -dim[1],  dim[2]), NULL, RM_CREATE_NOP);
		verts[vert_i++] = RM_vert_create(mesh, float3(-dim[0],  dim[1],  dim[2]), NULL, RM_CREATE_NOP);
		verts[vert_i++] = RM_vert_create(mesh, float3(-dim[0],  dim[1], -dim[2]), NULL, RM_CREATE_NOP);
		verts[vert_i++] = RM_vert_create(mesh, float3(-dim[0], -dim[1], -dim[2]), NULL, RM_CREATE_NOP);

		RM_face_create_verts(mesh, verts, ARRAY_SIZE(verts), NULL, RM_CREATE_NOP, true);
	} while (false);

	// +X
	do {
		int vert_i = 0;
		verts[vert_i++] = RM_vert_create(mesh, float3( dim[0],  dim[1], -dim[2]), NULL, RM_CREATE_NOP);
		verts[vert_i++] = RM_vert_create(mesh, float3( dim[0],  dim[1],  dim[2]), NULL, RM_CREATE_NOP);
		verts[vert_i++] = RM_vert_create(mesh, float3( dim[0], -dim[1],  dim[2]), NULL, RM_CREATE_NOP);
		verts[vert_i++] = RM_vert_create(mesh, float3( dim[0], -dim[1], -dim[2]), NULL, RM_CREATE_NOP);

		RM_face_create_verts(mesh, verts, ARRAY_SIZE(verts), NULL, RM_CREATE_NOP, true);
	} while (false);

	// -Y
	do {
		int vert_i = 0;
		verts[vert_i++] = RM_vert_create(mesh, float3( dim[0], -dim[1], -dim[2]), NULL, RM_CREATE_NOP);
		verts[vert_i++] = RM_vert_create(mesh, float3( dim[0], -dim[1],  dim[2]), NULL, RM_CREATE_NOP);
		verts[vert_i++] = RM_vert_create(mesh, float3(-dim[0], -dim[1],  dim[2]), NULL, RM_CREATE_NOP);
		verts[vert_i++] = RM_vert_create(mesh, float3(-dim[0], -dim[1], -dim[2]), NULL, RM_CREATE_NOP);

		RM_face_create_verts(mesh, verts, ARRAY_SIZE(verts), NULL, RM_CREATE_NOP, true);
	} while (false);

	// +Y
	do {
		int vert_i = 0;
		verts[vert_i++] = RM_vert_create(mesh, float3(-dim[0], dim[1],  dim[2]), NULL, RM_CREATE_NOP);
		verts[vert_i++] = RM_vert_create(mesh, float3( dim[0], dim[1],  dim[2]), NULL, RM_CREATE_NOP);
		verts[vert_i++] = RM_vert_create(mesh, float3( dim[0], dim[1], -dim[2]), NULL, RM_CREATE_NOP);
		verts[vert_i++] = RM_vert_create(mesh, float3(-dim[0], dim[1], -dim[2]), NULL, RM_CREATE_NOP);

		RM_face_create_verts(mesh, verts, ARRAY_SIZE(verts), NULL, RM_CREATE_NOP, true);
	} while (false);

	// -Z
	do {
		int vert_i = 0;
		verts[vert_i++] = RM_vert_create(mesh, float3(-dim[0],  dim[1], -dim[2]), NULL, RM_CREATE_NOP);
		verts[vert_i++] = RM_vert_create(mesh, float3( dim[0],  dim[1], -dim[2]), NULL, RM_CREATE_NOP);
		verts[vert_i++] = RM_vert_create(mesh, float3( dim[0], -dim[1], -dim[2]), NULL, RM_CREATE_NOP);
		verts[vert_i++] = RM_vert_create(mesh, float3(-dim[0], -dim[1], -dim[2]), NULL, RM_CREATE_NOP);

		RM_face_create_verts(mesh, verts, ARRAY_SIZE(verts), NULL, RM_CREATE_NOP, true);
	} while (false);

	// +Z
	do {
		int vert_i = 0;
		verts[vert_i++] = RM_vert_create(mesh, float3( dim[0], -dim[1], dim[2]), NULL, RM_CREATE_NOP);
		verts[vert_i++] = RM_vert_create(mesh, float3( dim[0],  dim[1], dim[2]), NULL, RM_CREATE_NOP);
		verts[vert_i++] = RM_vert_create(mesh, float3(-dim[0],  dim[1], dim[2]), NULL, RM_CREATE_NOP);
		verts[vert_i++] = RM_vert_create(mesh, float3(-dim[0], -dim[1], dim[2]), NULL, RM_CREATE_NOP);

		RM_face_create_verts(mesh, verts, ARRAY_SIZE(verts), NULL, RM_CREATE_NOP, true);
	} while (false);

	return mesh;
}
