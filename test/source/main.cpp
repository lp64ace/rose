#include "ghost/ghost_api.h"
#include "gpu/gpu.h"

#include <stdlib.h>

int main ( void ) {
	GHOST_SystemHandle instance = ghostCreateSystem ( );

	{
		GHOST_WindowHandle window = ghostSpawnWindow ( instance , NULL , "Rose" , GHOST_kUseDefault , GHOST_kUseDefault , GHOST_kUseDefault , GHOST_kUseDefault );
		ghostSetWindowDrawingContext ( window , GHOST_kDrawingContextTypeOpenGL );
		GPU_Context *context = GPU_context_create ( window );

		GPU_init ( );

		while ( !ghostGetSystemShouldExit ( instance ) ) {
			ghostProcessEvents ( instance );
			ghostDispatchEvents ( instance );
			if ( ghostActivateWindowContext ( window ) ) {
				GPU_context_active_set ( context );
				GPU_clear_color ( 0.0825f , 0.0825f , 0.0825f , 1.0f );
				GPU_clear_depth ( 1.0f );

				ghostSwapBuffers ( window );
			}
		}

		GPU_exit ( );

		GPU_context_discard ( context );
	}

	ghostDisposeSystem ( instance );
	return 0;
}
