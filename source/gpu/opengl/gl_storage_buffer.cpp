#include "gl_storage_buffer.h"
#include "gl_vertex_buffer.h"
#include "gl_texture.h"
#include "gl_context.h"

namespace rose {
namespace gpu {

GLStorageBuf::GLStorageBuf ( size_t size , int usage , const char *name ) : StorageBuf ( size , name ) {
	this->mUsage = usage;
	/* Do not create UBO GL buffer here to allow allocation from any thread. */
	assert ( size <= GLContext::MaxSSBOSize );
}

GLStorageBuf::~GLStorageBuf ( ) {
	GLContext::BufFree ( this->mSSBOId );
}

void GLStorageBuf::Init ( ) {
	assert ( GLContext::Get ( ) );

	glGenBuffers ( 1 , &this->mSSBOId );
	glBindBuffer ( GL_SHADER_STORAGE_BUFFER , this->mSSBOId );
	glBufferData ( GL_SHADER_STORAGE_BUFFER , this->mSizeInBytes , nullptr , usage_to_gl ( this->mUsage ) );
}

void GLStorageBuf::Update ( const void *data ) {
	if ( this->mSSBOId == 0 ) {
		this->Init ( );
	}
	glBindBuffer ( GL_SHADER_STORAGE_BUFFER , this->mSSBOId );
	glBufferSubData ( GL_SHADER_STORAGE_BUFFER , 0 , this->mSizeInBytes , data );
	glBindBuffer ( GL_SHADER_STORAGE_BUFFER , 0 );
}

void GLStorageBuf::Bind ( int slot ) {
	if ( slot >= GLContext::MaxSSBOBinds ) {
		fprintf (
			stderr ,
			"Error: Trying to bind \"%s\" ssbo to slot %d which is above the reported limit of %d.\n" ,
			this->mName ,
			slot ,
			GLContext::MaxSSBOBinds );
		return;
	}

	if ( this->mSSBOId == 0 ) {
		this->Init ( );
	}

	if ( this->mData != nullptr ) {
		this->Update ( this->mData );
		MEM_SAFE_FREE ( this->mData );
	}

	this->mSlot = slot;
	glBindBufferBase ( GL_SHADER_STORAGE_BUFFER , this->mSlot , this->mSSBOId );
}

void GLStorageBuf::BindAs ( GLenum target ) {
	if ( this->mSSBOId != 0 ) {
		fprintf ( stderr , "Trying to use storage buf as indirect buffer but buffer was never filled." );
	}
	glBindBuffer ( target , this->mSSBOId );
}

void GLStorageBuf::Unbind ( ) {
#ifdef _DEBUG
	/* NOTE: This only unbinds the last bound slot. */
	glBindBufferBase ( GL_SHADER_STORAGE_BUFFER , this->mSlot , 0 );
	/* Hope that the context did not change. */
#endif
	this->mSlot = -1;
}

void GLStorageBuf::Clear ( int internal_format , unsigned int data_format , void *data ) {
	if ( this->mSSBOId == 0 ) {
		this->Init ( );
	}

	if ( GLContext::DirectStateAccessSupport ) {
		glClearNamedBufferData ( this->mSSBOId ,
					 format_to_gl_internal_format ( internal_format ) ,
					 format_to_gl_data_format ( internal_format ) ,
					 data_format_to_gl ( data_format ) ,
					 data );
	} else {
		/* WATCH(@fclem): This should be ok since we only use clear outside of drawing functions. */
		glBindBuffer ( GL_SHADER_STORAGE_BUFFER , this->mSSBOId );
		glClearBufferData ( GL_SHADER_STORAGE_BUFFER ,
				    format_to_gl_internal_format ( internal_format ) ,
				    format_to_gl_data_format ( internal_format ) ,
				    data_format_to_gl ( data_format ) ,
				    data );
		glBindBuffer ( GL_SHADER_STORAGE_BUFFER , 0 );
	}
}

void GLStorageBuf::CopySub ( VertBuf *src_ , unsigned int dst_offset , unsigned int src_offset , unsigned int copy_size ) {
	GLVertBuf *src = static_cast< GLVertBuf * >( src_ );
	GLStorageBuf *dst = this;

	if ( dst->mSSBOId == 0 ) {
		dst->Init ( );
	}
	if ( src->mVboId == 0 ) {
		src->Bind ( );
	}

	if ( GLContext::DirectStateAccessSupport ) {
		glCopyNamedBufferSubData ( src->mVboId , dst->mSSBOId , src_offset , dst_offset , copy_size );
	} else {
		/* This binds the buffer to GL_ARRAY_BUFFER and upload the data if any. */
		src->Bind ( );
		glBindBuffer ( GL_COPY_WRITE_BUFFER , dst->mSSBOId );
		glCopyBufferSubData ( GL_ARRAY_BUFFER , GL_COPY_WRITE_BUFFER , src_offset , dst_offset , copy_size );
		glBindBuffer ( GL_COPY_WRITE_BUFFER , 0 );
	}
}

void GLStorageBuf::Read ( void *data ) {
	if ( this->mSSBOId == 0 ) {
		this->Init ( );
	}

	if ( GLContext::DirectStateAccessSupport ) {
		glGetNamedBufferSubData ( this->mSSBOId , 0 , this->mSizeInBytes , data );
	} else {
		/* This binds the buffer to GL_ARRAY_BUFFER and upload the data if any. */
		glBindBuffer ( GL_SHADER_STORAGE_BUFFER , this->mSSBOId );
		glGetBufferSubData ( GL_SHADER_STORAGE_BUFFER , 0 , this->mSizeInBytes , data );
		glBindBuffer ( GL_SHADER_STORAGE_BUFFER , 0 );
	}
}

}
}
