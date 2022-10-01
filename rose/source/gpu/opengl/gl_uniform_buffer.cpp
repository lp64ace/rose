#include "gl_uniform_buffer.h"

#include "gl_context.h"

namespace rose {
namespace gpu {

GLUniformBuf::GLUniformBuf ( size_t size , const char *name ) : UniformBuf ( size , name ) {
	/* Do not create ubo GL buffer here to allow allocation from any thread. */
	assert ( size <= GLContext::MaxUboSize );
}

GLUniformBuf::~GLUniformBuf ( ) {
	GLContext::BufFree ( this->mUboId );
}

void GLUniformBuf::Init ( ) {
	assert ( GLContext::Get ( ) );

	glGenBuffers ( 1 , &this->mUboId );
	glBindBuffer ( GL_UNIFORM_BUFFER , this->mUboId );
	glBufferData ( GL_UNIFORM_BUFFER , this->mSizeInBytes , nullptr , GL_DYNAMIC_DRAW );
}

void GLUniformBuf::Update ( const void *data ) {
	if ( this->mUboId == 0 ) {
		this->Init ( );
	}
	glBindBuffer ( GL_UNIFORM_BUFFER , this->mUboId );
	glBufferSubData ( GL_UNIFORM_BUFFER , 0 , this->mSizeInBytes , data );
	glBindBuffer ( GL_UNIFORM_BUFFER , 0 );
}

void GLUniformBuf::Bind ( int slot ) {
	if ( slot >= GLContext::MaxUboBinds ) {
		fprintf ( stderr , "Error: Trying to bind \"%s\" ubo to slot %d which is above the reported limit of %d.\n" ,
			  this->mName , slot , GLContext::MaxUboBinds );
		return;
	}

	if ( this->mUboId == 0 ) {
		this->Init ( );
	}

	if ( this->mData != nullptr ) {
		this->Update ( this->mData );
		MEM_SAFE_FREE ( this->mData );
	}

	this->mSlot = slot;
	glBindBufferBase ( GL_UNIFORM_BUFFER , this->mSlot , this->mUboId );
}

void GLUniformBuf::Unbind ( ) {
	this->mSlot = -1;
}

}
}
