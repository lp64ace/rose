#include "ghost_callbackconsumer.h"

GHOST_CallbackEventConsumer::GHOST_CallbackEventConsumer ( GHOST_EventCallbackProcPtr eventCallback , GHOST_UserDataPtr userData ) {
	this->mEventCallback = eventCallback;
	this->mUserData = userData;
}

bool GHOST_CallbackEventConsumer::ProcessEvent ( GHOST_IntEvent *evt ) {
	return this->mEventCallback ( ( GHOST_EventHandle ) evt , mUserData );
}
