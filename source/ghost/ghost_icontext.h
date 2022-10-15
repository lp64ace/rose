#pragma once

#include "ghost_types.h"

class GHOST_IntContext {
public:
	// Destructor.
	virtual ~GHOST_IntContext ( ) = default;

	/** Makes the specified rendering context the calling thread's current rendering context. 
	* All subsequent context calls made by the thread are drawn on the device identified.
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

	/** Swaps front and back buffers of a window.
	* \return A boolean success indicator. */
	virtual GHOST_TStatus SwapBuffers ( ) = 0;
};
