#pragma once

#include "ghost/ghost_types.h"

#include "ghost_timertask.h"

#include <vector>

class GHOST_TimerManager {
public:
	// Create an empty timer manager with no timer tasks.
	GHOST_TimerManager ( );

	// Destroy the timer manager.
	virtual ~GHOST_TimerManager ( );

	/** Searches the list of timers to find the specified timer task.
	* \return Indication of existense of the timer within the timer list. */
	bool HasTimer ( GHOST_TimerTask *timer ) const;

	/** Adds a timer task to the list, it is only added when it is not already present in the list.
	* \param timer The timer task added to the list.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value indicating that the timer has been added to the lsit,
	* otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	GHOST_TStatus AddTimer ( GHOST_TimerTask *timer );

	/** Removes a timer task from the list, it is only removed when it is found in the list. 
	* This will also delete the timer.
	* \param timer The timer to be removed from the list.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value indicating that the timer has been removed from the lsit,
	* otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	GHOST_TStatus RemoveTimer ( GHOST_TimerTask *timer );

	/** Finds the soonest time the next timer would fire.
	* \return The soonest time the next timer would fire, or GHOST_kFireTimeNext if no timer that should
	* fire exists. */
	uint64_t GetNextFireTime ( ) const;

	/** Chekcs all timer task to see if they are expired and fires them if needed.
	* \param time The current time.
	* \return Returns \c True if any timers were fired. */
	bool FireTimers ( uint64_t time );

	/** Checks the specified timer task to see if they are expired and fires them if needed.
	* \param time The current time.
	* \param task The timer task to check and optionally fire.
	* \return \c True if the timer fired. */
	bool FireTimer ( uint64_t time , GHOST_TimerTask *task );
protected:
	/** Removes all the timers from the timer manager's list 
	* and deletes them. */
	void DisposeTimers ( );
protected:
	std::vector<GHOST_TimerTask *> mTasks;
};
