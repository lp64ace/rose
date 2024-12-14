#include "MEM_guardedalloc.h"

#include "LIB_utildefines.h"
#include "LIB_math_vector.h"

#include "RM_include.h"

RMesh *RM_preset_cube_create(const float dim[3]) {
	RMesh *mesh = RM_mesh_create();

	const char axis[3] = {'X', 'Y', 'Z'};
	const char sign[2] = {-1, +1};
	const float plane2D_co[4][2] = {{-1.0f, -1.0f}, {+1.0f, -1.0f}, {+1.0f, +1.0f}, {-1.0f, +1.0f}};

	for (int axis_i = 0; axis_i < ARRAY_SIZE(axis); axis_i++) {
		for (int sign_i = 0; sign_i < ARRAY_SIZE(sign); sign_i++) {
			const char axis_v = axis[axis_i];
			const char sign_v = sign[sign_i];

			RMVert *verts[4];

			const int loop_itr = axis_i * 2 + sign_i;

			for (int vert_i = 0; vert_i < ARRAY_SIZE(plane2D_co); vert_i++) {
				float v_co[3];

				int plane2d_co_itr = 0;

				for (int i = 0; i < ARRAY_SIZE(v_co); i++) {
					const int i_v = (i) % ARRAY_SIZE(v_co);
					v_co[i_v] = (i_v == axis_i) ? sign_v : plane2D_co[vert_i][plane2d_co_itr++];
					v_co[i_v] *= dim[i_v];
				}

				verts[vert_i] = RM_vert_create(mesh, v_co, NULL, RM_CREATE_NOP);
			}

			RM_face_create_verts(mesh, verts, ARRAY_SIZE(verts), NULL, RM_CREATE_NOP, true);
		}
	}

	return mesh;
}
