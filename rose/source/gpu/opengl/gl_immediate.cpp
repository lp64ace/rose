#include "gl_immediate.h"

#include "gl_context.h"
#include "gl_batch.h"
#include "gl_vertex_array.h"

#include "gpu/intern/gpu_context_private.h"
#include "gpu/intern/gpu_shader_private.h"
#include "gpu/intern/gpu_vertex_format_private.h"

#include <gl/glew.h>

namespace rose {
namespace gpu {

GLImmediate::GLImmediate ( ) {
	glGenVertexArrays ( 1 , &mVaoId );
	glBindVertexArray ( mVaoId ); /* Necessary for glObjectLabel. */

	mBuffer.mBufferSize = DEFAULT_INTERNAL_BUFFER_SIZE;
	glGenBuffers ( 1 , &mBuffer.mVboId );
	glBindBuffer ( GL_ARRAY_BUFFER , mBuffer.mVboId );
	glBufferData ( GL_ARRAY_BUFFER , mBuffer.mBufferSize , nullptr , GL_DYNAMIC_DRAW );

	mBufferStrict.mBufferSize = DEFAULT_INTERNAL_BUFFER_SIZE;
	glGenBuffers ( 1 , &mBufferStrict.mVboId );
	glBindBuffer ( GL_ARRAY_BUFFER , mBufferStrict.mVboId );
	glBufferData ( GL_ARRAY_BUFFER , mBufferStrict.mBufferSize , nullptr , GL_DYNAMIC_DRAW );

	glBindBuffer ( GL_ARRAY_BUFFER , 0 );
	glBindVertexArray ( 0 );
}

GLImmediate::~GLImmediate ( ) {
	glDeleteVertexArrays ( 1 , &mVaoId );

	glDeleteBuffers ( 1 , &mBuffer.mVboId );
	glDeleteBuffers ( 1 , &mBufferStrict.mVboId );
}

unsigned char *GLImmediate::Begin ( ) {
	// How many bytes do we need for this draw call?
	const size_t bytes_needed = vertex_buffer_size ( &mVertexFormat , mVertexLen );
	// Does the current buffer have enough room?
	const size_t available_bytes = GetBufSize ( ) - GetBufOffset ( );

	glBindBuffer ( GL_ARRAY_BUFFER , GetVboId ( ) );

	bool recreate_buffer = false;
	if ( bytes_needed > GetBufSize ( ) ) {
		// expand the internal buffer
		GetBufSize ( ) = bytes_needed;
		recreate_buffer = true;
	} else if ( bytes_needed < DEFAULT_INTERNAL_BUFFER_SIZE &&
		    GetBufSize ( ) > DEFAULT_INTERNAL_BUFFER_SIZE ) {
		// shrink the internal buffer
		GetBufSize ( ) = DEFAULT_INTERNAL_BUFFER_SIZE;
		recreate_buffer = true;
	}

	// ensure vertex data is aligned 
	// Might waste a little space, but it's safe.
	const unsigned int pre_padding = _padding ( GetBufOffset ( ) , mVertexFormat.Stride );

	if ( !recreate_buffer && ( ( bytes_needed + pre_padding ) <= available_bytes ) ) {
		GetBufOffset ( ) += pre_padding;
	} else {
		/* orphan this buffer & start with a fresh one */
		glBufferData ( GL_ARRAY_BUFFER , GetBufSize ( ) , nullptr , GL_DYNAMIC_DRAW );
		GetBufOffset ( ) = 0;
	}

	{
		GLint bufsize;
		glGetBufferParameteriv ( GL_ARRAY_BUFFER , GL_BUFFER_SIZE , &bufsize );
		LIB_assert ( GetBufOffset ( ) + bytes_needed <= bufsize );
	}

	GLbitfield access = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
	if ( !mStrictVertexLen ) {
		access |= GL_MAP_FLUSH_EXPLICIT_BIT;
	}
	void *data = glMapBufferRange ( GL_ARRAY_BUFFER , GetBufOffset ( ) , bytes_needed , access );
	LIB_assert ( data != nullptr );

	this->mBytesMapped = bytes_needed;
	return ( unsigned char * ) data;
}

void GLImmediate::End ( ) {
	LIB_assert ( this->mPrimType != GPU_PRIM_NONE ); /* make sure we're between a Begin/End pair */

	unsigned int buffer_bytes_used = this->mBytesMapped;
	if ( !this->mStrictVertexLen ) {
		if ( this->mVertexIdx != this->mVertexLen ) {
			this->mVertexLen = this->mVertexIdx;
			buffer_bytes_used = vertex_buffer_size ( &this->mVertexFormat , this->mVertexLen );
			/* unused buffer bytes are available to the next immBegin */
		}
		/* tell OpenGL what range was modified so it doesn't copy the whole mapped range */
		glFlushMappedBufferRange ( GL_ARRAY_BUFFER , 0 , buffer_bytes_used );
	}
	glUnmapBuffer ( GL_ARRAY_BUFFER );

	if ( this->mVertexLen > 0 ) {
		GLContext::Get ( )->StateManager->ApplyState ( );

		/* We convert the offset in vertex offset from the buffer's start.
		 * This works because we added some padding to align the first vertex. */
		unsigned int v_first = GetBufOffset ( ) / this->mVertexFormat.Stride;
		GLVertArray::UpdateBindings ( this->mVaoId , v_first , &this->mVertexFormat , reinterpret_cast< Shader * >( this->mShader )->mInterface );

		/* Update matrices. */
		GPU_shader_bind ( this->mShader );

#ifdef __APPLE__
		glDisable ( GL_PRIMITIVE_RESTART );
#endif
		glDrawArrays ( prim_to_gl ( this->mPrimType ) , 0 , this->mVertexLen );
#ifdef __APPLE__
		glEnable ( GL_PRIMITIVE_RESTART );
#endif
		/* These lines are causing crash on startup on some old GPU + drivers.
		 * They are not required so just comment them. (T55722) */
		 // glBindBuffer(GL_ARRAY_BUFFER, 0);
		 // glBindVertexArray(0);
	}

	this->GetBufOffset ( ) += buffer_bytes_used;
}

}
}
