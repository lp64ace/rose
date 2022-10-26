#include "ghost_windowwin32.h"

#include "ghost_contextwgl.h"

#include <sstream>
#include <string>
#include <cmath>

GHOST_WindowWin32::GHOST_WindowWin32 ( GHOST_WindowWin32 *parent , const char *title , int32_t left , int32_t top , uint32_t width , uint32_t height ) : mDC ( NULL ) {
	DWORD ExtendedStyle = ( parent ) ? 0 : WS_EX_APPWINDOW;
	DWORD Style = ( parent ) ? WS_POPUPWINDOW | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SIZEBOX : WS_OVERLAPPEDWINDOW;

	left = ( left == GHOST_kUseDefault ) ? CW_USEDEFAULT : left;
	top = ( top == GHOST_kUseDefault ) ? CW_USEDEFAULT : top;
	width = ( width == GHOST_kUseDefault ) ? CW_USEDEFAULT : width;
	height = ( height == GHOST_kUseDefault ) ? CW_USEDEFAULT : height;

	size_t len = strlen ( title ) + 1;
	wchar_t *wtitle = ( wchar_t * ) LocalAlloc ( LMEM_ZEROINIT , len * sizeof ( wchar_t ) );
	MultiByteToWideChar ( 0 , 0 , title , len , wtitle , len );

	this->mHandle = CreateWindowExW (
		ExtendedStyle , L"GHOST_WindowClass" , wtitle , Style , left , top , width , height ,
		( parent ) ? ( HWND ) parent->GetOSWindow ( ) : HWND_DESKTOP , NULL , GetModuleHandle ( NULL ) , NULL
	);

	LocalFree ( wtitle );

	/* Store a pointer to this class in the window structure. */
	::SetWindowLongPtr ( this->mHandle , GWLP_USERDATA , ( LONG_PTR ) this );

	if ( this->mHandle ) {
		::ShowWindow ( this->mHandle , SW_NORMAL );
		::UpdateWindow ( this->mHandle );
		this->mDC = GetDC ( this->mHandle );
	}

	this->SetDrawingContextType ( GHOST_kDrawingContextTypeOpenGL );
	this->SetSwapInterval ( 0 );
}

GHOST_WindowWin32::~GHOST_WindowWin32 ( ) {
	if ( this->mHandle ) {
		::SetWindowLongPtr ( this->mHandle , GWLP_USERDATA , ( LONG_PTR ) nullptr );
		DestroyWindow ( this->mHandle );
		this->mHandle = NULL;
	}
}

GHOST_Context *GHOST_WindowWin32::NewDrawingContext ( GHOST_TDrawingContextType type ) {
	GHOST_Context *context = nullptr;
	if ( type == GHOST_kDrawingContextTypeOpenGL ) {
		context = new GHOST_ContextWGL ( this->mHandle , this->mDC );
		if ( context->InitDrawingContext ( ) ) {
			return context;
		} else {
			delete context; context = nullptr;
		}
	}
	return context;
}

bool GHOST_WindowWin32::GetValid ( ) const {
	return this->mHandle && this->mDC;
}

void *GHOST_WindowWin32::GetOSWindow ( ) const {
	return this->mHandle;
}

void GHOST_WindowWin32::SetTitle ( const char *title ) {
	::SetWindowTextA ( this->mHandle , title );
}

std::string GHOST_WindowWin32::GetTitle ( ) const {
	std::string result;
	result.resize ( ::GetWindowTextLengthA ( this->mHandle ) );
	::GetWindowTextA ( this->mHandle , &result [ 0 ] , result.size ( ) + 1 );
	return result;
}

GHOST_Rect GHOST_WindowWin32::GetWindowBounds ( ) const {
	RECT rect;
	GetWindowRect ( this->mHandle , &rect );
	return { rect.left,rect.top,rect.right,rect.bottom };
}

GHOST_Rect GHOST_WindowWin32::GetClientBounds ( ) const {
	RECT rect;
	GetClientRect ( this->mHandle , &rect );
	return { rect.left,rect.top,rect.right,rect.bottom };
}

void GHOST_WindowWin32::ScreenToClient ( int *p_x , int *p_y ) const {
	POINT pt = { ( p_x ) ? *p_x : 0 , ( p_y ) ? *p_y : 0 };
	if ( ::ScreenToClient ( this->mHandle , &pt ) ) {
		if ( p_x ) *p_x = pt.x;
		if ( p_y ) *p_y = pt.y;
	}
}

void GHOST_WindowWin32::ClientToScreen ( int *p_x , int *p_y ) const {
	POINT pt = { ( p_x ) ? *p_x : 0 , ( p_y ) ? *p_y : 0 };
	if ( ::ClientToScreen ( this->mHandle , &pt ) ) {
		if ( p_x ) *p_x = pt.x;
		if ( p_y ) *p_y = pt.y;
	}
}
