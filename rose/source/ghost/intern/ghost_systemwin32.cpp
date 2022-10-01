#include "ghost_systemwin32.h"
#include "ghost_windowwin32.h"

#include "ghost_windowmanager.h"

#include <CommCtrl.h>
#include <windows.h>

uint8_t GHOST_SystemWin32::GetNumDisplays ( ) const {
	return GetSystemMetrics ( SM_CMONITORS );
}

void GHOST_SystemWin32::GetMainDisplayDimensions ( uint32_t *width , uint32_t *height ) const {
	if ( width ) { *width = ::GetSystemMetrics ( SM_CXSCREEN ); }
	if ( height ) { *height = ::GetSystemMetrics ( SM_CYSCREEN ); }
}

void GHOST_SystemWin32::GetAllDisplayDimensions ( uint32_t *width , uint32_t *height ) const {
	if ( width ) { *width = ::GetSystemMetrics ( SM_CXVIRTUALSCREEN ); }
	if ( height ) { *height = ::GetSystemMetrics ( SM_CYVIRTUALSCREEN ); }
}

GHOST_TStatus GHOST_SystemWin32::Init ( ) {
	GHOST_TStatus success = GHOST_System::Init ( );
	InitCommonControls ( );
	if ( success ) {
		WNDCLASSW wc;
		ZeroMemory ( &wc , sizeof ( wc ) );
		wc.cbClsExtra = sizeof ( wc );
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.lpfnWndProc = GHOST_SystemWin32::WndProc;
		wc.hInstance = ::GetModuleHandle ( NULL );
		wc.hIcon = ::LoadIcon ( wc.hInstance , L"APPICON" );

		if ( !wc.hIcon ) {
			::LoadIconW ( NULL , IDI_APPLICATION );
		}

		wc.hCursor = ::LoadCursor ( NULL , IDC_ARROW );
		wc.hbrBackground = ( HBRUSH ) COLOR_BACKGROUND;
		wc.lpszMenuName = 0;
		wc.lpszClassName = L"GHOST_WindowClass";

		if ( ::RegisterClassW ( &wc ) == 0 ) {
			success = GHOST_kFailure;
		}
	}
	this->mWindowManager = new GHOST_WindowManager ( );
	return success;
}

GHOST_TStatus GHOST_SystemWin32::Exit ( ) {
	return GHOST_System::Exit ( );
}

GHOST_IntWindow *GHOST_SystemWin32::SpawnWindow ( const char *title , int32_t left , int32_t top , int32_t width , int32_t height , const GHOST_IntWindow *parent ) {
	GHOST_WindowWin32 *window = new GHOST_WindowWin32 ( ( GHOST_WindowWin32 * ) parent , title , left , top , width , height );
	this->mWindowManager->AddWindow ( window );
	return window;
}

bool GHOST_SystemWin32::ProcessEvents ( bool waitForEvent ) {
	MSG msg;
	bool hasEventHandled = false;

	do {
		GHOST_TimerManager *timerMgr = this->mTimerManager;

		if ( waitForEvent && !::PeekMessageW ( &msg , NULL , 0 , 0 , PM_NOREMOVE ) ) {
		}

		if ( timerMgr->FireTimers ( this->GetMilliseconds ( ) ) ) {
			hasEventHandled = true;
		}

		// Process all the events waiting for us
		while ( ::PeekMessageW ( &msg , NULL , 0 , 0 , PM_REMOVE ) != 0 ) {
			// TranslateMessage doesn't alter the message, and doesn't change our raw keyboard data.
			// Needed for MapVirtualKey or if we ever need to get chars from wm_ime_char or similar.
			::TranslateMessage ( &msg );
			::DispatchMessageW ( &msg );
			hasEventHandled = true;
		}

		// PeekMessage above is allowed to dispatch messages to the wndproc without us
		// noticing, so we need to check the event manager here to see if there are
		// events waiting in the queue.
		hasEventHandled |= this->mEventManager->GetNumEvents ( ) > 0;
	} while ( waitForEvent && !hasEventHandled );

	return hasEventHandled;
}

GHOST_Event *ProcessMouseButtonEvent ( GHOST_TEventType type , GHOST_WindowWin32 *window , GHOST_TButton mask , WPARAM wParam , LPARAM lParam ) {
	GHOST_EventMouseButton *data = new GHOST_EventMouseButton ( );
	data->button = mask;
	data->x = LOWORD ( lParam );
	data->y = HIWORD ( lParam );
	if ( type == GHOST_kEventWheel ) {
		data->deltaWheel = ( ( short ) HIWORD ( wParam ) );
	} else {
		data->deltaWheel = 0;
	}
	return new GHOST_Event ( type , data , GHOST_System::GetSystem ( )->GetMilliseconds ( ) , window );
}

GHOST_Event *ProcessWindowCloseEvent ( GHOST_WindowWin32 *window ) {
	return new GHOST_Event ( GHOST_kEventWindowClose , nullptr , GHOST_System::GetSystem ( )->GetMilliseconds ( ) , window );
}

LRESULT WINAPI GHOST_SystemWin32::WndProc ( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam ) {
	GHOST_WindowWin32 *self = ( GHOST_WindowWin32 * ) GetWindowLongPtrW ( hWnd , GWLP_USERDATA );
	GHOST_SystemWin32 *sys = ( GHOST_SystemWin32 * ) GHOST_System::GetSystem ( );
	switch ( msg ) {
		case WM_CLOSE: {
			/* The WM_CLOSE message is sent as a signal that a window
			* or an application should terminate. Restore if minimized. */
			if ( GHOST_System::GetSystem ( ) ) {
				GHOST_System::GetSystem ( )->SetSystemShouldExit ( true );
			}
		}break;
		case WM_DESTROY: {
			sys->mEventManager->PushEvent ( ProcessWindowCloseEvent ( self ) );
		}break;
		case WM_MOUSEMOVE: {
			sys->mEventManager->PushEvent ( ProcessMouseButtonEvent ( GHOST_kEventCursorMove , self , GHOST_kButtonMaskNone , wParam , lParam ) );
		}break;
		case WM_LBUTTONDOWN: {
			sys->mEventManager->PushEvent ( ProcessMouseButtonEvent ( GHOST_kEventButtonDown , self , GHOST_kButtonMaskLeft , wParam , lParam ) );
		}break;
		case WM_LBUTTONUP: {
			sys->mEventManager->PushEvent ( ProcessMouseButtonEvent ( GHOST_kEventButtonUp , self , GHOST_kButtonMaskLeft , wParam , lParam ) );
		}break;
		case WM_RBUTTONDOWN: {
			sys->mEventManager->PushEvent ( ProcessMouseButtonEvent ( GHOST_kEventButtonDown , self , GHOST_kButtonMaskRight , wParam , lParam ) );
		}break;
		case WM_RBUTTONUP: {
			sys->mEventManager->PushEvent ( ProcessMouseButtonEvent ( GHOST_kEventButtonUp , self , GHOST_kButtonMaskRight , wParam , lParam ) );
		}break;
		case WM_MBUTTONDOWN: {
			sys->mEventManager->PushEvent ( ProcessMouseButtonEvent ( GHOST_kEventButtonDown , self , GHOST_kButtonMaskMiddle , wParam , lParam ) );
		}break;
		case WM_MBUTTONUP: {
			sys->mEventManager->PushEvent ( ProcessMouseButtonEvent ( GHOST_kEventButtonUp , self , GHOST_kButtonMaskMiddle , wParam , lParam ) );
		}break;
		case WM_XBUTTONDOWN: {
			sys->mEventManager->PushEvent ( ProcessMouseButtonEvent ( GHOST_kEventButtonDown , self , ( GHOST_TButton ) ( GHOST_kButtonMaskRight + HIWORD ( wParam ) ) , wParam , lParam ) );
		}break;
		case WM_XBUTTONUP: {
			sys->mEventManager->PushEvent ( ProcessMouseButtonEvent ( GHOST_kEventButtonUp , self , ( GHOST_TButton ) ( GHOST_kButtonMaskRight + HIWORD ( wParam ) ) , wParam , lParam ) );
		}break;
		case WM_MOUSEWHEEL: {
			sys->mEventManager->PushEvent ( ProcessMouseButtonEvent ( GHOST_kEventWheel , self , GHOST_kButtonMaskNone , wParam , lParam ) );
		}break;
	}
	return DefWindowProc ( hWnd , msg , wParam , lParam );
}
