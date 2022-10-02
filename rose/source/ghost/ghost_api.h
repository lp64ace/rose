#pragma once

#include "ghost_types.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

	/** Definition of a callback routine that receives events.
	* \param evt The event received.
	* \param userdata The callback's user data, supplied to #GHOST_CreateSystem.
	* \return Indication on wether the event was handled. */
	typedef bool ( *GHOST_EventCallbackProcPtr )( GHOST_EventHandle evt , GHOST_UserDataPtr userdata );

	/** Create the one and only 'GHOST' system, GHOST_System constructor is protected
	* so that  the user will be forced to create this method to init the system. This will
	* select the apropriate system based on the platform.
	* \return If the function succeds the return value is a handle to the system
	* (a nonzero value), otherwise the return value is NULL indicating failure. */
	extern GHOST_SystemHandle ghostCreateSystem ( void );

	/** Disposes the one and only 'GHOST' system, calling this without first calling
	* the CreateSystem will cause this function to fail.
	* \param handle The handle to the system that was returned from the GHOST_create_system method.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	extern GHOST_TStatus ghostDisposeSystem ( GHOST_SystemHandle handle );

	/** Returns the system exit flag set by the SetSystemShouldExit method.
	* \param handle The handle to the system .
	* \return Indication as to wether the application should exit. */
	extern bool ghostGetSystemShouldExit ( GHOST_SystemHandle handle );

	/** Mark the application exit flag, use the GetSystemShouldExit to get the value of this flag.
	* \param handle The handle to the system .
	* \param shouldExit A flag indication wether or not the system should exit. */
	extern void ghostSetSystemShouldExit ( GHOST_SystemHandle handle , bool shouldExit );

	/***************************************************************************************
	* Time(r) functionality
	***************************************************************************************/

	/** Returns the system time, returns the number of seconds since the start of the
	* \param systemhandle The handle to the system.
	* system process, based on ANSI clock() routine, this is more accurate than GetMilliseconds.
	* return The number of seconds elapsed since the start of the process. */
	extern GHOST_TimePoint ghostGetTime ( GHOST_SystemHandle systemhandle );

	/** Returns the system time.
	* Returns the number of milliseconds since the start of the system process.
	* Based on ANSI clock() routine.
	* \param systemhandle The handle to the system.
	* \return The number of milliseconds. */
	extern uint64_t ghostGetMilliseconds ( GHOST_SystemHandle systemhandle );

	/** Installs a timer.
	* Note that, on most operating systems, messages need to be processed in order
	* for the timer callbacks to be invoked.
	* \param systemhandle The handle to the system.
	* \param delay The time to wait for the first call to the timerProc (in milliseconds).
	* \param interval The interval between calls to the timerProc (in milliseconds).
	* \param timerProc The callback invoked when the interval expires.
	* \param userData Placeholder for user data.
	* \return A timer task (0 if timer task installation failed). */
	extern GHOST_TimerTaskHandle ghostInstallTimer ( GHOST_SystemHandle systemhandle , uint64_t delay , uint64_t interval , GHOST_TimerProcPtr timerProc , GHOST_UserDataPtr userData );

	/** Removes a timer.
	* \param systemhandle The handle to the system.
	* \param timertaskhandle Timer task to be removed.
	* \return Indication of success. */
	extern GHOST_TStatus ghostRemoveTimer ( GHOST_SystemHandle systemhandle , GHOST_TimerTaskHandle timertaskhandle );

	/** Returns the timer task's timer procedure callback function.
	* \param timertaskhandle The timer task handle.
	* \return The timer callback. */
	extern GHOST_TimerProcPtr ghostGetTimerProc ( GHOST_TimerTaskHandle timertaskhandle );

	/** Updates the timer callback of the timer task.
	* \param timertaskhandle The timer task handle.
	* \param callback The new callback method for the task. */
	extern void ghostSetTimerProc ( GHOST_TimerTaskHandle timertaskhandle , GHOST_TimerProcPtr callback );

	/** Returns the user data for the timer task.
	* \param timertaskhandle The timer task handle.
	* \return The user data for the timer task. */
	extern GHOST_UserDataPtr ghostGetTimerUserData ( GHOST_TimerTaskHandle timertaskhandle );

	/** Updates the timer callback for the timer task.
	* \param timertaskhandle The timer task handle.
	* \param data The new user data for the timer task. */
	extern void ghotSetTimerUserData ( GHOST_TimerTaskHandle timertaskhandle , GHOST_UserDataPtr data );

	/***************************************************************************************
	* Display management functionality
	***************************************************************************************/

	/** Returns the number of displays on this system.
	* \param systemhandle The handle to the system.
	* \return The number of displays. */
	extern uint8_t ghostGetNumDisplays ( GHOST_SystemHandle systemhandle );

	/** Returns the dimensions of the main display on this system.
	* \param systemhandle The handle to the system.
	* \param width A pointer the width gets put in.
	* \param height A pointer the height gets put in. */
	extern void ghostGetMainDisplayResolution ( GHOST_SystemHandle systemhandle , uint32_t *width , uint32_t *height );

	/** Returns the dimensions of all displays combine.
	* (the current workspace).
	* No need to worry about overlapping monitors.
	* \param systemhandle The handle to the system.
	* \param width A pointer the width gets put in.
	* \param height A pointer the height gets put in. */
	extern void ghostGetAllDisplayResolution ( GHOST_SystemHandle systemhandle , uint32_t *width , uint32_t *height );

	/***************************************************************************************
	* Window management functionality
	***************************************************************************************/

	/** Create a new window.
	* The new window is added to the list of windows managed.
	* Never explicitly delete the window, use DisposeWindow() instead.
	* \param system The handle to the system.
	* \param parent Handle of parent (or owner) window, or NULL
	* \param title The name of the window.
	* (displayed in the title bar of the window if the OS supports it).
	* \param left The coordinate of the left edge of the window.
	* \param top The coordinate of the top edge of the window.
	* \param width The width the window.
	* \param height The height the window.
	* \return A handle to the new window ( == NULL if creation failed). */
	extern GHOST_WindowHandle ghostSpawnWindow ( GHOST_SystemHandle system , GHOST_WindowHandle parent , const char *title , int32_t left , int32_t top , uint32_t width , uint32_t height );

	/** Returns the window user data.
	* \param windowhandle The handle to the window.
	* \return The window user data. */
	extern GHOST_UserDataPtr ghostGetWindowUserData ( GHOST_WindowHandle windowhandle );

	/** Changes the window user data.
	* \param windowhandle The handle to the window.
	* \param userdata The window user data. */
	extern void ghostSetWindowUserData ( GHOST_WindowHandle windowhandle , GHOST_UserDataPtr userdata );

	/** Returns whether a window is valid.
	* \param windowhandle Handle to the window to be checked.
	* \return Indication of validity. */
	extern bool ghostGetWindowValid ( GHOST_WindowHandle windowhandle );

	/** Returns the type of drawing context used in this window.
	* \param windowhandle The handle to the window.
	* \return The current type of drawing context. */
	extern GHOST_TDrawingContextType ghostGetWindowDrawingContext ( GHOST_WindowHandle window );

	/** Tries to install a rendering context in this window.
	* \param windowhandle The handle to the window.
	* \param type The type of rendering context to be isntalled.
	* \return Indication as to whether installation has succeeded. */
	extern GHOST_TStatus ghostSetWindowDrawingContext ( GHOST_WindowHandle window , GHOST_TDrawingContextType type );

	/** Swap buffers for the specified window.
	* \param window The window whose front and back buffer we want to swap.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	extern GHOST_TStatus ghostSwapBuffers ( GHOST_WindowHandle window );

	/** Set the interval of the #GHOST_swap_buffers method.
	* \param window The window whose swap interval we want to update.
	* \param interval The interval we want to use.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	extern GHOST_TStatus ghostSetSwapInterval ( GHOST_WindowHandle window , int interval );

	/** Fetches the swap interval of the #GHOST_swap_buffers method.
	* \param window The window whose swap interval we want to fetch.
	* \param interval Pointer to the variable we want to update with the interval value.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	extern GHOST_TStatus ghostGetSwapInterval ( GHOST_WindowHandle window , int *interval );

	/** Activate the drawing context of the window.
	* \param window The window whose front and back buffer we want to swap.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	extern GHOST_TStatus ghostActivateWindowContext ( GHOST_WindowHandle window );

	/** Release the drawing context of the window.
	* \param window The window whose front and back buffer we want to swap.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	extern GHOST_TStatus ghostReleaseWindowContext ( GHOST_WindowHandle window );

	/** Returns the client bounds rectangle of the specified window.
	* \param window The winodw handle whose window client rect we want to get.
	* \return Returns the client rect of the window. */
	extern GHOST_Rect gostGetWindowClientBounds ( GHOST_WindowHandle window );

	/** Returns the bounds rectangle of the specified window.
	* \param window The winodw handle whose window rect we want to get.
	* \return Returns the rect of the window. */
	extern GHOST_Rect gostGetWindowBounds ( GHOST_WindowHandle window );

	/***************************************************************************************
	* Event management functionality
	***************************************************************************************/

	/** Retrieves events from the system and stores them in the queue.
	* \param systemhandle: The handle to the system.
	* \param waitForEvent: Boolean to indicate that #ProcessEvents should.
	* wait (block) until the next event before returning.
	* \return Indication of the presence of events. */
	extern bool ghostProcessEvents ( GHOST_SystemHandle systemhandle , bool wait = false );

	/** Retrieves events from the queue and send them to the event consumers.
	* call ghostProcessEvents to retrieve new events from the current system.
	* \param systemhandle: The handle to the system. */
	extern void ghostDispatchEvents ( GHOST_SystemHandle systemhandle );

	/** Creates an event consumer object
	* \param callback The event callback routine.
	* \param userdata Pointer to user data returned to the callback routine. */
	extern GHOST_EventConsumerHandle ghostCreateEventConsumer ( GHOST_EventCallbackProcPtr callback , GHOST_UserDataPtr userdata );

	/** Disposes an event consumer object
	* \param consumerhandle: Handle to the event consumer.
	* \return An indication of success. */
	extern GHOST_TStatus ghostDisposeEventConsumer ( GHOST_EventConsumerHandle consumerhandle );

	/** Adds the given event consumer to our list.
	* \param systemhandle: The handle to the system.
	* \param consumerhandle: The event consumer to add.
	* \return Indication of success. */
	extern GHOST_TStatus ghostAddEventConsumer ( GHOST_SystemHandle systemhandle , GHOST_EventConsumerHandle consumerhandle );

	/** Remove the given event consumer to our list.
	* \param systemhandle: The handle to the system.
	* \param consumerhandle: The event consumer to remove.
	* \return Indication of success. */
	extern GHOST_TStatus ghotRemoveEventConsumer ( GHOST_SystemHandle systemhandle , GHOST_EventConsumerHandle consumerhandle );

	/** Returns the event type from an event handle.
	* \param evthandle The handle to the event.
	* \return Returns a GHOST_TEventType describing the type of the event. */
	extern GHOST_TEventType ghostGetEventType ( GHOST_EventHandle evthandle );

	/** Returns the data associated with the event.
	* \param evthandle The handle to the event.
	* \return Returns the data of the event this can be casted to the appropriate event type data according to the event type. */
	extern GHOST_TEventDataPtr ghostGetEventData ( GHOST_EventHandle evthandle );

	/** Returns the time ( in milliseconds ) when the event was generated.
	* \param evthandle The handle to the event.
	* \return The event generation time. */
	extern uint64_t ghostGetEventTimestamp ( GHOST_EventHandle evthandle );

	/** Returns a handle to the window that generated the message.
	* \param evthandle The handle to the event.
	* \return The handle of the window that generated the message or NULL if the message is a system message. */
	extern GHOST_WindowHandle ghostGetEventWindow ( GHOST_EventHandle evthandle );

#ifdef __cplusplus
}
#endif
