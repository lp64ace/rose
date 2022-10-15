#pragma once

#include "ghost_window.h"

#include <vector>

class GHOST_WindowManager final {
public:
	// Constructor
	GHOST_WindowManager ( );

	// Destructor.
	~GHOST_WindowManager ( );

	/** Add a window to our list.
	* It is only added if it is not already in the list.
	* \param window Pointer to the window to be added.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	GHOST_TStatus AddWindow ( GHOST_Window *window );

	/** Remove a window from our list.
	* \param window Pointer to the window to be removed.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	GHOST_TStatus RemoveWindow ( const GHOST_Window *window );

	/** Returns whether the window is in our list.
	* \param window Pointer to the window to query.
	* \return A boolean indicator. */
	bool GetWindowFound ( const GHOST_Window *window ) const;

	/** Return a vector of the windows currently managed by this class.
	* \return Constant reference to the vector of windows managed */
	const std::vector<GHOST_Window *> &GetWindows ( ) const;
protected:
	std::vector<GHOST_Window *> mWindows; // The list of windows managed.
};
