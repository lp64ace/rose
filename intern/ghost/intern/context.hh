#pragma once

class WindowInterface;

class ContextInterface {
public:
	virtual ~ContextInterface() = default;

	/** Returns true if the context is valid. */
	virtual bool IsValid() = 0;

	/* -------------------------------------------------------------------- */
	/** \name Context Management
	 * \{ */

	/** Make this context the active rendering context for this application instance. */
	virtual bool Activate() = 0;

	/** Make the current rendering context an invalid rendering context. */
	virtual bool Deactivate() = 0;
	
	/** Returns the swap interval of this context. */
	virtual int GetSwapInterval() = 0;

	/** Set the swap interval of this context */
	virtual bool SetSwapInterval(int interval) = 0;

	/** Excange the front and back buffer of the window. */
	virtual bool SwapBuffers() = 0;

	/* \} */
};
