#pragma once

#include "ghost_context.h"

class GHOST_ContextNone : public GHOST_Context {
public:
	/** Makes the specified rendering context the calling thread's current rendering context.
	* All subsequent context calls made by the thread are drawn on the device identified.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	GHOST_TStatus ActivateDrawingContext ( ) { return GHOST_kSuccess; }

	/** Makes the specified rendering context so that it's no longer current.
	* All subsequent context calls made by the thread are drawn on the device identified.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	GHOST_TStatus ReleaseDrawingContext ( ) { return GHOST_kSuccess; }

	/** Called immediately after new to initialize. If this fails then immediately delete the object.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	GHOST_TStatus InitDrawingContext ( ) { return GHOST_kSuccess; }

	/** Swaps front and back buffers of a window.
	* \return A boolean success indicator. */
	GHOST_TStatus SwapBuffers ( ) { return GHOST_kSuccess; }

	/** Checks if it is OK for a remove the native display
	* \return Indication as to whether removal has succeeded. */
	GHOST_TStatus ReleaseNativeHandles ( ) { return GHOST_kSuccess; }

	/** Gets the current swap interval for #SwapBuffers.
	* \param pointer to location to return swap interval.
	* \return A boolean success indicator of if swap interval was successfully read. */
	GHOST_TStatus GetSwapInterval ( int *interval ) const { return GHOST_kSuccess; }

	/** Sets the swap interval for #SwapBuffers.
	* \param interval The swap interval to use.
	* \return A boolean success indicator of if swap interval was successfully read. */
	GHOST_TStatus SetSwapInterval ( int interval ) const { return GHOST_kSuccess; }
};
