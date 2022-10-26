#include "ghost/ghost_api.h"
#include "ghost/ghost_isystem.h"

#include "ghost_callbackconsumer.h"

GHOST_SystemHandle GHOST_create_system ( void ) {
	if ( GHOST_IntSystem::CreateSystem ( ) ) {
		return ( GHOST_SystemHandle ) GHOST_IntSystem::GetSystem ( );
	}
	return NULL;
}

GHOST_TStatus GHOST_dispose_system ( GHOST_SystemHandle handle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) handle;
	return system->DisposeSystem ( );
}

bool GHOST_get_system_should_exit ( GHOST_SystemHandle handle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) handle;
	return system->GetSystemShouldExit ( );
}

void GHOST_set_system_should_exit ( GHOST_SystemHandle handle , bool shouldExit ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) handle;
	system->SetSystemShouldExit ( shouldExit );
}

/***************************************************************************************
* Time(r) functionality
***************************************************************************************/

GHOST_TimePoint GHOST_get_time ( GHOST_SystemHandle systemhandle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return system->GetTime ( );
}

uint64_t GET_get_milliseconds ( GHOST_SystemHandle systemhandle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return system->GetMilliseconds ( );
}

GHOST_TimerTaskHandle GHOST_install_timer ( GHOST_SystemHandle systemhandle , uint64_t delay , uint64_t interval , GHOST_TimerProcPtr timerProc , GHOST_UserDataPtr userData ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return ( GHOST_TimerTaskHandle ) system->InstallTimer ( delay , interval , timerProc , userData );
}

GHOST_TStatus GHOST_remove_timer ( GHOST_SystemHandle systemhandle , GHOST_TimerTaskHandle timertaskhandle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return system->RemoveTimer ( ( GHOST_IntTimerTask * ) timertaskhandle );
}

GHOST_TimerProcPtr GHOST_get_timer_proc ( GHOST_TimerTaskHandle timertaskhandle ) {
	GHOST_IntTimerTask *task = ( GHOST_IntTimerTask * ) timertaskhandle;
	return task->GetTimerProc ( );
}

void GHOST_set_timer_proc ( GHOST_TimerTaskHandle timertaskhandle , GHOST_TimerProcPtr callback ) {
	GHOST_IntTimerTask *task = ( GHOST_IntTimerTask * ) timertaskhandle;
	task->SetTimerProc ( callback );
}

GHOST_UserDataPtr GHOST_get_timer_user_data ( GHOST_TimerTaskHandle timertaskhandle ) {
	GHOST_IntTimerTask *task = ( GHOST_IntTimerTask * ) timertaskhandle;
	return task->GetUserData ( );
}

void GHOST_set_timer_user_data ( GHOST_TimerTaskHandle timertaskhandle , GHOST_UserDataPtr data ) {
	GHOST_IntTimerTask *task = ( GHOST_IntTimerTask * ) timertaskhandle;
	task->SetUserData ( data );
}

/***************************************************************************************
* Display management functionality
***************************************************************************************/

uint8_t GHOST_get_num_displays ( GHOST_SystemHandle systemhandle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return system->GetNumDisplays ( );
}

void GHOST_get_main_display_resolution ( GHOST_SystemHandle systemhandle , uint32_t *width , uint32_t *height ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return system->GetMainDisplayDimensions ( width , height );
}

void GHOST_get_all_display_resolution ( GHOST_SystemHandle systemhandle , uint32_t *width , uint32_t *height ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return system->GetAllDisplayDimensions ( width , height );
}

/***************************************************************************************
* Window management functionality
***************************************************************************************/

GHOST_WindowHandle GHOST_spawn_window ( GHOST_SystemHandle systemhandle , GHOST_WindowHandle parent , const char *title , int32_t left , int32_t top , uint32_t width , uint32_t height ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return ( GHOST_WindowHandle ) system->SpawnWindow ( title , left , top , width , height , ( GHOST_IntWindow * ) parent );
}

GHOST_TStatus GHOST_destroy_window ( GHOST_SystemHandle systemhandle , GHOST_WindowHandle windowhandle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return system->DisposeWindow ( window );
}

GHOST_UserDataPtr GHOST_get_window_user_data ( GHOST_WindowHandle windowhandle ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->GetUserData ( );
}

void GHOST_set_window_user_data ( GHOST_WindowHandle windowhandle , GHOST_UserDataPtr userdata ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	window->SetUserData ( userdata );
}

bool GHOST_get_window_valid ( GHOST_WindowHandle windowhandle ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->GetValid ( );
}

GHOST_TDrawingContextType GHOST_get_window_drawing_context ( GHOST_WindowHandle windowhandle ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->GetDrawingContextType ( );
}

GHOST_TStatus GHOST_set_window_drawing_context ( GHOST_WindowHandle windowhandle , GHOST_TDrawingContextType type ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->SetDrawingContextType ( type );
}

GHOST_TStatus GHOST_swap_buffers ( GHOST_WindowHandle windowhandle ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->SwapBuffers ( );
}

GHOST_TStatus GHOST_activate_window_context ( GHOST_WindowHandle windowhandle ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->ActivateDrawingContext ( );
}

GHOST_TStatus GHOST_release_window_context ( GHOST_WindowHandle windowhandle ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->ReleaseDrawingContext ( );
}

GHOST_TStatus GHOST_set_swap_interval ( GHOST_WindowHandle windowhandle , int interval ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->SetSwapInterval ( interval );
}

GHOST_TStatus GHOST_get_swap_interval ( GHOST_WindowHandle windowhandle , int *interval ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->GetSwapInterval ( interval );
}

GHOST_Rect GHOST_get_window_client_bounds ( GHOST_WindowHandle windowhandle ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->GetClientBounds ( );
}

GHOST_Rect GHOST_get_window_bounds ( GHOST_WindowHandle windowhandle ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->GetWindowBounds ( );
}

void GHOST_screen_to_client ( GHOST_WindowHandle windowhandle , int *p_x , int *p_y ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	window->ScreenToClient ( p_x , p_y );
}

void GHOST_client_to_screen ( GHOST_WindowHandle windowhandle , int *p_x , int *p_y ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	window->ClientToScreen ( p_x , p_y );
}

/***************************************************************************************
* Event management functionality
***************************************************************************************/

bool GHOST_process_events ( GHOST_SystemHandle systemhandle , bool wait ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return system->ProcessEvents ( wait );
}

void GHOST_dispatch_events ( GHOST_SystemHandle systemhandle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	system->DispatchEvents ( );
}

GHOST_EventConsumerHandle GHOST_create_event_consumer ( GHOST_EventCallbackProcPtr callback , GHOST_UserDataPtr userdata ) {
	return ( GHOST_EventConsumerHandle ) new GHOST_CallbackEventConsumer ( callback , userdata );
}

GHOST_TStatus GHOST_dispose_event_consumer ( GHOST_EventConsumerHandle consumerhandle ) {
	delete ( ( GHOST_CallbackEventConsumer * ) consumerhandle );
	return GHOST_kSuccess;
}

GHOST_TStatus GHOST_add_event_consumer ( GHOST_SystemHandle systemhandle , GHOST_EventConsumerHandle consumerhandle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return system->AddConsumer ( ( GHOST_CallbackEventConsumer * ) consumerhandle );
}

GHOST_TStatus GHOST_remove_event_consumer ( GHOST_SystemHandle systemhandle , GHOST_EventConsumerHandle consumerhandle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return system->RemoveConsumer ( ( GHOST_CallbackEventConsumer * ) consumerhandle );
}

GHOST_TEventType GHOST_get_event_type ( GHOST_EventHandle evthandle ) {
	GHOST_IntEvent *event = ( GHOST_IntEvent * ) evthandle;
	return event->GetType ( );
}

GHOST_TEventDataPtr GHOST_get_event_data ( GHOST_EventHandle evthandle ) {
	GHOST_IntEvent *event = ( GHOST_IntEvent * ) evthandle;
	return event->GetData ( );
}

uint64_t GHOST_get_event_timestamp ( GHOST_EventHandle evthandle ) {
	GHOST_IntEvent *event = ( GHOST_IntEvent * ) evthandle;
	return event->GetTime ( );
}

GHOST_WindowHandle GHOST_get_event_window ( GHOST_EventHandle evthandle ) {
	GHOST_IntEvent *event = ( GHOST_IntEvent * ) evthandle;
	return ( GHOST_WindowHandle ) event->GetWindow ( );
}
