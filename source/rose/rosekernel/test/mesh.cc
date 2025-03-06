#include "MEM_guardedalloc.h"

#include "LIB_math_geom.h"
#include "LIB_math_vector_types.hh"

#include "DNA_screen_types.h"
#include "DNA_windowmanager_types.h"

#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_lib_remap.h"
#include "KER_main.h"
#include "KER_mesh.h"
#include "KER_object.h"

#include "RM_include.h"

#include "gtest/gtest.h"

namespace {

TEST(Mesh, Tesselation) {
    KER_idtype_init();
    
    Main *main = KER_main_new();
    do {
        RMesh *rm_cube = RM_preset_cube_create((const float[3]){1.0f, 1.0f, 1.0f});
	    EXPECT_NE(rm_cube, nullptr);

        Mesh *me_cube = (Mesh *)KER_object_obdata_add_from_type(main, OB_MESH, "Cube");
        EXPECT_NE(me_cube, nullptr);
    
        RMeshToMeshParams params = {
		    .cd_mask_extra = 0,
	    };
	    RM_mesh_rm_to_me(main, rm_cube, me_cube, &params);

        EXPECT_EQ(me_cube->totvert, 4 * 6);
        EXPECT_EQ(me_cube->totedge, 4 * 6);
        EXPECT_EQ(me_cube->totloop, 4 * 6);
        /* Since we are working with triangles there are two trianles in each face. */
        EXPECT_EQ(poly_to_tri_count(me_cube->totpoly, me_cube->totloop), 2 * 6);

        const float (*vert)[3] = KER_mesh_vert_positions(me_cube);
        const float (*norm)[3] = KER_mesh_vert_normals_ensure(me_cube);
        const MLoopTri *tris = KER_mesh_looptris(me_cube);

        // left
        
        EXPECT_EQ(float3(norm[tris[0].tri[0]]), float3(-1.0f, 0.0f, 0.0f));
        EXPECT_EQ(float3(norm[tris[0].tri[1]]), float3(-1.0f, 0.0f, 0.0f));
        EXPECT_EQ(float3(norm[tris[0].tri[2]]), float3(-1.0f, 0.0f, 0.0f));

        EXPECT_EQ(float3(norm[tris[1].tri[0]]), float3(-1.0f, 0.0f, 0.0f));
        EXPECT_EQ(float3(norm[tris[1].tri[1]]), float3(-1.0f, 0.0f, 0.0f));
        EXPECT_EQ(float3(norm[tris[1].tri[2]]), float3(-1.0f, 0.0f, 0.0f));

        // right

        EXPECT_EQ(float3(norm[tris[2].tri[0]]), float3( 1.0f, 0.0f, 0.0f));
        EXPECT_EQ(float3(norm[tris[2].tri[1]]), float3( 1.0f, 0.0f, 0.0f));
        EXPECT_EQ(float3(norm[tris[2].tri[2]]), float3( 1.0f, 0.0f, 0.0f));

        EXPECT_EQ(float3(norm[tris[3].tri[0]]), float3( 1.0f, 0.0f, 0.0f));
        EXPECT_EQ(float3(norm[tris[3].tri[1]]), float3( 1.0f, 0.0f, 0.0f));
        EXPECT_EQ(float3(norm[tris[3].tri[2]]), float3( 1.0f, 0.0f, 0.0f));

        // bottom

        EXPECT_EQ(float3(norm[tris[4].tri[0]]), float3(0.0f, -1.0f, 0.0f));
        EXPECT_EQ(float3(norm[tris[4].tri[1]]), float3(0.0f, -1.0f, 0.0f));
        EXPECT_EQ(float3(norm[tris[4].tri[2]]), float3(0.0f, -1.0f, 0.0f));

        EXPECT_EQ(float3(norm[tris[5].tri[0]]), float3(0.0f, -1.0f, 0.0f));
        EXPECT_EQ(float3(norm[tris[5].tri[1]]), float3(0.0f, -1.0f, 0.0f));
        EXPECT_EQ(float3(norm[tris[5].tri[2]]), float3(0.0f, -1.0f, 0.0f));

        // top

        EXPECT_EQ(float3(norm[tris[6].tri[0]]), float3(0.0f,  1.0f, 0.0f));
        EXPECT_EQ(float3(norm[tris[6].tri[1]]), float3(0.0f,  1.0f, 0.0f));
        EXPECT_EQ(float3(norm[tris[6].tri[2]]), float3(0.0f,  1.0f, 0.0f));

        EXPECT_EQ(float3(norm[tris[7].tri[0]]), float3(0.0f,  1.0f, 0.0f));
        EXPECT_EQ(float3(norm[tris[7].tri[1]]), float3(0.0f,  1.0f, 0.0f));
        EXPECT_EQ(float3(norm[tris[7].tri[2]]), float3(0.0f,  1.0f, 0.0f));

        // back

        EXPECT_EQ(float3(norm[tris[8].tri[0]]), float3(0.0f, 0.0f, -1.0f));
        EXPECT_EQ(float3(norm[tris[8].tri[1]]), float3(0.0f, 0.0f, -1.0f));
        EXPECT_EQ(float3(norm[tris[8].tri[2]]), float3(0.0f, 0.0f, -1.0f));

        EXPECT_EQ(float3(norm[tris[9].tri[0]]), float3(0.0f, 0.0f, -1.0f));
        EXPECT_EQ(float3(norm[tris[9].tri[1]]), float3(0.0f, 0.0f, -1.0f));
        EXPECT_EQ(float3(norm[tris[9].tri[2]]), float3(0.0f, 0.0f, -1.0f));

        // front

        EXPECT_EQ(float3(norm[tris[10].tri[0]]), float3(0.0f, 0.0f,  1.0f));
        EXPECT_EQ(float3(norm[tris[10].tri[1]]), float3(0.0f, 0.0f,  1.0f));
        EXPECT_EQ(float3(norm[tris[10].tri[2]]), float3(0.0f, 0.0f,  1.0f));

        EXPECT_EQ(float3(norm[tris[11].tri[0]]), float3(0.0f, 0.0f,  1.0f));
        EXPECT_EQ(float3(norm[tris[11].tri[1]]), float3(0.0f, 0.0f,  1.0f));
        EXPECT_EQ(float3(norm[tris[11].tri[2]]), float3(0.0f, 0.0f,  1.0f));

        RM_mesh_free(rm_cube);
    } while (false);
    KER_main_free(main);
}

}  // namespace
