#pragma once

#include "intern/platform.hh"

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
};
