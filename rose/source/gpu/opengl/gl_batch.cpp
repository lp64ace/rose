#include "gl_batch.h"
#include "gl_shader.h"
#include "gl_shader_interface.h"
#include "gl_context.h"
#include "gl_vertex_array.h"
#include "gl_storage_buffer.h"

namespace rose {
namespace gpu {

GLVaoCache::GLVaoCache ( ) {
	this->Init ( );
}

GLVaoCache::~GLVaoCache ( ) {
	this->Clear ( );
}

void GLVaoCache::Init ( ) {
	this->mContext = nullptr;
	this->mInterface = nullptr;
	this->mIsDynamicVaoCount = false;
	for ( int i = 0; i < GPU_VAO_STATIC_LEN; i++ ) {
		this->mStaticVaos.Interfaces [ i ] = nullptr;
		this->mStaticVaos.VaoIds [ i ] = 0;
	}
	this->mVaoBaseInstance = 0;
	this->mBaseInstance = 0;
	this->mVaoId = 0;
}

void GLVaoCache::Insert ( const GLShaderInterface *interface , GLuint vao ) {
	/* Now insert the cache. */
	if ( !this->mIsDynamicVaoCount ) {
		int i; /* find first unused slot */
		for ( i = 0; i < GPU_VAO_STATIC_LEN; i++ ) {
			if ( this->mStaticVaos.VaoIds [ i ] == 0 ) {
				break;
			}
		}

		if ( i < GPU_VAO_STATIC_LEN ) {
			this->mStaticVaos.Interfaces [ i ] = interface;
			this->mStaticVaos.VaoIds [ i ] = vao;
		} else {
			/* Erase previous entries, they will be added back if drawn again. */
			for ( int i = 0; i < GPU_VAO_STATIC_LEN; i++ ) {
				if ( this->mStaticVaos.Interfaces [ i ] != nullptr ) {
					const_cast< GLShaderInterface * >( this->mStaticVaos.Interfaces [ i ] )->RefRemove ( this );
					this->mContext->VaoFree ( this->mStaticVaos.VaoIds [ i ] );
				}
			}
			/* Not enough place switch to dynamic. */
			this->mIsDynamicVaoCount = true;
			/* Init dynamic arrays and let the branch below set the values. */
			this->mDynamicVaos.Count = GPU_BATCH_VAO_DYN_ALLOC_COUNT;
			this->mDynamicVaos.Interfaces = ( const GLShaderInterface ** ) MEM_callocN ( this->mDynamicVaos.Count * sizeof ( GLShaderInterface * ) , "dyn vaos interfaces" );
			this->mDynamicVaos.VaoIds = ( GLuint * ) MEM_callocN ( this->mDynamicVaos.Count * sizeof ( GLuint ) , "dyn vaos ids" );
		}
	}

	if ( this->mIsDynamicVaoCount ) {
		int i; /* find first unused slot */
		for ( i = 0; i < this->mDynamicVaos.Count; i++ ) {
			if ( this->mDynamicVaos.VaoIds [ i ] == 0 ) {
				break;
			}
		}

		if ( i == this->mDynamicVaos.Count ) {
			/* Not enough place, realloc the array. */
			i = this->mDynamicVaos.Count;
			this->mDynamicVaos.Count += GPU_BATCH_VAO_DYN_ALLOC_COUNT;
			this->mDynamicVaos.Interfaces = ( const GLShaderInterface ** ) MEM_recallocN ( ( void * ) this->mDynamicVaos.Interfaces , sizeof ( GLShaderInterface * ) * this->mDynamicVaos.Count );
			this->mDynamicVaos.VaoIds = ( GLuint * ) MEM_recallocN ( this->mDynamicVaos.VaoIds , sizeof ( GLuint ) * this->mDynamicVaos.Count );
		}
		this->mDynamicVaos.Interfaces [ i ] = interface;
		this->mDynamicVaos.VaoIds [ i ] = vao;
	}

	const_cast< GLShaderInterface * >( interface )->RefAdd ( this );
}

void GLVaoCache::Remove ( const GLShaderInterface *interface ) {
	const int count = ( this->mIsDynamicVaoCount ) ? this->mDynamicVaos.Count : GPU_VAO_STATIC_LEN;
	GLuint *vaos = ( this->mIsDynamicVaoCount ) ? this->mDynamicVaos.VaoIds : this->mStaticVaos.VaoIds;
	const GLShaderInterface **interfaces = ( this->mIsDynamicVaoCount ) ? this->mDynamicVaos.Interfaces : this->mStaticVaos.Interfaces;
	for ( int i = 0; i < count; i++ ) {
		if ( interfaces [ i ] == interface ) {
			this->mContext->VaoFree ( vaos [ i ] );
			vaos [ i ] = 0;
			interfaces [ i ] = nullptr;
			break; // cannot have duplicates.
		}
	}
}

void GLVaoCache::Clear ( ) {
	GLContext *ctx = GLContext::Get ( );
	const int count = ( this->mIsDynamicVaoCount ) ? this->mDynamicVaos.Count : GPU_VAO_STATIC_LEN;
	GLuint *vaos = ( this->mIsDynamicVaoCount ) ? this->mDynamicVaos.VaoIds : this->mStaticVaos.VaoIds;
	const GLShaderInterface **interfaces = ( this->mIsDynamicVaoCount ) ? this->mDynamicVaos.Interfaces : this->mStaticVaos.Interfaces;
	/* Early out, nothing to free. */
	if ( this->mContext == nullptr ) {
		return;
	}

	if ( this->mContext == ctx ) {
		glDeleteVertexArrays ( count , vaos );
		glDeleteVertexArrays ( 1 , &this->mVaoBaseInstance );
	} else {
		/* TODO(fclem): Slow way. Could avoid multiple mutex lock here */
		for ( int i = 0; i < count; i++ ) {
			this->mContext->VaoFree ( vaos [ i ] );
		}
		this->mContext->VaoFree ( this->mVaoBaseInstance );
	}

	for ( int i = 0; i < count; i++ ) {
		if ( interfaces [ i ] != nullptr ) {
			const_cast< GLShaderInterface * >( interfaces [ i ] )->RefRemove ( this );
		}
	}

	if ( this->mIsDynamicVaoCount ) {
		MEM_freeN ( ( void * ) this->mDynamicVaos.Interfaces );
		MEM_freeN ( this->mDynamicVaos.VaoIds );
	}

	if ( this->mContext ) {
		this->mContext->VaoCacheUnregister ( this );
	}
	/* Reinit. */
	this->Init ( );
}

GLuint GLVaoCache::Lookup ( const GLShaderInterface *interface ) {
	const int count = ( this->mIsDynamicVaoCount ) ? this->mDynamicVaos.Count : GPU_VAO_STATIC_LEN;
	const GLShaderInterface **interfaces = ( this->mIsDynamicVaoCount ) ? this->mDynamicVaos.Interfaces : this->mStaticVaos.Interfaces;
	for ( int i = 0; i < count; i++ ) {
		if ( interfaces [ i ] == interface ) {
			return ( this->mIsDynamicVaoCount ) ? this->mDynamicVaos.VaoIds [ i ] : this->mStaticVaos.VaoIds [ i ];
		}
	}
	return 0;
}

void GLVaoCache::ContextCheck ( ) {
	GLContext *ctx = GLContext::Get ( );
	assert ( ctx );

	if ( this->mContext != ctx ) {
		if ( this->mContext != nullptr ) {
			/* IMPORTANT: Trying to draw a batch in multiple different context will trash the VAO cache.
			 * This has major performance impact and should be avoided in most cases. */
			this->mContext->VaoCacheUnregister ( this );
		}
		this->Clear ( );
		this->mContext = ctx;
		this->mContext->VaoCacheRegister ( this );
	}
}

unsigned int GLVaoCache::BaseInstanceVaoGet ( GPU_Batch *batch , int i_first ) {
	this->ContextCheck ( );
	/* Make sure the interface is up to date. */
	Shader *shader = GLContext::Get ( )->Shader;
	GLShaderInterface *interface = static_cast< GLShaderInterface * >( shader->mInterface );
	if ( this->mInterface != interface ) {
		VaoGet ( batch );
		/* Trigger update. */
		this->mBaseInstance = 0;
	}
	/**
	 * There seems to be a nasty bug when drawing using the same VAO reconfiguring (T71147).
	 * We just use a throwaway VAO for that. Note that this is likely to degrade performance.
	 */
#ifdef __APPLE__
	glDeleteVertexArrays ( 1 , &this->mVaoBaseInstance );
	this->mVaoBaseInstance = 0;
	this->mBaseInstance = 0;
#endif

	if ( this->mVaoBaseInstance == 0 ) {
		glGenVertexArrays ( 1 , &this->mVaoBaseInstance );
	}

	if ( this->mBaseInstance != i_first ) {
		this->mBaseInstance = i_first;
		GLVertArray::UpdateBindings ( this->mVaoBaseInstance , batch , this->mInterface , i_first );
	}
	return mVaoBaseInstance;
}

unsigned int GLVaoCache::VaoGet ( GPU_Batch *batch ) {
	this->ContextCheck ( );

	Shader *shader = GLContext::Get ( )->Shader;
	GLShaderInterface *interface = static_cast< GLShaderInterface * >( shader->mInterface );
	if ( this->mInterface != interface ) {
		this->mInterface = interface;
		this->mVaoId = this->Lookup ( this->mInterface );

		if ( this->mVaoId == 0 ) {
			/* Cache miss, create a new VAO. */
			glGenVertexArrays ( 1 , &this->mVaoId );
			this->Insert ( this->mInterface , this->mVaoId );
			GLVertArray::UpdateBindings ( this->mVaoId , batch , this->mInterface , 0 );
		}
	}

	return this->mVaoId;
}

/* -------------------------------------------------------------------- */
/** \name Drawing */

void GLBatch::Bind ( int i_first ) {
	GLContext::Get ( )->StateManager->ApplyState ( );

	if ( this->Flag & GPU_BATCH_DIRTY ) {
		this->Flag &= ~GPU_BATCH_DIRTY;
		this->mVaoCache.Clear ( );
	}

#if GPU_TRACK_INDEX_RANGE
	/* Can be removed if GL 4.3 is required. */
	if ( !GLContext::FixedRestartIndexSupport && ( this->Elem != nullptr ) ) {
		glPrimitiveRestartIndex ( this->GetElem ( )->restart_index ( ) );
	}
#endif

	/* Can be removed if GL 4.2 is required. */
	if ( !GLContext::BaseInstanceSupport && ( i_first > 0 ) ) {
		glBindVertexArray ( this->mVaoCache.BaseInstanceVaoGet ( this , i_first ) );
	} else {
		glBindVertexArray ( this->mVaoCache.VaoGet ( this ) );
	}
}

void GLBatch::Draw ( int v_first , int v_count , int i_first , int i_count ) {
	this->Bind ( i_first );

	assert ( v_count > 0 && i_count > 0 );

	GLenum gl_type = prim_to_gl ( this->PrimType );

	printf ( "batch draw %d vertices %d indices using shader %s.\n" ,
		 v_count ,
		 i_count ,
		 GPU_shader_get_name ( this->Shader ) );

	if ( this->Elem ) {
		const GLIndexBuf *el = this->GetElem ( );
		GLenum index_type = index_buf_type_to_gl ( el->mIndexType );
		GLint base_index = el->mIndexBase;
		void *v_first_ofs = el->offset_ptr ( v_first );

		if ( GLContext::BaseInstanceSupport ) {
			glDrawElementsInstancedBaseVertexBaseInstance (
				gl_type , v_count , index_type , v_first_ofs , i_count , base_index , i_first );
		} else {
			glDrawElementsInstancedBaseVertex (
				gl_type , v_count , index_type , v_first_ofs , i_count , base_index );
		}
	} else {
#ifdef __APPLE__
		glDisable ( GL_PRIMITIVE_RESTART );
#endif
		if ( GLContext::BaseInstanceSupport ) {
			glDrawArraysInstancedBaseInstance ( gl_type , v_first , v_count , i_count , i_first );
		} else {
			glDrawArraysInstanced ( gl_type , v_first , v_count , i_count );
		}
#ifdef __APPLE__
		glEnable ( GL_PRIMITIVE_RESTART );
#endif
	}
}

void GLBatch::DrawIndirect ( GPU_StorageBuf *indirect_buf , intptr_t offset ) {
	this->Bind ( 0 );

	/* TODO(fclem): Make the barrier and binding optional if consecutive draws are issued. */
	dynamic_cast< GLStorageBuf * >( unwrap ( indirect_buf ) )->BindAs ( GL_DRAW_INDIRECT_BUFFER );
	/* This barrier needs to be here as it only work on the currently bound indirect buffer. */
	glMemoryBarrier ( GL_COMMAND_BARRIER_BIT );

	GLenum gl_type = prim_to_gl ( this->PrimType );
	if ( this->Elem ) {
		const GLIndexBuf *el = this->GetElem ( );
		GLenum index_type = index_buf_type_to_gl ( el->mIndexType );
		glDrawElementsIndirect ( gl_type , index_type , ( GLvoid * ) offset );
	} else {
		glDrawArraysIndirect ( gl_type , ( GLvoid * ) offset );
	}
	// Unbind.
	glBindBuffer ( GL_DRAW_INDIRECT_BUFFER , 0 );
}

void GLBatch::MultiDrawIndirect ( GPU_StorageBuf *indirect_buf , int count , intptr_t offset , intptr_t stride ) {
	this->Bind ( 0 );

	/* TODO(fclem): Make the barrier and binding optional if consecutive draws are issued. */
	dynamic_cast< GLStorageBuf * >( unwrap ( indirect_buf ) )->BindAs ( GL_DRAW_INDIRECT_BUFFER );
	/* This barrier needs to be here as it only work on the currently bound indirect buffer. */
	glMemoryBarrier ( GL_COMMAND_BARRIER_BIT );

	GLenum gl_type = prim_to_gl ( this->PrimType );
	if ( this->Elem ) {
		const GLIndexBuf *el = this->GetElem ( );
		GLenum index_type = index_buf_type_to_gl ( el->mIndexType );
		glMultiDrawElementsIndirect ( gl_type , index_type , ( GLvoid * ) offset , count , stride );
	} else {
		glMultiDrawArraysIndirect ( gl_type , ( GLvoid * ) offset , count , stride );
	}
	// Unbind.
	glBindBuffer ( GL_DRAW_INDIRECT_BUFFER , 0 );
}

}
}
