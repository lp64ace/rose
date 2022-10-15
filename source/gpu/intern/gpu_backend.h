#pragma once

#include "gpu_info_private.h"

namespace rose {
namespace gpu {

class Context;
class Shader;
class Texture;
class VertBuf;
class IndexBuf;
class UniformBuf;
class StorageBuf;
class Batch;
class FrameBuffer;

class GPU_Backend {
public:
	virtual ~GPU_Backend ( ) = default;
	virtual void DeleteResources ( ) = 0;

	static GPU_Backend *Get ( );

	virtual Context *ContextAlloc ( void *ghost_window ) = 0;
	virtual Shader *ShaderAlloc ( const char *name ) = 0;
	virtual Texture *TextureAlloc ( const char *name ) = 0;
	virtual VertBuf *VertBufAlloc ( ) = 0;
	virtual IndexBuf *IndexBufAlloc ( ) = 0;
	virtual UniformBuf *UniformBufAlloc ( size_t size , const char *name ) = 0;
	virtual StorageBuf *StorageBufAlloc ( size_t size , int usage , const char *name ) = 0;
	virtual Batch *BatchAlloc ( ) = 0;
	virtual FrameBuffer *FrameBufferAlloc ( const char *name ) = 0;

	virtual void ComputeDispatch ( int groups_x_len , int groups_y_len , int groups_z_len ) = 0;
};

}
}
