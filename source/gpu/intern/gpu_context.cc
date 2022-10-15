#include "gpu_context_private.h"
#include "gpu_matrix_private.h"
#include "gpu_backend.h"

#include "ghost/ghost_api.h"

#include "gpu/gpu_state.h"
#include "gpu/gpu_framebuffer.h"
#include "gpu/gpu_init_exit.h"

static thread_local rose::gpu::Context *active_ctx = nullptr;
static int num_backend_users = 0;

namespace rose {
namespace gpu {

// gpu_backend.cpp
void gpu_backend_create ( );
void gpu_backend_discard ( );

int Context::ContextCounter = 0;

Context::Context ( ) {
	this->mGhostWindow = nullptr;
	this->mContextId = Context::ContextCounter++;
	this->MatrixState = GPU_matrix_state_create ( );
}

Context::~Context ( ) {
	GPU_matrix_state_discard ( this->MatrixState );
	delete this->FrontLeft;
	delete this->BackLeft;
	delete this->FrontRight;
	delete this->BackRight;
	delete this->Imm;
}

Context *Context::Get ( ) {
	return active_ctx;
}

}
}

using namespace rose::gpu;

GPU_Context *GPU_context_create ( void *ghost_window ) {
	if ( ghostActivateWindowContext ( ( GHOST_WindowHandle ) ghost_window ) ) {
		{
			if ( num_backend_users == 0 ) {
				// Automatically create backend when first context is created.
				gpu_backend_create ( );
				GPU_init ( );
			}
			num_backend_users++;
		}

		Context *ctx = GPU_Backend::Get ( )->ContextAlloc ( ghost_window );

		GPU_context_active_set ( wrap ( ctx ) );
		return wrap ( ctx );
	}
	return nullptr;
}

void GPU_context_discard ( GPU_Context *ctx_ ) {
	Context *ctx = unwrap ( ctx_ );
	delete ctx;
	active_ctx = nullptr;

	{
		num_backend_users--;
		assert ( num_backend_users >= 0 );
		if ( num_backend_users == 0 ) {
			// Discard backend when last context is discarded.
			GPU_exit ( );
			gpu_backend_discard ( );
		}
	}
}

void GPU_context_active_set ( GPU_Context *ctx_ ) {
	Context *ctx = unwrap ( ctx_ );

	if ( active_ctx ) {
		active_ctx->Deactivate ( );
	}

	active_ctx = ctx;

	if ( ctx ) {
		ctx->Activate ( );
	}
}

GPU_Context *GPU_context_active_get ( ) {
	return wrap ( Context::Get ( ) );
}

void GPU_flush ( void ) { return Context::Get ( )->Flush ( ); }
void GPU_finish ( void ) { return Context::Get ( )->Finish ( ); }
void GPU_apply_state ( void ) { return Context::Get ( )->StateManager->ApplyState ( ); }
