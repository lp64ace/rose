#include "gl_context.h"
#include "gl_backend.h"
#include "gl_batch.h"
#include "gl_immediate.h"

#include "ghost/ghost_api.h"

#include <gl/glew.h>

namespace rose {
namespace gpu {

GLContext::GLContext ( void *ghost_window , GLSharedOrphanLists &shared_orphan_list ) : mSharedOrphanList ( shared_orphan_list ) {
	float data [ 4 ] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glGenBuffers ( 1 , &this->mDefaultAttrVbo );
	glBindBuffer ( GL_ARRAY_BUFFER , this->mDefaultAttrVbo );
	glBufferData ( GL_ARRAY_BUFFER , sizeof ( data ) , data , GL_STATIC_DRAW );
	glBindBuffer ( GL_ARRAY_BUFFER , 0 );

	this->mGhostWindow = ghost_window;
	if ( this->mGhostWindow ) {
		ghostActivateWindowContext ( ( GHOST_WindowHandle ) this->mGhostWindow );
		GHOST_Rect rect = gostGetWindowClientBounds ( ( GHOST_WindowHandle ) this->mGhostWindow );
		int w = rect.right - rect.left;
		int h = rect.bottom - rect.top;

		GLuint default_fbo = 0;

		if ( default_fbo != 0 ) {
			/* Bind default framebuffer, otherwise state might be undefined because of
			 * detect_mip_render_workaround(). */
			glBindFramebuffer ( GL_FRAMEBUFFER , default_fbo );
			this->FrontLeft = new GLFrameBuffer ( "front_left" , this , GL_COLOR_ATTACHMENT0 , default_fbo , w , h );
			this->BackLeft = new GLFrameBuffer ( "back_left" , this , GL_COLOR_ATTACHMENT0 , default_fbo , w , h );
		} else {
			this->FrontLeft = new GLFrameBuffer ( "front_left" , this , GL_FRONT_LEFT , 0 , w , h );
			this->BackLeft = new GLFrameBuffer ( "back_left" , this , GL_BACK_LEFT , 0 , w , h );
		}

		GLboolean supports_stereo_quad_buffer = GL_FALSE;
		glGetBooleanv ( GL_STEREO , &supports_stereo_quad_buffer );
		if ( supports_stereo_quad_buffer ) {
			this->FrontRight = new GLFrameBuffer ( "front_right" , this , GL_FRONT_RIGHT , 0 , w , h );
			this->BackRight = new GLFrameBuffer ( "back_right" , this , GL_BACK_RIGHT , 0 , w , h );
		}
	} else {
		this->BackLeft = new GLFrameBuffer ( "back_left" , this , GL_NONE , 0 , 0 , 0 );
	}
	this->StateManager = new GLStateManager ( );
	this->Imm = new GLImmediate ( );

	this->ActiveFb = this->BackLeft;
	static_cast< GLStateManager * >( StateManager )->ActiveFb = static_cast< GLFrameBuffer * >( this->ActiveFb );
}

GLContext::~GLContext ( ) {
	for ( GLVaoCache *cache : this->mVaoCaches ) {
		cache->Clear ( );
	}
	glDeleteBuffers ( 1 , &this->mDefaultAttrVbo );
	delete this->StateManager;
}

void GLContext::Activate ( ) {
	if ( this->mGhostWindow ) {
		ghostActivateWindowContext ( ( GHOST_WindowHandle ) this->mGhostWindow );

		GHOST_Rect rect = gostGetWindowClientBounds ( ( GHOST_WindowHandle ) this->mGhostWindow );
		int w = rect.right - rect.left;
		int h = rect.bottom - rect.top;

		if ( ghostActivateWindowContext ( ( GHOST_WindowHandle ) this->mGhostWindow ) ) {
			glViewport ( 0 , 0 , w , h );
			this->OrphansClear ( );
		}

		if ( this->FrontLeft ) {
			this->FrontLeft->SetSize ( w , h );
		}
		if ( this->BackLeft ) {
			this->BackLeft->SetSize ( w , h );
		}
		if ( this->FrontRight ) {
			this->FrontRight->SetSize ( w , h );
		}
		if ( this->BackRight ) {
			this->BackRight->SetSize ( w , h );
		}
	}

	immActivate ( );
}

void GLContext::Deactivate ( ) {
	immDeactivate ( );

	if ( this->mGhostWindow ) {
		ghostReleaseWindowContext ( ( GHOST_WindowHandle ) this->mGhostWindow );
	}
}

void GLContext::Flush ( ) {
	glFlush ( );
}

void GLContext::Finish ( ) {
	glFinish ( );
}

GLContext *GLContext::Get ( ) {
	return ( GLContext * ) Context::Get ( );
}

void GLContext::GetMemoryStatistics ( int *total , int *free ) {
	if ( has_gl_extension ( "GL_NVX_gpu_memory_info" ) ) {
		/* Returned value in Kb. */
		glGetIntegerv ( GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX , total );
		glGetIntegerv ( GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX , free );
	} else if ( has_gl_extension ( "GL_ATI_meminfo" ) ) {
		int stats [ 4 ];
		glGetIntegerv ( GL_TEXTURE_FREE_MEMORY_ATI , stats );

		*total = 0;
		*free = stats [ 0 ]; // Total memory free in the pool.
	} else {
		*total = 0;
		*free = 0;
	}
}

void GLContext::OrphansAdd ( Vector<unsigned int> &orphan_list , std::mutex &list_mutex , unsigned int id ) {
	list_mutex.lock ( );
	orphan_list.Append ( id );
	list_mutex.unlock ( );
}

void GLSharedOrphanLists::OrphansClear ( ) {
	this->mListsMutex.lock ( );
	if ( !this->mBuffers.IsEmpty ( ) ) {
		glDeleteBuffers ( ( unsigned int ) this->mBuffers.Size ( ) , this->mBuffers.Data ( ) );
		this->mBuffers.Clear ( );
	}
	if ( !this->mTextures.IsEmpty ( ) ) {
		glDeleteTextures ( ( unsigned int ) this->mTextures.Size ( ) , this->mTextures.Data ( ) );
		this->mTextures.Clear ( );
	}
	this->mListsMutex.unlock ( );
}

void GLContext::OrphansClear ( ) {
	this->mListsMutex.lock ( );
	if ( !this->mOrphanedVertarrays.IsEmpty ( ) ) {
		glDeleteVertexArrays ( ( unsigned int ) this->mOrphanedVertarrays.Size ( ) , this->mOrphanedVertarrays.Data ( ) );
		this->mOrphanedVertarrays.Clear ( );
	}
	if ( !this->mOrphanedVertarrays.IsEmpty ( ) ) {
		glDeleteFramebuffers ( ( unsigned int ) this->mOrphanedVertarrays.Size ( ) , this->mOrphanedVertarrays.Data ( ) );
		this->mOrphanedVertarrays.Clear ( );
	}
	this->mListsMutex.unlock ( );

	this->mSharedOrphanList.OrphansClear ( );
}

void GLContext::VaoFree ( unsigned int vao_id ) {
	if ( this == GLContext::Get ( ) ) {
		glDeleteVertexArrays ( 1 , &vao_id );
	} else {
		OrphansAdd ( this->mOrphanedVertarrays , this->mListsMutex , vao_id );
	}
}

void GLContext::FboFree ( unsigned int fbo_id ) {
	if ( this == GLContext::Get ( ) ) {
		glDeleteFramebuffers ( 1 , &fbo_id );
	} else {
		OrphansAdd ( this->mOrphanedFramebuffers , this->mListsMutex , fbo_id );
	}
}

void GLContext::BufFree ( unsigned int buf_id ) {
	// Any context can free.
	if ( GLContext::Get ( ) ) {
		glDeleteBuffers ( 1 , &buf_id );
	} else {
		GLSharedOrphanLists *orphan_list = GLBackend::Get ( )->SharedOrphanListGet ( );
		OrphansAdd ( orphan_list->mBuffers , orphan_list->mListsMutex , buf_id );
	}
}

void GLContext::TexFree ( unsigned int tex_id ) {
	// Any context can free.
	if ( GLContext::Get ( ) ) {
		glDeleteTextures ( 1 , &tex_id );
	} else {
		GLSharedOrphanLists *orphan_list = GLBackend::Get ( )->SharedOrphanListGet ( );
		OrphansAdd ( orphan_list->mTextures , orphan_list->mListsMutex , tex_id );
	}
}

void GLContext::VaoCacheRegister ( GLVaoCache *cache ) {
	this->mListsMutex.lock ( );
	this->mVaoCaches.insert ( cache );
	this->mListsMutex.unlock ( );
}

void GLContext::VaoCacheUnregister ( GLVaoCache *cache ) {
	this->mListsMutex.lock ( );
	this->mVaoCaches.erase ( cache );
	this->mListsMutex.unlock ( );
}

}
}
