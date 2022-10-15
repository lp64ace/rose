#pragma once

#ifndef _WIN32
#  error This header should only be included for WIN32 applications.
#endif

#include "ghost_context.h"

#include <windows.h>
#undef CreateWindow

class GHOST_ContextWGL : public GHOST_Context {
public:
	/** creates a new OpenGL rendering context, which is suitable for drawing on the device referenced by hdc. 
	* The pixel format of the device context will be properly changed.
	* \param hWnd A handle to the window of the context.
	* \param hDC A handle to the window's device context. */
	GHOST_ContextWGL ( HWND hWnd , HDC hDC );

	//! Destructor
	~GHOST_ContextWGL ( );

	/** Makes the specified rendering context the calling thread's current rendering context.
	* All subsequent context calls made by the thread are drawn on the device identified.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	GHOST_TStatus ActivateDrawingContext ( );

	/** Makes the specified rendering context so that it's no longer current.
	* All subsequent context calls made by the thread are drawn on the device identified.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	GHOST_TStatus ReleaseDrawingContext ( );

	/** Called immediately after new to initialize. If this fails then immediately delete the object.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	GHOST_TStatus InitDrawingContext ( );

	/** Swaps front and back buffers of a window.
	* \return A boolean success indicator. */
	GHOST_TStatus SwapBuffers ( );

	/** Checks if it is OK for a remove the native display
	* \return Indication as to whether removal has succeeded. */
	GHOST_TStatus ReleaseNativeHandles ( );

	/** Gets the current swap interval for #SwapBuffers.
	* \param pointer to location to return swap interval.
	* \return A boolean success indicator of if swap interval was successfully read. */
	GHOST_TStatus GetSwapInterval ( int *interval ) const;

	/** Sets the swap interval for #SwapBuffers.
	* \param interval The swap interval to use.
	* \return A boolean success indicator of if swap interval was successfully read. */
	GHOST_TStatus SetSwapInterval ( int interval ) const;
private:
	HGLRC mHandle;
	HWND mWindowHandle;
	HDC mDeviceHandle;

	static HGLRC sSharedContext;
	static ULONG sSharedCount;
};
