#pragma once

#include "gpu/gpu_batch.h"
#include "gpu/gpu_primitive.h"
#include "gpu/gpu_shader.h"
#include "gpu/gpu_vertex_format.h"

namespace rose {
namespace gpu {

class Immediate {
public:
	// Pointer to the mapped buffer data for the current vertex.
	unsigned char *mVertexData = nullptr;
	// Current vertex index.
	unsigned int mVertexIdx = 0;
	// Length of the buffer in vertices.
	unsigned int mVertexLen = 0;
	// Which attributes of current vertex have not been given values?
	uint16_t mUnassignedAttrBits = 0;
	// Attributes that needs to be set. One bit per attribute.
	uint16_t mEnabledAttrBits = 0;

	// Current draw call specification.
	int mPrimType = GPU_PRIM_NONE;
	GPU_VertFormat mVertexFormat = {};
	GPU_Shader *mShader = nullptr;
	// Enforce strict vertex count (disabled when using #immBeginAtMost).
	bool mStrictVertexLen = true;

	// Batch in construction when using #immBeginBatch.
	GPU_Batch *mBatch = nullptr;

	float mUniformColor [ 4 ];
public:
	Immediate ( ) { }
	virtual ~Immediate ( ) { }

	virtual unsigned char *Begin ( ) = 0;
	virtual void End ( ) = 0;
};

}
}

void immActivate ( );
void immDeactivate ( );

GPU_Shader *immGetShader ( );
