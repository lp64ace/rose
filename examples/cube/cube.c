#include "MEM_guardedalloc.h"

#include "LIB_math_geom.h"

#include "DNA_screen_types.h"
#include "DNA_windowmanager_types.h"

#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_lib_remap.h"
#include "KER_main.h"
#include "KER_mesh.h"
#include "KER_object.h"

#include "RM_include.h"

int main(void) {
	KER_idtype_init();

    Main *main = KER_main_new();

    RMesh *rm_cube = RM_preset_cube_create((const float[3]){1.0f, 1.0f, 1.0f});
	if (!rm_cube) {
		return 0xf3;
	}

	Mesh *me_cube = (Mesh *)KER_object_obdata_add_from_type(main, OB_MESH, "Cube");
	if (!me_cube) {
		return 0xf4;
	}

	RMeshToMeshParams params = {
		.cd_mask_extra = 0,
	};
	RM_mesh_rm_to_me(main, rm_cube, me_cube, &params);
	RM_mesh_free(rm_cube);

	/**
	 * This code is responsible for printing the vertices and normals of the triangles,
	 * this routine doesn't have much to do with the original test.
	 * 
	 * It provides a way to check if the mesh was correctly created.
	 * Providing insight regarding the well-being of both the rosemesh and the rosekernel module, 
	 * the first one is responsible for creating the mesh, the second one is responsible for tessellating it into triangles.
	 */
	const float (*vert)[3] = KER_mesh_vert_positions(me_cube);
	const float (*norm)[3] = KER_mesh_vert_normals_ensure(me_cube);
	const MLoopTri *tris = KER_mesh_looptris(me_cube);
	for (int i = 0; i < poly_to_tri_count(me_cube->totpoly, me_cube->totloop); i++) {
		const unsigned int *idx = tris[i].tri;
        fprintf(stdout, "Triangle %u\n", i);
        for (int j = 0; j < ARRAY_SIZE(tris[i].tri); j++) {
            const float *v = vert[idx[j]];
            const float *n = norm[idx[j]];
            fprintf(stdout, "\tv(%4.1f %4.1f %4.1f)\tn(%4.1f %4.1f %4.1f)\n", v[0], v[1], v[2], n[0], n[1], n[2]);
        }
	}

    KER_main_free(main);
    return 0;
}