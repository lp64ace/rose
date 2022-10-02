#include "gpu_backend.h"

#include "gpu/opengl/gl_backend.h"

#include <stdio.h>
#include <assert.h>

namespace rose {
namespace gpu {

static const int g_backend_type = GPU_BACKEND_OPENGL;
static GPU_Backend *g_backend = nullptr;

void gpu_backend_create ( ) {
	assert ( g_backend == nullptr );
	assert ( GPU_backend_supported ( ) );

	switch ( g_backend_type ) {
		case GPU_BACKEND_OPENGL: {
			g_backend = ( GPU_Backend * ) new GLBackend ( );
		}break;
		default: {
			fprintf ( stderr , "No backend specified" );
		}break;
	}
}

void gpu_backend_discard ( ) {
	delete g_backend; g_backend = nullptr;
}

GPU_Backend *GPU_Backend::Get ( ) {
	return g_backend;
}

}
}

using namespace rose::gpu;

bool GPU_backend_supported ( void ) {
	switch ( g_backend_type ) {
		case GPU_BACKEND_OPENGL: return true;
	}
	fprintf ( stderr , "No backend specified" );
	return false;
}

int GPU_backend_get_type ( void ) {
	return g_backend_type;
}