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

				t1 = t2;
				ghostSwapBuffers ( window );
			}
		}

		GPU_exit ( );

		GPU_context_discard ( context );
	}

	ghostDisposeSystem ( instance );
	return 0;
}
