#include "ghost/ghost_api.h"
#include "gpu/gpu.h"

int main ( void ) {
	GHOST_SystemHandle instance = ghostCreateSystem ( );

	{
		GHOST_WindowHandle window = ghostSpawnWindow ( instance , NULL , "Rose" , GHOST_kUseDefault , GHOST_kUseDefault , GHOST_kUseDefault , GHOST_kUseDefault );
		ghostSetWindowDrawingContext ( window , GHOST_kDrawingContextTypeOpenGL );
		GPU_Context *context = GPU_context_create ( window );

		ghostSetSwapInterval ( window , 0 );

		GPU_init ( );

		GPU_Shader *shader = GPU_shader_get_builtin_shader ( GPU_SHADER_DEFAULT_DIFFUSE );

		GPU_VertFormat *format = GPU_vertformat_create ( );
		GPU_vertformat_from_shader ( format , shader );

		GPU_VertBuf *verts = GPU_vertbuf_create_with_format ( format );

		const int apos = GPU_vertformat_attr_id_get ( format , "pos" );

		const float position_data [ ] = {
			 0.75f ,  0.75f , 0.0f ,
			-0.75f ,  0.75f , 0.0f ,
			-0.75f , -0.75f , 0.0f ,
			 0.75f , -0.75f , 0.0f
		};

		GPU_vertbuf_data_alloc ( verts , 4 );
		GPU_vertbuf_attr_set ( verts , apos , 0 , position_data + 0 );
		GPU_vertbuf_attr_set ( verts , apos , 1 , position_data + 3 );
		GPU_vertbuf_attr_set ( verts , apos , 2 , position_data + 6 );
		GPU_vertbuf_attr_set ( verts , apos , 3 , position_data + 9 );

		GPU_IndexBuf *ibo = GPU_indexbuf_calloc ( );

		{
			GPU_IndexBufBuilder ibo_builder = { };
			GPU_indexbuf_init_ex ( &ibo_builder , GPU_PRIM_TRIS , 6 , 4 );
			GPU_indexbuf_add_tri_verts ( &ibo_builder , 0 , 1 , 2 );
			GPU_indexbuf_add_tri_verts ( &ibo_builder , 2 , 3 , 0 );
			GPU_indexbuf_build_in_place ( &ibo_builder , ibo );
		}

		GPU_vertformat_free ( format );

		GPU_Batch *batch = GPU_batch_create_ex ( GPU_PRIM_TRIS , verts , ibo , GPU_BATCH_OWNS_VBO | GPU_BATCH_OWNS_INDEX );

		float t1 = ( float ) ghostGetTime ( instance );

		while ( !ghostGetSystemShouldExit ( instance ) ) {
			ghostProcessEvents ( instance );
			ghostDispatchEvents ( instance );
			if ( ghostActivateWindowContext ( window ) ) {
				float t2 = ghostGetTime ( instance );
				float dt = t2 - t1;

				GPU_context_active_set ( context );
				GPU_clear_color ( 0.075f , 0.075f , 0.075f , 1.0f );
				GPU_clear_depth ( 1.0f );

				GPU_matrix_push ( );
				GPU_matrix_push_projection ( );
				GPU_matrix_identity_set ( );

				GPU_matrix_rotate_3f ( 45.0f * t2 , 0.0f , 1.0f , 0.0f );
				GPU_matrix_rotate_3f ( 20.0f * t2 , 1.0f , 1.0f , 0.0f );

				GPU_face_culling ( GPU_CULL_NONE );

				GPU_batch_set_shader ( batch , shader );
				GPU_batch_uniform_4f ( batch , "color" , 0.85f , 0.5f , 0.25f , 1.0f );
				GPU_batch_draw ( batch );

				GPU_matrix_pop_projection ( );
				GPU_matrix_pop ( );

				t1 = t2;

				ghostSwapBuffers ( window );
			}
		}

		GPU_batch_discard ( batch );

		GPU_exit ( );

		GPU_context_discard ( context );
	}

	ghostDisposeSystem ( instance );
	return 0;
}
