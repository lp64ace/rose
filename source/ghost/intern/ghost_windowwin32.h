#pragma once

#ifndef _WIN32
#  error This header should only be included for WIN32 applications.
#endif

#include <windows.h>
#undef CreateWindow

#include "ghost_window.h"

class GHOST_WindowWin32 : public GHOST_Window {
public:
	/** Creates a new window and opens it.
	* To check if the window was created properly, use the #GetValid method.
	* \param parent The parent window.
	* \param title The text shown in the title bar of the window.
	* \param left The coordinate of the left edge of the window.
	* \param top The coordinate of the top edge of the window.
	* \param width The width the window.
	* \param height The height the window. */
	GHOST_WindowWin32 ( GHOST_WindowWin32 *parent , const char *title , int32_t left , int32_t top , uint32_t width , uint32_t height );

	//! Closes the window and disposes resources allocated.
	~GHOST_WindowWin32 ( );

	/** Returns indication as to whether the window is valid.
	* \return The validity of the window. */
	bool GetValid ( ) const;

	/** Returns the associated OS object/handle.
	* \return The associated OS object/handle. */
	void *GetOSWindow ( ) const;

	/** Sets the title displayed in the title bar.
	* \param title: The title to display in the title bar. */
	void SetTitle ( const char *title );

	/** Returns the title displayed in the title bar.
	* \return The title displayed in the title bar. */
	std::string GetTitle ( ) const;

	/** Retrieves the dimensions of the bounding rectangle of the specified window.
	* The dimensions are given in screen coordinates that are relative to the upper-left corner of the screen.
	* \return A GHOST_Rect structure describing the area of the window. */
	GHOST_Rect GetWindowBounds ( ) const;

	/** Retrieves the coordinates of a window's client area.
	* The client coordinates specify the upper-left and lower-right corners of the client area.
	* Because client coordinates are relative to the upper-left corner of a window's client area,
	* the coordinates of the upper-left corner are (0,0).
	* \return A GHOST_Rect structure describing the client area of the window. */
	GHOST_Rect GetClientBounds ( ) const;

	/** The ScreenToClient function converts the screen coordinates of a specified point on the screen to client-area coordinates.
	* \param p_x The x coordinate we want to convert, if this is NULL it's value is ignored.
	* \param p_y The y coordinate we want to convert, if this is NULL it's value is ignored.
	*/
	void ScreenToClient ( int *p_x , int *p_y ) const;

	/** The ClientToScreen function converts the client-area coordinates of a specified point to screen coordinates.
	* \param p_x The x coordinate we want to convert, if this is NULL it's value is ignored.
	* \param p_y The y coordinate we want to convert, if this is NULL it's value is ignored.
	*/
	void ClientToScreen ( int *p_x , int *p_y ) const;
protected:
	/** Tries to install a rendering context in this window.
	* \param type: The type of rendering context installed.
	* \return Indication as to whether installation has succeeded. */
	GHOST_Context *NewDrawingContext ( GHOST_TDrawingContextType type );
private:
	HWND mHandle;
	HDC mDC;
};
