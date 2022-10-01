#pragma once

#include "gpu/intern/gpu_uniform_buffer_private.h"

namespace rose {
namespace gpu {

class GLUniformBuf : public UniformBuf {
private:
	// Slot to which this UBO is currently bound. -1 if not bound.
	int mSlot = -1;
	// OpenGL object handle.
	unsigned int mUboId = 0;
public:
	GLUniformBuf ( size_t size , const char *name );
	~GLUniformBuf ( );

	void Update ( const void *data ) override;
	void Bind ( int slot ) override;
	void Unbind ( ) override;
private:
	void Init ( );
};

}
}
