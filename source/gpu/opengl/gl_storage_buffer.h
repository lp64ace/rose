#pragma once

#include "gpu/intern/gpu_storage_buffer_private.h"

#include <gl/glew.h>

namespace rose {
namespace gpu {

class GLStorageBuf : public StorageBuf {
private:
	// Slot to which this UBO is currently bound. -1 if not bound.
	int mSlot = -1;
	// OpenGL object handle.
	unsigned int mSSBOId = 0;
	// Usage type.
	int mUsage;
public:
	GLStorageBuf ( size_t size , int usage , const char *name );
	~GLStorageBuf ( );

	void Update ( const void *data ) override;
	void Bind ( int slot ) override;
	void Unbind ( ) override;
	void Clear ( int internal_format , unsigned int data_format , void *data ) override;
	void CopySub ( VertBuf *src , unsigned int dst_offset , unsigned int src_offset , unsigned int copy_size ) override;
	void Read ( void *data ) override;

	// Special internal function to bind SSBOs to indirect argument targets.
	void BindAs ( GLenum target );
private:
	void Init ( );
};

}
}
