#include "gpu/gpu_init_exit.h"

#include "gpu_shader_create_info_private.h"
#include "gpu_shader_dependency_private.h"

/**
 * although the order of initialization and shutdown should not matter
 * (except for the extensions), I chose alphabetical and reverse alphabetical order
 */

static bool initialized = false;

void GPU_init ( void ) {
	if ( initialized ) {
		return;
	}

	initialized = true;

	gpu_shader_dependency_init ( );
	gpu_shader_create_info_init ( );
}

void GPU_exit ( void ) {
	gpu_shader_dependency_exit ( );
	gpu_shader_create_info_exit ( );

	initialized = false;
}

bool GPU_is_init ( void ) {
	return initialized;
}
