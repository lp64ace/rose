#pragma once

#include "ghost/ghost_icontext.h"

class GHOST_Context : public GHOST_IntContext {
public:
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

	/** Called immediately after new to initialize. If this fails then immediately delete the object.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	virtual GHOST_TStatus InitDrawingContext ( ) = 0;

	/** Swaps front and back buffers of a window.
	* \return A boolean success indicator. */
	virtual GHOST_TStatus SwapBuffers ( ) = 0;

	/** Checks if it is OK for a remove the native display
	* \return Indication as to whether removal has succeeded. */
	virtual GHOST_TStatus ReleaseNativeHandles ( ) = 0;

	/** Gets the current swap interval for #SwapBuffers.
	* \param pointer to location to return swap interval.
	* \return A boolean success indicator of if swap interval was successfully read. */
	virtual GHOST_TStatus GetSwapInterval ( int *interval ) const = 0;

	/** Sets the swap interval for #SwapBuffers.
	* \param interval The swap interval to use.
	* \return A boolean success indicator of if swap interval was successfully read. */
	virtual GHOST_TStatus SetSwapInterval ( int interval ) const = 0;

	/** Returns the user data assigned to the window.
	* \return The user data. */
	GHOST_UserDataPtr GetUserData ( ) const;

	/** Updates the window's user data.
	* \param data The new user data for the window. */
	void SetUserData ( GHOST_UserDataPtr data );
protected:
	GHOST_UserDataPtr mUserData;
};
