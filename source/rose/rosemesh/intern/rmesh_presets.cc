#include "MEM_alloc.h"

#include "LIB_utildefines.h"

#include "RM_include.h"

RMesh *RM_preset_cube_create(const float dim[3]) {
	RMesh *mesh = RM_mesh_create();

	const char axis[3] = {'X', 'Y', 'Z'};
	const char sign[2] = {-1, +1};
	const float plane2D_co[4][2] = {{-1.0f, -1.0f}, {+1.0f, -1.0f}, {+1.0f, +1.0f}, {-1.0f, +1.0f}};

	const float uv_sz[2] = {1.0f / 4.0f, 1.0f / 3.0f};

	for (int axis_i = 0; axis_i < ARRAY_SIZE(axis); axis_i++) {
		for (int sign_i = 0; sign_i < ARRAY_SIZE(sign); sign_i++) {
			const char axis_v = axis[axis_i];
			const char sign_v = sign[sign_i];

			/**
			 * Since we want to unwrap the Cube open in the texture we will have to use the
			 * following example :
			 *   ^
			 *   |      |----|
			 *   |      | +y |
			 *   | |----|----|----|----|
			 *   | | -x | +z | +x | -z |
			 *   | |----|----|----|----|
			 *   |      | -y |
			 *   |      |----|
			 * [0,0]--------------------------->
			 * The loop will iterate in the following order : -X , +X , -Y , +Y , -Z , +Z.
			 */
			const float uv_cy = (axis_v == 'Y') * sign_v + 1.5f;
			const float uv_cx = ((axis_v == 'Z') ? 2.5f : 1.5f) + (1 - axis_i) * sign_v;

			RMVert *verts[4];

			const int loop_itr = axis_i * 2 + sign_i;

			for (int vert_i = 0; vert_i < ARRAY_SIZE(plane2D_co); vert_i++) {
				float v_co[3];
				float v_uv[2] = {uv_cx, uv_cy};

				int plane2d_co_itr = 0;

				for (int i = 0; i < ARRAY_SIZE(v_co); i++) {
					const int i_v = (i) % ARRAY_SIZE(v_co);
					v_co[i_v] = (i_v == axis_i) ? sign_v : plane2D_co[vert_i][plane2d_co_itr++];
					v_co[i_v] *= dim[i_v];
				}

				/**
				 * TODO: unwrap the uv coordinates appropriately around the uv_cx and uv_cy to show
				 * the texture in correct orientation.
				 */

				mul_v2_v2(v_uv, uv_sz);

				verts[vert_i] = RM_vert_create(mesh, v_co, NULL, RM_CREATE_NOP);
			}

			RM_face_create_verts(mesh, verts, ARRAY_SIZE(verts), NULL, RM_CREATE_NOP, true);
		}
	}

	return mesh;
}
