#pragma once

#include "ghost_types.h"

class GHOST_IntTimerTask {
public:
	virtual ~GHOST_IntTimerTask ( ) = default;

	/** Returns the timer task's timer procedure callback function.
	* \return The timer callback. */
	virtual GHOST_TimerProcPtr GetTimerProc ( ) const = 0;

	/** Updates the timer callback of the timer task.
	* \param callback The new callback method for the task. */
	virtual void SetTimerProc ( GHOST_TimerProcPtr callback ) = 0;

	/** Returns the user data for the timer task.
	* \return The user data for the timer task. */
	virtual GHOST_UserDataPtr GetUserData ( ) const = 0;

	/** Updates the timer callback for the timer task.
	* \param data The new user data for the timer task. */
	virtual void SetUserData ( GHOST_UserDataPtr data ) = 0;
};
