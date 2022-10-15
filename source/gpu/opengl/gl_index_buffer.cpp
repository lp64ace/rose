#include "gl_index_buffer.h"
#include "gl_context.h"

namespace rose {
namespace gpu {

GLIndexBuf::~GLIndexBuf ( ) {
	GLContext::BufFree ( this->mIboId );
}

void GLIndexBuf::Bind ( ) {
	if ( this->mIsSubrange ) {
		static_cast< GLIndexBuf * >( this->mSource )->Bind ( );
		return;
	}

	const bool allocate_on_device = this->mIboId == 0;
	if ( allocate_on_device ) {
		glGenBuffers ( 1 , &this->mIboId );
	}

	glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER , this->mIboId );

	if ( this->mData != nullptr || allocate_on_device ) {
		size_t size = this->GetSize ( );
		/* Sends data to GPU. */
		glBufferData ( GL_ELEMENT_ARRAY_BUFFER , size , this->mData , GL_STATIC_DRAW );
		/* No need to keep copy of data in system memory. */
		MEM_SAFE_FREE ( this->mData );
	}
}
void GLIndexBuf::BindAsSSBO ( unsigned int binding ) {
	this->Bind ( );
	assert ( this->mIboId != 0 );
	glBindBufferBase ( GL_SHADER_STORAGE_BUFFER , binding , this->mIboId );
}

void GLIndexBuf::UploadData ( ) {
	this->Bind ( );
}

const unsigned int *GLIndexBuf::Read ( ) const {
	assert ( IsActive ( ) );
	void *data = glMapBuffer ( GL_ELEMENT_ARRAY_BUFFER , GL_READ_ONLY );
	uint32_t *result = static_cast< uint32_t * >( data );
	return result;
}

void GLIndexBuf::UpdateSub ( unsigned int start , unsigned int len , const void *data ) {
	glBufferSubData ( GL_ELEMENT_ARRAY_BUFFER , start , len , data );
}

bool GLIndexBuf::IsActive ( ) const {
	if ( !this->mIboId ) {
		return false;
	}
	int active_ibo_id = 0;
	glGetIntegerv ( GL_ELEMENT_ARRAY_BUFFER_BINDING , &active_ibo_id );
	return this->mIboId == active_ibo_id;
}

}
}
