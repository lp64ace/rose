#pragma once

#include "gpu/intern/gpu_backend.h"

#include "gl_context.h"
#include "gl_shader.h"
#include "gl_compute.h"
#include "gl_texture.h"
#include "gl_vertex_buffer.h"
#include "gl_index_buffer.h"
#include "gl_uniform_buffer.h"
#include "gl_storage_buffer.h"
#include "gl_batch.h"
#include "gl_framebuffer.h"

int get_gl_version ( );
bool has_gl_extension ( const char *name );

namespace rose {
namespace gpu {

class GLBackend : public GPU_Backend {
	GLSharedOrphanLists mSharedOrphanList;
public:
	GLBackend ( );
	~GLBackend ( );

	static GLBackend *Get ( ) { return static_cast< GLBackend * >( GPU_Backend::Get ( ) ); }

	Context *ContextAlloc ( void *ghost_window ) { return new GLContext ( ghost_window , mSharedOrphanList ); }
	Shader *ShaderAlloc ( const char *name ) { return new GLShader ( name ); }
	Texture *TextureAlloc ( const char *name ) { return new GLTexture ( name ); }
	VertBuf *VertBufAlloc ( ) { return new GLVertBuf ( ); }
	IndexBuf *IndexBufAlloc ( ) { return new GLIndexBuf ( ); }
	UniformBuf *UniformBufAlloc ( size_t size , const char *name ) { return new GLUniformBuf ( size , name ); }
	StorageBuf *StorageBufAlloc ( size_t size , int usage , const char *name ) { return new GLStorageBuf ( size , usage , name ); }
	Batch *BatchAlloc ( ) { return new GLBatch ( ); }
	FrameBuffer *FrameBufferAlloc ( const char *name ) { return new GLFrameBuffer ( name ); }

	void DeleteResources ( );

	void ComputeDispatch ( int groups_x_len , int groups_y_len , int groups_z_len );

	GLSharedOrphanLists *SharedOrphanListGet ( ) { return &this->mSharedOrphanList; }
private:
	void InitCapabilities ( );

	void InitPlatform ( );
	void ExitPlatform ( );
};

}
}
