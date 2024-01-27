#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <vector>

#include "intern/context.hh"
#include "intern/platform.hh"
#include "intern/window.hh"

class WindowsPlatform : public PlatformInterface {
public:
	/** Initialize the platform for windows, we can assume that we are on a WIN32 system in this source file. */
	WindowsPlatform();

	/** Destruct the paltform object, we can assume that we are on a WIN32 system in this source file. */
	~WindowsPlatform();

	/** Returns true if the platform object was initialized with success. */
	bool IsValid() const;

public:
	/* -------------------------------------------------------------------- */
	/** \name Window Managing
	 * \{ */

	/**
	 * Create a window of the specified dimensions and title and install a new rendering context for it.
	 *
	 * \param parent The parent window (can be NULL in case it is the application window).
	 * \param width The width of the window client in pixels.
	 * \param height The height of the window client in pixels.
	 *
	 * \note The window is not a classic operating system window, it is a borderless window with no accessories since we are
	 * gonna handle all the widget rendering.
	 *
	 * \return Returns a handle to the window or NULL if a window could not be created.
	 */
	WindowInterface *InitWindow(WindowInterface *parent, int width, int height);

	/** If the window is valid the window is destroyed and the associated memory is freed. */
	void CloseWindow(WindowInterface *window);

	/** This function should return true if the specified window is registered in this platform. */
	bool IsWindow(WindowInterface *window);

	/* \} */

	/* -------------------------------------------------------------------- */
	/** \name Context Managing
	 * \{ */

	/**
	 * This method will attempt to create and install a context of the specified type for the window.
	 *
	 * Windows have a default context when they are created and all windows must have a valid context attached to them, so even
	 * if a context of the specified type could not be installed then we fallback to the default one, which can be obtained by
	 * calling #GetContext.
	 *
	 * \return Returns the newly installed context or NULL if the context could not be installed (the window will still have a
	 * valid context).
	 */
	ContextInterface *InstallContext(WindowInterface *window, int type);

	/** This method will return the current context of the window. */
	ContextInterface *GetContext(WindowInterface *window);

	/* \} */

	/* -------------------------------------------------------------------- */
	/** \name System Managing
	 * \{ */

	/** Process the events that the operating system generated. */
	bool ProcessEvents(bool wait);

	/* \} */
public:
	static LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	/* -------------------------------------------------------------------- */
	/** \name Windows Handles
	 * \{ */

	WNDCLASSEXA _WndClass;

	/* \} */

	std::vector<WindowInterface *> _Windows;

	bool _IsValid;
};

class WindowsWindow : public WindowInterface {
public:
	WindowsWindow(WindowsWindow *parent, int width, int height);
	~WindowsWindow();

	/* -------------------------------------------------------------------- */
	/** \name Size/Pos Managing
	 * \{ */

	/** Sets the size of the entire window, including borders and decorations. */
	void SetWindowSize(GSize size);
	/** Sets the size of the client area of the window, excluding borders and decorations. */
	void SetClientSize(GSize size);
	/** Sets the position of the top-left corner of the window. */
	void SetPos(GPosition position);

	/** Retrieves the rectangular coordinates of the entire window, including borders and decorations. */
	void GetWindowRect(GRect *r_rect) const;
	/** Retrieves the rectangular coordinates of the client area of the window, excluding borders and decorations. */
	void GetClientRect(GRect *r_rect) const;

	/* \} */

	/* -------------------------------------------------------------------- */
	/** \name Context Managing
	 * \{ */

	/**
	 * Attempt to install a new rendering context of the specified type for the window. If this function fails, the rendering
	 * context for the window will fallback to the old type to prevent having a window with no context (in this case, the
	 * function will still return NULL). If the new context is installed successfully, the return value is a pointer to the
	 * specified context.
	 *
	 * \param type The type of the rendering context to be installed.
	 * \return A pointer to the installed context or NULL if installation fails.
	 */
	ContextInterface *InstallContext(int type);

	/** Returns the context of the specified window, this should never return NULL. */
	ContextInterface *GetContext() const;

	/* \} */
private:
	/* -------------------------------------------------------------------- */
	/** \name Window Handles
	 * \{ */

	HWND _hWnd;
	HDC _hDC;

	/* \} */

	ContextInterface *_Context;

	friend class WindowsOpenGLContext;
};

class WindowsOpenGLContext : public ContextInterface {
public:
	WindowsOpenGLContext(WindowsWindow *window);
	~WindowsOpenGLContext();

	/** Returns true if the context is valid. */
	bool IsValid();

	/* -------------------------------------------------------------------- */
	/** \name Context Management
	 * \{ */

	/** Make this context the active rendering context for this application instance. */
	bool Activate();

	/** Make the current rendering context an invalid rendering context. */
	bool Deactivate();

	/** Excange the front and back buffer of the window. */
	bool SwapBuffers();

	/* \} */
private:
	/* -------------------------------------------------------------------- */
	/** \name Context Handles
	 * \{ */

	HDC _hDC;
	HGLRC _hGLRC;

	/* \} */
};
