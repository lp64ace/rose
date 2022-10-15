#pragma once

#include "gpu/intern/gpu_immediate_private.h"

namespace rose {
namespace gpu {

// size of internal buffer
#define DEFAULT_INTERNAL_BUFFER_SIZE (4 * 1024 * 1024)

class GLImmediate : public Immediate {
private:
	// Use two buffers for strict and non-strict vertex count to 
	// avoid some huge driver slowdown (see T70922).
	// Use accessor functions to get / modify.
	struct {
		unsigned int mVboId = 0; // Opengl Handle for this buffer.
		size_t mBufferOffset = 0; // Offset of the mapped data in data.
		size_t mBufferSize = 0; // Size of the whole buffer in bytes.
	} mBuffer , mBufferStrict ;

	// Size in bytes of the mapped region. 
	size_t mBytesMapped = 0;
	// Vertex array for this immediate mode instance.
	unsigned int mVaoId = 0;
public:
	GLImmediate ( );
	~GLImmediate ( );

	unsigned char *Begin ( ) override;
	void End ( ) override;
private:
	unsigned int &GetVboId ( ) { return this->mStrictVertexLen ? this->mBufferStrict.mVboId : this->mBuffer.mVboId; }
	size_t &GetBufOffset ( ) { return this->mStrictVertexLen ? this->mBufferStrict.mBufferOffset : this->mBuffer.mBufferOffset; }
	size_t &GetBufSize ( ) { return this->mStrictVertexLen ? this->mBufferStrict.mBufferSize : this->mBuffer.mBufferSize; }
};

}
}
