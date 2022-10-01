#pragma once

#include "ghost_system.h"

#ifndef _WIN32
#  error This header should only be included for WIN32 applications.
#endif

#include <windows.h>
#undef CreateWindow

class GHOST_SystemWin32 : public GHOST_System {
public:
	/** Returns the number of displays on this system.
	* \return The number of displays. */
	uint8_t GetNumDisplays ( ) const;

	/** Retrieves the dimensions of the main display on this system.
	* \param width A pointer to the variable that should receiver the width of the main display.
	* \param height A pointer to the variable that should receiver the height of the main display. */
	void GetMainDisplayDimensions ( uint32_t *width , uint32_t *height ) const;

	/** Retrieves the combine dimensions of all monitors.
	* \param width A pointer to the variable that should receiver the width of all the monitors.
	* \param height A pointer to the variable that should receiver the height of all the monitors. */
	void GetAllDisplayDimensions ( uint32_t *width , uint32_t *height ) const;
public:
	/** Retrieves events from the system and stores them in the queue.
	* \param waitForEvent: Flag to wait for an event (or return immediately).
	* \return Indication of the presence of events. */
	bool ProcessEvents ( bool waitForEvent );
public:
	/** Create a new window.
	* The new window is added to the list of windows managed.
	* Never explicity delete the window, use #DisposeWindow instead.
	* \param title The name of the window.
	* \param left The coordinate of the left edge of the window.
	* \param top The coordinate of the top edge of the window.
	* \param width The width the window.
	* \param height The height the window.
	* \param state The state of the window when opened.
	* \param type The type of drawing context installed in this window.
	* \param settings Misc OpenGL settings.
	* \param exclusive Use to show the window on top and ignore others (used full-screen).
	* \param dialog Stay on top of parent window, no icon in taskbar, can't be minimized.
	* \param parent Parent (embedder) window.
	* \return The new window (or NULL if creation failed). */
	GHOST_IntWindow *SpawnWindow ( const char *title , int32_t left , int32_t top , int32_t width , int32_t height , const GHOST_IntWindow *parent = NULL );
protected:
	/** Called after the system has been created and the default managers have als been created.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	virtual GHOST_TStatus Init ( );

	/** Called before the system is to be disposed, it can be assumed that the managers have not yet been deleted.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	virtual GHOST_TStatus Exit ( );
private:
	static LRESULT WINAPI WndProc ( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam );
};
