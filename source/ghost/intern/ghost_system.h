#pragma once

#include "ghost/ghost_isystem.h"

#include "ghost_timermanager.h"
#include "ghost_windowmanager.h"
#include "ghost_eventmanager.h"
#include "ghost_callbackconsumer.h"

// Platform indipendent system methods.

class GHOST_System : public GHOST_IntSystem {
public:
	/***************************************************************************************
	* Time(r) functionality
	***************************************************************************************/

	/** Returns the system time, returns the number of seconds since the start of the
	* system process, based on ANSI clock() routine, this is more accurate than GetMilliseconds.
	* return The number of seconds elapsed since the start of the process. */
	GHOST_TimePoint GetTime ( ) const;

	/** Returns the system time, returns the number of milliseconds since the start of the
	* system process, based on ANSI clock() routine.
	* \return The number of milliseconds elapsed since the start of the process. */
	uint64_t GetMilliseconds ( ) const;

	/** Installs a timer.
	* Note that, on most operating systems, messages need to be processed in order
	* for the timer callbacks to be invoked.
	* \param delay The time to wait for the first call to the timerProc (in milliseconds).
	* \param interval The internval between calls to the timer callback proc (in milliseconds).
	* \param proc The callback invoked when the interval expires.
	* \param Placeholder for user data.
	* \return A timer task (NULL if timer task installation failed). */
	GHOST_IntTimerTask *InstallTimer ( uint64_t delay , uint64_t interval , GHOST_TimerProcPtr proc , GHOST_UserDataPtr user );

	/** Remove a timer from the timer manager of this system.
	* \param timer The timer task to be removed.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	GHOST_TStatus RemoveTimer ( GHOST_IntTimerTask *timer );
public:
	/** Mark the application exit flag, use the GetSystemShouldExit to get the value of this flag.
	* \param shouldExit A flag indication wether or not the system should exit. */
	inline void SetSystemShouldExit ( bool shouldExit ) { this->mShouldExit = shouldExit; }

	/** Returns the system exit flag set by the SetSystemShouldExit method.
	* \return Indication as to wether the application should exit. */
	inline bool GetSystemShouldExit ( ) const { return this->mShouldExit; }
public:
	/***************************************************************************************
	* Window management functionality
	***************************************************************************************/

	/** Create a new window.
	* The new window is added to the list of windows managed.
	* Never explicity delete the window, use #DisposeWindow instead.
	* \param title The name of the window.
	* \param left The coordinate of the left edge of the window.
	* \param top The coordinate of the top edge of the window.
	* \param width The width the window.
	* \param height The height the window.
	* \param state The state of the window when opened.
	* \param type The type of drawing context installed in this window.
	* \param settings Misc OpenGL settings.
	* \param exclusive Use to show the window on top and ignore others (used full-screen).
	* \param dialog Stay on top of parent window, no icon in taskbar, can't be minimized.
	* \param parent Parent (embedder) window.
	* \return The new window (or NULL if creation failed). */
	virtual GHOST_IntWindow *SpawnWindow ( const char *title , int32_t left , int32_t top , int32_t width , int32_t height , const GHOST_IntWindow *parent = NULL ) = 0;

	/** Dispose a window.
	* \param window Pointer to the window to be disposed.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	GHOST_TStatus DisposeWindow ( GHOST_IntWindow *window );
public:
	/***************************************************************************************
	* Event management functionality
	***************************************************************************************/

	//! Dispatch the events to the event consumers.
	void DispatchEvents ( );

	/** Adds a consumer to the list of event consumers.
	* \param consumer The consumer added to the list.
	* \return Indication as to whether addition has succeeded. */
	GHOST_TStatus AddConsumer ( GHOST_IntEventConsumer *consumer );

	/** Removes a consumer from the list of event consumers.
	* \param consumer The consumer removed from the list.
	* \return Indication as to whether removal has succeeded. */
	GHOST_TStatus RemoveConsumer ( GHOST_IntEventConsumer *consumer );
protected:
	/** Called after the system has been created and the default managers have als been created.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	virtual GHOST_TStatus Init ( );

	/** Called before the system is to be disposed, it can be assumed that the managers have not yet been deleted.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	virtual GHOST_TStatus Exit ( );
private:
	bool mShouldExit;

	time_t mStartTime;
};
