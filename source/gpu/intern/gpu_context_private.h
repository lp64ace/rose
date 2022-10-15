#pragma once

#include "gpu/gpu_context.h"

#include "gpu_shader_private.h"
#include "gpu_state_private.h"
#include "gpu_immediate_private.h"

struct GPU_MatrixState;

namespace rose {
namespace gpu {

class FrameBuffer;

class Context {
public:
	struct ::GPU_MatrixState *MatrixState = nullptr;
	StateManager *StateManager = nullptr;
	FrameBuffer *ActiveFb = nullptr;
	Shader *Shader = nullptr;
	Immediate *Imm = nullptr;

	/**
	* All 4 window frame-buffers.
	* None of them are valid in an off-screen context.
	* Right frame-buffers are only available if using stereo rendering.
	* Front frame-buffers contains (in principle, but not always) the last frame color.
	* Default frame-buffer is back_left.
	*/
	FrameBuffer *BackLeft = nullptr;
	FrameBuffer *FrontLeft = nullptr;
	FrameBuffer *BackRight = nullptr;
	FrameBuffer *FrontRight = nullptr;

	static int ContextCounter;
	int mContextId;
protected:
	void *mGhostWindow;
public:
	Context ( );
	virtual ~Context ( );

	static Context *Get ( );

	virtual void Activate ( ) = 0;
	virtual void Deactivate ( ) = 0;

	// Will push all pending commands to the GPU.
	virtual void Flush ( ) = 0;
	// Will wait until the GPU has finished executing all commands.
	virtual void Finish ( ) = 0;

	virtual void GetMemoryStatistics ( int *total , int *free ) = 0;
};

/* Syntactic sugar. */
static inline GPU_Context *wrap ( Context *ctx ) {
	return reinterpret_cast< GPU_Context * >( ctx );
}
static inline Context *unwrap ( GPU_Context *ctx ) {
	return reinterpret_cast< Context * >( ctx );
}
static inline const Context *unwrap ( const GPU_Context *ctx ) {
	return reinterpret_cast< const Context * >( ctx );
}

}
}
