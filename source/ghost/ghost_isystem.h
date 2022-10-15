#pragma once

#include "ghost_types.h"

#include "ghost_itimertask.h"
#include "ghost_iwindow.h"
#include "ghost_ievtconsumer.h"

class GHOST_TimerManager;
class GHOST_WindowManager;
class GHOST_EventManager;

class GHOST_IntSystem { // GHOST System Interface
	static GHOST_IntSystem *sGhostSystem;
protected:
	GHOST_TimerManager *mTimerManager;
	GHOST_WindowManager *mWindowManager;
	GHOST_EventManager *mEventManager;
public:
	/** Create the one and only 'GHOST' system, GHOST_System constructor is protected
	* so that  the user will be forced to create this method to init the system. This will
	* select the apropriate system based on the platform.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	static GHOST_TStatus CreateSystem ( );

	/** Returns the one and only GHOST system.
	* \return If a system has been created returns the system otherwise returns NULL. */
	static GHOST_IntSystem *GetSystem ( );

	/** Disposes the one and only 'GHOST' system, calling this without first calling
	* the CreateSystem will cause this function to fail.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	static GHOST_TStatus DisposeSystem ( );
public:
	/** Mark the application exit flag, use the GetSystemShouldExit to get the value of this flag.
	* \param shouldExit A flag indication wether or not the system should exit. */
	virtual void SetSystemShouldExit ( bool shouldExit ) = 0;

	/** Returns the system exit flag set by the SetSystemShouldExit method.
	* \return Indication as to wether the application should exit. */
	virtual bool GetSystemShouldExit ( ) const = 0;
public:
	/***************************************************************************************
	* Time(r) functionality
	***************************************************************************************/

	/** Returns the system time, returns the number of seconds since the start of the
	* system process, based on ANSI clock() routine, this is more accurate than GetMilliseconds.
	* return The number of seconds elapsed since the start of the process. */
	virtual GHOST_TimePoint GetTime ( ) const = 0;

	/** Returns the system time, returns the number of milliseconds since the start of the
	* system process, based on ANSI clock() routine.
	* \return The number of milliseconds elapsed since the start of the process. */
	virtual uint64_t GetMilliseconds ( ) const = 0;

	/** Installs a timer.
	* Note that, on most operating systems, messages need to be processed in order
	* for the timer callbacks to be invoked.
	* \param delay The time to wait for the first call to the timerProc (in milliseconds).
	* \param interval The internval between calls to the timer callback proc (in milliseconds).
	* \param proc The callback invoked when the interval expires.
	* \param Placeholder for user data.
	* \return A timer task (NULL if timer task installation failed). */
	virtual GHOST_IntTimerTask *InstallTimer ( uint64_t delay , uint64_t interval , GHOST_TimerProcPtr proc , GHOST_UserDataPtr user ) = 0;

	/** Remove a timer from the timer manager of this system.
	* \param timer The timer task to be removed.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	virtual GHOST_TStatus RemoveTimer ( GHOST_IntTimerTask *timer ) = 0;
public:
	/***************************************************************************************
	* Display management functionality
	***************************************************************************************/

	/** Returns the number of displays on this system.
	* \return The number of displays. */
	virtual uint8_t GetNumDisplays ( ) const = 0;

	/** Retrieves the dimensions of the main display on this system.
	* \param width A pointer to the variable that should receiver the width of the main display.
	* \param height A pointer to the variable that should receiver the height of the main display. */
	virtual void GetMainDisplayDimensions ( uint32_t *width , uint32_t *height ) const = 0;

	/** Retrieves the combine dimensions of all monitors.
	* \param width A pointer to the variable that should receiver the width of all the monitors.
	* \param height A pointer to the variable that should receiver the height of all the monitors. */
	virtual void GetAllDisplayDimensions ( uint32_t *width , uint32_t *height ) const = 0;
public:
	/***************************************************************************************
	* Event management functionality
	***************************************************************************************/

	/** Retrieves events from the system and stores them in the queue.
	* \param waitForEvent: Flag to wait for an event (or return immediately).
	* \return Indication of the presence of events. */
	virtual bool ProcessEvents ( bool waitForEvent ) = 0;
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
	virtual GHOST_TStatus DisposeWindow ( GHOST_IntWindow *window ) = 0;
public:
	/***************************************************************************************
	* Event management functionality
	***************************************************************************************/

	//! Dispatch the events to the event consumers.
	virtual void DispatchEvents ( ) = 0;

	/** Adds a consumer to the list of event consumers.
	* \param consumer The consumer added to the list.
	* \return Indication as to whether addition has succeeded. */
	virtual GHOST_TStatus AddConsumer ( GHOST_IntEventConsumer *consumer ) = 0;

	/** Removes a consumer from the list of event consumers.
	* \param consumer The consumer removed from the list.
	* \return Indication as to whether removal has succeeded. */
	virtual GHOST_TStatus RemoveConsumer ( GHOST_IntEventConsumer *consumer ) = 0;
protected:
	/** This is protected so that the user will use the create system method instead.
	* To delete the system use the dispose system method. */
	GHOST_IntSystem ( ) = default;

	/** The system should becreated through the create system method and deleted 
	* through the dispose system method. */
	virtual ~GHOST_IntSystem ( ) = default;
protected:
	/** Called after the system has been created and the default managers have als been created.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	virtual GHOST_TStatus Init ( ) { return GHOST_kSuccess; }

	/** Called before the system is to be disposed, it can be assumed that the managers have not yet been deleted.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	virtual GHOST_TStatus Exit ( ) { return GHOST_kSuccess; }
};
