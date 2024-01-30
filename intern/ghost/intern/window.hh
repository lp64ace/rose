#pragma once

#include "platform.h"

class ContextInterface;

class WindowInterface {
public:
	virtual ~WindowInterface() = default;

	/* -------------------------------------------------------------------- */
	/** \name Size/Pos Managing
	 * \{ */

	/** Retrieves the size of the entire window, including borders and decorations. */
	virtual GSize GetWindowSize() const;
	/** Retrieves the size of the client area of the window, excluding borders and decorations. */
	virtual GSize GetClientSize() const;
	/** Retrieves the position of the top-left corner of the window. */
	virtual GPosition GetPos() const;

	/** Sets the size of the entire window, including borders and decorations. */
	virtual void SetWindowSize(GSize size) = 0;
	/** Sets the size of the client area of the window, excluding borders and decorations. */
	virtual void SetClientSize(GSize size) = 0;
	/** Sets the position of the top-left corner of the window. */
	virtual void SetPos(GPosition position) = 0;

	/** Retrieves the rectangular coordinates of the entire window, including borders and decorations. */
	virtual void GetWindowRect(GRect *r_rect) const = 0;
	/** Retrieves the rectangular coordinates of the client area of the window, excluding borders and decorations. */
	virtual void GetClientRect(GRect *r_rect) const = 0;

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
	virtual ContextInterface *InstallContext(int type) = 0;

	/** Returns the context of the specified window, this should never return NULL. */
	virtual ContextInterface *GetContext() const = 0;

	/* \} */
public:
	/* -------------------------------------------------------------------- */
	/** \name UserData
	 * \{ */

	void SetUserData(void *userdata);
	void *GetUserData(void) const;

	/* \} */

private:
	void *_UserData;
};
