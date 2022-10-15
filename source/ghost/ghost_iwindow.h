#pragma once

#include "ghost_types.h"

#include <sstream>
#include <string>
#include <cmath>

class GHOST_IntWindow {
public:
	// Constructor.
	GHOST_IntWindow ( ) = default;

	// Destructor.
	virtual ~GHOST_IntWindow ( ) = default;

	/** Returns indication as to whether the window is valid.
	* \return The validity of the window. */
	virtual bool GetValid ( ) const = 0;

	/** Returns the associated OS object/handle.
	* \return The associated OS object/handle. */
	virtual void *GetOSWindow ( ) const = 0;

	/** Sets the title displayed in the title bar.
	* \param title: The title to display in the title bar. */
	virtual void SetTitle ( const char *title ) = 0;

	/** Returns the title displayed in the title bar.
	* \return The title displayed in the title bar. */
	virtual std::string GetTitle ( ) const = 0;

	/** Retrieves the dimensions of the bounding rectangle of the specified window. 
	* The dimensions are given in screen coordinates that are relative to the upper-left corner of the screen.
	* \return A GHOST_Rect structure describing the area of the window. */
	virtual GHOST_Rect GetWindowBounds ( ) const = 0;

	/** Retrieves the coordinates of a window's client area. 
	* The client coordinates specify the upper-left and lower-right corners of the client area. 
	* Because client coordinates are relative to the upper-left corner of a window's client area, 
	* the coordinates of the upper-left corner are (0,0).
	* \return A GHOST_Rect structure describing the client area of the window. */
	virtual GHOST_Rect GetClientBounds ( ) const = 0;

	/** Retrieves the position of the window.
	* \return Returns a GHOST_Point structure describint the position of the top left window's point. */
	virtual GHOST_Point GetWindowPoint ( ) const = 0;

	/** Retrieves the size of the window's area.
	* \return Returns a GHOST_Size structure describint the size of the area of the window. */
	virtual GHOST_Size GetWindowSize ( ) const = 0;

	/** Retrieves the size of the window's client area.
	* \return Returns a GHOST_Size structure describint the size of the client area of the window. */
	virtual GHOST_Size GetClientSize ( ) const = 0;

	/** Returns the user data assigned to the window.
	* \return The user data. */
	virtual GHOST_UserDataPtr GetUserData ( ) const = 0;

	/** Updates the window's user data.
	* \param data The new user data for the window. */
	virtual void SetUserData ( GHOST_UserDataPtr data ) = 0;

	/** Swaps front and back buffers of a window.
	* \return A boolean success indicator. */
	virtual GHOST_TStatus SwapBuffers ( ) = 0;

	/** Gets the current swap interval for #SwapBuffers.
	* \param pointer to location to return swap interval.
	* \return A boolean success indicator of if swap interval was successfully read. */
	virtual GHOST_TStatus GetSwapInterval ( int *interval ) const = 0;

	/** Sets the swap interval for #SwapBuffers.
	* \param interval The swap interval to use.
	* \return A boolean success indicator of if swap interval was successfully read. */
	virtual GHOST_TStatus SetSwapInterval ( int interval ) const = 0;

	/** Returns the type of drawing context used in this window.
	* \return The current type of drawing context. */
	virtual GHOST_TDrawingContextType GetDrawingContextType ( ) const = 0;

	/** Tries to install a rendering context in this window.
	* \param type The type of rendering context to be isntalled.
	* \return Indication as to whether installation has succeeded. */
	virtual GHOST_TStatus SetDrawingContextType ( GHOST_TDrawingContextType type ) = 0;

	/** Activates the drawing context of this window.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	virtual GHOST_TStatus ActivateDrawingContext ( ) = 0;

	/** Makes the specified rendering context so that it's no longer current.
	* All subsequent context calls made by the thread are drawn on the device identified.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	virtual GHOST_TStatus ReleaseDrawingContext ( ) = 0;
};
