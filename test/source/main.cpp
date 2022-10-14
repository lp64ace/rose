#include "ghost/ghost_api.h"
#include "gpu/gpu.h"
#include "mesh/bmesh.h"

int main ( void ) {
	GHOST_SystemHandle instance = ghostCreateSystem ( );

	{
		GHOST_WindowHandle window = ghostSpawnWindow ( instance , NULL , "Rose" , GHOST_kUseDefault , GHOST_kUseDefault , GHOST_kUseDefault , GHOST_kUseDefault );
		ghostSetWindowDrawingContext ( window , GHOST_kDrawingContextTypeOpenGL );
		GPU_Context *context = GPU_context_create ( window );

		ghostSetSwapInterval ( window , 0 );

		GPU_init ( );

		GPU_Shader *shader = GPU_shader_get_builtin_shader ( GPU_SHADER_DEFAULT_DIFFUSE );

		const float co [ ] = {
			-1.0f ,  1.0f ,  0.0f ,
			-1.0f , -1.0f ,  0.0f ,
			 1.0f , -1.0f ,  0.0f ,
		};

		float last_upd = ( float ) ghostGetTime ( instance );

		while ( !ghostGetSystemShouldExit ( instance ) ) {
			ghostProcessEvents ( instance );
			ghostDispatchEvents ( instance );
			if ( ghostActivateWindowContext ( window ) ) {
				float curr_upd = ghostGetTime ( instance );
				float dt = curr_upd - last_upd;

				GPU_context_active_set ( context );
				GPU_clear_color ( 0.075f , 0.075f , 0.075f , 1.0f );
				GPU_clear_depth ( 1.0f );

				BMesh *bm = BM_mesh_create ( &bm_mesh_chunksize_default , nullptr );

				BMVert *v1 = BM_vert_create ( bm , co + 0 , nullptr , BM_CREATE_NOP );
				BMVert *v2 = BM_vert_create ( bm , co + 3 , nullptr , BM_CREATE_NOP );
				BMVert *v3 = BM_vert_create ( bm , co + 6 , nullptr , BM_CREATE_NOP );

				BMVert *v_arr [ ] = { v1 , v2 , v3 };

				BMFace *f = BM_face_create_verts ( bm , v_arr , 3 , nullptr , BM_CREATE_NO_DOUBLE , true );

				printf ( "BMesh Face Len : %d\n" , bm->TotFace );

				BM_mesh_elem_table_ensure ( bm , BM_VERT | BM_EDGE | BM_FACE );

				for ( int i = 0; i < bm->TotFace; i++ ) {
					BMFace *f_i = bm->FaceTable [ i ];

					BM_face_normal_update ( f_i );

					printf ( "|-- BMFace : %d\n" , f_i->Len );
					BMLoop *l_first = BM_FACE_FIRST_LOOP ( f_i );
					BMLoop *l_iter = l_first;
					do {
						printf ( "|------ BMVert "
							 "[ Coord = Vec3f(%4.1ff %4.1ff %4.1ff) ]"
							 "[ Normal = Vec3f(%4.1ff %4.1ff %4.1ff) ]\n" ,
							 l_iter->Vert->Coord [ 0 ] ,
							 l_iter->Vert->Coord [ 1 ] ,
							 l_iter->Vert->Coord [ 2 ] ,
							 f_i->Normal [ 0 ] ,
							 f_i->Normal [ 1 ] ,
							 f_i->Normal [ 2 ] );
						l_iter = l_iter->Next;
					} while ( l_first != l_iter );
				}

				printf ( "\n" );

				BM_mesh_free ( bm );

				ghostSwapBuffers ( window );
				last_upd = curr_upd;
			}
		}

		GPU_exit ( );

		GPU_context_discard ( context );
	}

	ghostDisposeSystem ( instance );
	return 0;
}
