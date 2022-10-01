#include "ghost_windowwin32.h"

#include "ghost_contextwgl.h"

#include <utf/utfconv.h>

GHOST_WindowWin32::GHOST_WindowWin32 ( GHOST_WindowWin32 *parent , const char *title , int32_t left , int32_t top , uint32_t width , uint32_t height ) {
	DWORD ExtendedStyle = ( parent ) ? 0 : WS_EX_APPWINDOW;
	DWORD Style = ( parent ) ? WS_POPUPWINDOW | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SIZEBOX : WS_OVERLAPPEDWINDOW;

	left = ( left == GHOST_kUseDefault ) ? CW_USEDEFAULT : left;
	top = ( top == GHOST_kUseDefault ) ? CW_USEDEFAULT : top;
	width = ( width == GHOST_kUseDefault ) ? CW_USEDEFAULT : width;
	height = ( height == GHOST_kUseDefault ) ? CW_USEDEFAULT : height;

	UTF16_ENCODE ( title );
	this->mHandle = CreateWindowExW (
		ExtendedStyle , L"GHOST_WindowClass" , title_16 , Style , left , top , width , height ,
		( parent ) ? ( HWND ) parent->GetOSWindow ( ) : HWND_DESKTOP , NULL , GetModuleHandle ( NULL ) , NULL
	);
	UTF16_UN_ENCODE ( title );

	/* Store a pointer to this class in the window structure. */
	::SetWindowLongPtr ( this->mHandle , GWLP_USERDATA , ( LONG_PTR ) this );

	if ( this->mHandle ) {
		::ShowWindow ( this->mHandle , SW_NORMAL );
		::UpdateWindow ( this->mHandle );
		this->mDC = GetDC ( this->mHandle );
	}
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
