#include "gl_vertex_buffer.h"
#include "gl_context.h"

namespace rose {
namespace gpu {

void GLVertBuf::Bind ( ) {
	assert ( GLContext::Get ( ) != nullptr );

	if ( this->mVboId == 0 ) {
		glGenBuffers ( 1 , &this->mVboId );
	}

	glBindBuffer ( GL_ARRAY_BUFFER , this->mVboId );

	if ( this->mFlag & GPU_VERTBUF_DATA_DIRTY ) {
		this->mVboSize = this->SizeUsedGet ( );
		/* Orphan the vbo to avoid sync then upload data. */
		glBufferData ( GL_ARRAY_BUFFER , this->mVboSize , nullptr , usage_to_gl ( this->mUsage ) );
		/* Do not transfer data from host to device when buffer is device only. */
		if ( this->mUsage != GPU_USAGE_DEVICE_ONLY ) {
			glBufferSubData ( GL_ARRAY_BUFFER , 0 , this->mVboSize , this->mData );
		}
		VertBuf::MemoryUsage += this->mVboSize;

		if ( this->mUsage == GPU_USAGE_STATIC ) {
			MEM_SAFE_FREE ( this->mData );
		}
		this->mFlag &= ~GPU_VERTBUF_DATA_DIRTY;
		this->mFlag |= GPU_VERTBUF_DATA_UPLOADED;
	}
}

void GLVertBuf::UpdateSub ( unsigned int start , unsigned int len , const void *data ) {
	glBufferSubData ( GL_ARRAY_BUFFER , start , len , data );
}

const void *GLVertBuf::Read ( ) const {
	assert ( IsActive ( ) );
	void *result = glMapBuffer ( GL_ARRAY_BUFFER , GL_READ_ONLY );
	return result;
}

void *GLVertBuf::Unmap ( const void *mapped ) const {
	void *result = MEM_mallocN ( this->mVboSize , __FUNCTION__ );
	memcpy ( result , mapped , this->mVboSize );
	return result;
}

void GLVertBuf::WrapHandle ( uint64_t handle ) {
	assert ( this->mVboId == 0 );
	assert ( glIsBuffer ( static_cast< unsigned int >( handle ) ) );
	this->mIsWrapper = true;
	this->mVboId = static_cast< unsigned int >( handle );
	/* We assume the data is already on the device, so no need to allocate or send it. */
	this->mFlag = GPU_VERTBUF_DATA_UPLOADED;
}

void GLVertBuf::AcquireData ( ) {
	if ( this->mUsage == GPU_USAGE_DEVICE_ONLY ) {
		return;
	}

	/* Discard previous data if any. */
	MEM_SAFE_FREE ( this->mData );
	this->mData = ( unsigned char * ) MEM_mallocN ( sizeof ( unsigned char ) * this->SizeAllocGet ( ) , __FUNCTION__ );
}

void GLVertBuf::ResizeData ( ) {
	if ( this->mUsage == GPU_USAGE_DEVICE_ONLY ) {
		return;
	}

	this->mData = ( unsigned char * ) MEM_reallocN ( this->mData , sizeof ( unsigned char ) * this->SizeAllocGet ( ) );
}

void GLVertBuf::ReleaseData ( ) {
	if ( this->mIsWrapper ) {
		return;
	}

	if ( this->mVboId != 0 ) {
		if ( this->mBufferTexture ) {
			GPU_texture_free ( this->mBufferTexture ); this->mBufferTexture = NULL;
		}
		GLContext::BufFree ( this->mVboId );
		this->mVboId = 0;
		VertBuf::MemoryUsage -= this->mVboSize;
	}

	MEM_SAFE_FREE ( this->mData );
}

void GLVertBuf::UploadData ( ) {
	this->Bind ( );
}

void GLVertBuf::DuplicateData ( VertBuf *_dst ) {
	assert ( GLContext::Get ( ) != nullptr );
	GLVertBuf *src = this;
	GLVertBuf *dst = static_cast< GLVertBuf * >( _dst );
	dst->mBufferTexture = nullptr;

	if ( src->mVboId != 0 ) {
		dst->mVboSize = src->SizeUsedGet ( );

		glGenBuffers ( 1 , &dst->mVboId );
		glBindBuffer ( GL_COPY_WRITE_BUFFER , dst->mVboId );
		glBufferData ( GL_COPY_WRITE_BUFFER , dst->mVboSize , nullptr , usage_to_gl ( dst->mUsage ) );

		glBindBuffer ( GL_COPY_READ_BUFFER , src->mVboId );

		glCopyBufferSubData ( GL_COPY_READ_BUFFER , GL_COPY_WRITE_BUFFER , 0 , 0 , dst->mVboSize );

		VertBuf::MemoryUsage += dst->mVboSize;
	}

	if ( this->mData != nullptr ) {
		dst->mData = ( unsigned char * ) MEM_dupallocN ( src->mData );
	}
}

void GLVertBuf::BindAsSSBO ( unsigned int binding ) {
	this->Bind ( );
	assert ( this->mVboId != 0 );
	glBindBufferBase ( GL_SHADER_STORAGE_BUFFER , binding , this->mVboId );
}

void GLVertBuf::BindAsTexture ( unsigned int binding ) {
	this->Bind ( );
	assert ( this->mVboId != 0 );
	if ( this->mBufferTexture == nullptr ) {
		this->mBufferTexture = GPU_texture_create_from_vertbuf ( "vertbuf_as_texture" , wrap ( this ) );
	}
	GPU_texture_bind ( this->mBufferTexture , binding );
}

bool GLVertBuf::IsActive ( ) const {
	if ( !this->mVboId ) {
		return false;
	}
	int active_vbo_id = 0;
	glGetIntegerv ( GL_ARRAY_BUFFER_BINDING , &active_vbo_id );
	return this->mVboId == active_vbo_id;
}

}
}
