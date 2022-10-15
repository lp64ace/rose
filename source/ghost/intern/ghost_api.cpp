#include "ghost/ghost_api.h"
#include "ghost/ghost_isystem.h"

#include "ghost_callbackconsumer.h"

GHOST_SystemHandle ghostCreateSystem ( void ) {
	if ( GHOST_IntSystem::CreateSystem ( ) ) {
		return ( GHOST_SystemHandle ) GHOST_IntSystem::GetSystem ( );
	}
	return NULL;
}

GHOST_TStatus ghostDisposeSystem ( GHOST_SystemHandle handle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) handle;
	return system->DisposeSystem ( );
}

bool ghostGetSystemShouldExit ( GHOST_SystemHandle handle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) handle;
	return system->GetSystemShouldExit ( );
}

void ghostSetSystemShouldExit ( GHOST_SystemHandle handle , bool shouldExit ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) handle;
	system->SetSystemShouldExit ( shouldExit );
}

/***************************************************************************************
* Time(r) functionality
***************************************************************************************/

GHOST_TimePoint ghostGetTime ( GHOST_SystemHandle systemhandle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return system->GetTime ( );
}

uint64_t ghostGetMilliseconds ( GHOST_SystemHandle systemhandle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return system->GetMilliseconds ( );
}

GHOST_TimerTaskHandle ghostInstallTimer ( GHOST_SystemHandle systemhandle , uint64_t delay , uint64_t interval , GHOST_TimerProcPtr timerProc , GHOST_UserDataPtr userData ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return ( GHOST_TimerTaskHandle ) system->InstallTimer ( delay , interval , timerProc , userData );
}

GHOST_TStatus ghostRemoveTimer ( GHOST_SystemHandle systemhandle , GHOST_TimerTaskHandle timertaskhandle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return system->RemoveTimer ( ( GHOST_IntTimerTask * ) timertaskhandle );
}

GHOST_TimerProcPtr ghostGetTimerProc ( GHOST_TimerTaskHandle timertaskhandle ) {
	GHOST_IntTimerTask *task = ( GHOST_IntTimerTask * ) timertaskhandle;
	return task->GetTimerProc ( );
}

void ghostSetTimerProc ( GHOST_TimerTaskHandle timertaskhandle , GHOST_TimerProcPtr callback ) {
	GHOST_IntTimerTask *task = ( GHOST_IntTimerTask * ) timertaskhandle;
	task->SetTimerProc ( callback );
}

GHOST_UserDataPtr ghostGetTimerUserData ( GHOST_TimerTaskHandle timertaskhandle ) {
	GHOST_IntTimerTask *task = ( GHOST_IntTimerTask * ) timertaskhandle;
	return task->GetUserData ( );
}

void ghotSetTimerUserData ( GHOST_TimerTaskHandle timertaskhandle , GHOST_UserDataPtr data ) {
	GHOST_IntTimerTask *task = ( GHOST_IntTimerTask * ) timertaskhandle;
	task->SetUserData ( data );
}

/***************************************************************************************
* Display management functionality
***************************************************************************************/

uint8_t ghostGetNumDisplays ( GHOST_SystemHandle systemhandle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return system->GetNumDisplays ( );
}

void ghostGetMainDisplayResolution ( GHOST_SystemHandle systemhandle , uint32_t *width , uint32_t *height ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return system->GetMainDisplayDimensions ( width , height );
}

void ghostGetAllDisplayResolution ( GHOST_SystemHandle systemhandle , uint32_t *width , uint32_t *height ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return system->GetAllDisplayDimensions ( width , height );
}

/***************************************************************************************
* Window management functionality
***************************************************************************************/

GHOST_WindowHandle ghostSpawnWindow ( GHOST_SystemHandle systemhandle , GHOST_WindowHandle parent , const char *title , int32_t left , int32_t top , uint32_t width , uint32_t height ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return ( GHOST_WindowHandle ) system->SpawnWindow ( title , left , top , width , height , ( GHOST_IntWindow * ) parent );
}

GHOST_UserDataPtr ghostGetWindowUserData ( GHOST_WindowHandle windowhandle ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->GetUserData ( );
}

void ghostSetWindowUserData ( GHOST_WindowHandle windowhandle , GHOST_UserDataPtr userdata ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	window->SetUserData ( userdata );
}

bool ghostGetWindowValid ( GHOST_WindowHandle windowhandle ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->GetValid ( );
}

GHOST_TDrawingContextType ghostGetWindowDrawingContext ( GHOST_WindowHandle windowhandle ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->GetDrawingContextType ( );
}

GHOST_TStatus ghostSetWindowDrawingContext ( GHOST_WindowHandle windowhandle , GHOST_TDrawingContextType type ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->SetDrawingContextType ( type );
}

GHOST_TStatus ghostSwapBuffers ( GHOST_WindowHandle windowhandle ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->SwapBuffers ( );
}

GHOST_TStatus ghostActivateWindowContext ( GHOST_WindowHandle windowhandle ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->ActivateDrawingContext ( );
}

GHOST_TStatus ghostReleaseWindowContext ( GHOST_WindowHandle windowhandle ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->ReleaseDrawingContext ( );
}

GHOST_TStatus ghostSetSwapInterval ( GHOST_WindowHandle windowhandle , int interval ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->SetSwapInterval ( interval );
}

GHOST_TStatus ghostGetSwapInterval ( GHOST_WindowHandle windowhandle , int *interval ) {

	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->GetSwapInterval ( interval );
}

GHOST_Rect gostGetWindowClientBounds ( GHOST_WindowHandle windowhandle ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->GetClientBounds ( );
}

GHOST_Rect gostGetWindowBounds ( GHOST_WindowHandle windowhandle ) {
	GHOST_IntWindow *window = ( GHOST_IntWindow * ) windowhandle;
	return window->GetWindowBounds ( );
}

/***************************************************************************************
* Event management functionality
***************************************************************************************/

bool ghostProcessEvents ( GHOST_SystemHandle systemhandle , bool wait ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return system->ProcessEvents ( wait );
}

void ghostDispatchEvents ( GHOST_SystemHandle systemhandle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	system->DispatchEvents ( );
}

GHOST_EventConsumerHandle ghostCreateEventConsumer ( GHOST_EventCallbackProcPtr callback , GHOST_UserDataPtr userdata ) {
	return ( GHOST_EventConsumerHandle ) new GHOST_CallbackEventConsumer ( callback , userdata );
}

GHOST_TStatus ghostDisposeEventConsumer ( GHOST_EventConsumerHandle consumerhandle ) {
	delete ( ( GHOST_CallbackEventConsumer * ) consumerhandle );
	return GHOST_kSuccess;
}

GHOST_TStatus ghostAddEventConsumer ( GHOST_SystemHandle systemhandle , GHOST_EventConsumerHandle consumerhandle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return system->AddConsumer ( ( GHOST_CallbackEventConsumer * ) consumerhandle );
}

GHOST_TStatus ghotRemoveEventConsumer ( GHOST_SystemHandle systemhandle , GHOST_EventConsumerHandle consumerhandle ) {
	GHOST_IntSystem *system = ( GHOST_IntSystem * ) systemhandle;
	return system->RemoveConsumer ( ( GHOST_CallbackEventConsumer * ) consumerhandle );
}

GHOST_TEventType ghostGetEventType ( GHOST_EventHandle evthandle ) {
	GHOST_IntEvent *event = ( GHOST_IntEvent * ) evthandle;
	return event->GetType ( );
}

GHOST_TEventDataPtr ghostGetEventData ( GHOST_EventHandle evthandle ) {
	GHOST_IntEvent *event = ( GHOST_IntEvent * ) evthandle;
	return event->GetData ( );
}

uint64_t ghostGetEventTimestamp ( GHOST_EventHandle evthandle ) {
	GHOST_IntEvent *event = ( GHOST_IntEvent * ) evthandle;
	return event->GetTime ( );
}

GHOST_WindowHandle ghostGetEventWindow ( GHOST_EventHandle evthandle ) {
	GHOST_IntEvent *event = ( GHOST_IntEvent * ) evthandle;
	return ( GHOST_WindowHandle ) event->GetWindow ( );
}
