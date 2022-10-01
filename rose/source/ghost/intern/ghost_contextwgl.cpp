#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/glew.h>
#include <gl/wglew.h>

#include "ghost_contextwgl.h"

#include <stdio.h>

HGLRC GHOST_ContextWGL::sSharedContext = NULL;
ULONG GHOST_ContextWGL::sSharedCount = 0;

GHOST_ContextWGL::GHOST_ContextWGL ( HWND hWnd , HDC hDC ) {
	this->mHandle = NULL;
	this->mWindowHandle = hWnd;
	this->mDeviceHandle = hDC;
}

GHOST_ContextWGL::~GHOST_ContextWGL ( ) {
	if ( this->mHandle != NULL ) {
		if ( this->mHandle == ::wglGetCurrentContext ( ) ) {
			::wglMakeCurrent ( NULL , NULL );
		}
		if ( this->mHandle != sSharedContext || sSharedCount == 1 ) {
			sSharedCount--;
			if ( sSharedCount == 0 ) {
				sSharedContext = NULL;
			}
			::wglDeleteContext ( this->mHandle );
		}
	}
}

GHOST_TStatus GHOST_ContextWGL::ActivateDrawingContext ( ) {
	if ( ::wglMakeCurrent ( this->mDeviceHandle , this->mHandle ) ) {
		return GHOST_kSuccess;
	}
	return GHOST_kFailure;
}

GHOST_TStatus GHOST_ContextWGL::ReleaseDrawingContext ( ) {
	if ( ::wglMakeCurrent ( NULL , NULL ) ) {
		return GHOST_kSuccess;
	}
	return GHOST_kFailure;
}

GHOST_TStatus GHOST_ContextWGL::InitDrawingContext ( ) {
	HGLRC prevHGLRC = ::wglGetCurrentContext ( );
	HDC prevHDC = ::wglGetCurrentDC ( );

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof ( PIXELFORMATDESCRIPTOR ),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_STEREO | PFD_SUPPORT_COMPOSITION,
		PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
		32,                   // Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,                   // Number of bits for the depthbuffer
		8,                    // Number of bits for the stencilbuffer
		0,                    // Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	int iPixelFormat = ::ChoosePixelFormat ( this->mDeviceHandle , &pfd );
	if ( iPixelFormat != 0 ) {
		if ( !::SetPixelFormat ( this->mDeviceHandle , iPixelFormat , &pfd ) ) {
			fprintf ( stderr , "Failed to select pixel format.\n" );
			::wglMakeCurrent ( prevHDC , prevHGLRC );
			return GHOST_kFailure;
		}
	} else {
		fprintf ( stderr , "Failed to select pixel format.\n" );
		::wglMakeCurrent ( prevHDC , prevHGLRC );
		return GHOST_kFailure;
	}

	this->mHandle = wglCreateContext ( this->mDeviceHandle );
	if ( this->mHandle ) {
		::wglMakeCurrent ( this->mDeviceHandle , this->mHandle );
	} else {
		::wglMakeCurrent ( prevHDC , prevHGLRC );
		return GHOST_kFailure;
	}

	if ( !::wglMakeCurrent ( this->mDeviceHandle , this->mHandle ) ) {
		::wglDeleteContext ( this->mHandle );
		::wglMakeCurrent ( prevHDC , prevHGLRC );
		return GHOST_kFailure;
	}

	GLenum err = glewInit ( );
	if ( GLEW_OK != err ) {
		fprintf ( stderr , "Error: %s\n" , glewGetErrorString ( err ) );
		::wglDeleteContext ( this->mHandle );
		::wglMakeCurrent ( prevHDC , prevHGLRC );
		return GHOST_kFailure;
	}
	fprintf ( stdout , "Using GLEW %s\n" , glewGetString ( GLEW_VERSION ) );

	GHOST_ContextWGL::sSharedCount++;

	if ( GHOST_ContextWGL::sSharedContext == NULL ) {
		GHOST_ContextWGL::sSharedContext = this->mHandle;
	} else if ( !::wglShareLists ( GHOST_ContextWGL::sSharedContext , this->mHandle ) ) {
		::wglDeleteContext ( this->mHandle );
		::wglMakeCurrent ( prevHDC , prevHGLRC );
		return GHOST_kFailure;
	}

	return GHOST_kSuccess;
}

GHOST_TStatus GHOST_ContextWGL::SwapBuffers ( ) {
	return ( ::SwapBuffers ( this->mDeviceHandle ) ) ? GHOST_kSuccess : GHOST_kFailure;
}

GHOST_TStatus GHOST_ContextWGL::ReleaseNativeHandles ( ) {
	GHOST_TStatus success = this->mHandle != GHOST_ContextWGL::sSharedContext ||
		GHOST_ContextWGL::sSharedCount == 1 ? GHOST_kSuccess : GHOST_kFailure;
	this->mWindowHandle = NULL;
	this->mDeviceHandle = NULL;
	return success;
}

GHOST_TStatus GHOST_ContextWGL::GetSwapInterval ( int *interval ) const {
	if ( WGLEW_EXT_swap_control ) {
		*interval = ::wglGetSwapIntervalEXT ( );
		return GHOST_kSuccess;
	}
	return GHOST_kFailure;
}

GHOST_TStatus GHOST_ContextWGL::SetSwapInterval ( int interval ) const {
	if ( WGLEW_EXT_swap_control ) {
		return ( ( ::wglSwapIntervalEXT ( interval ) ) == TRUE ) ? GHOST_kSuccess : GHOST_kFailure;
	}
	return GHOST_kFailure;
}
