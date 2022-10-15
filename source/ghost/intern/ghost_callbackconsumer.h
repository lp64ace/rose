#pragma once

#include "ghost/ghost_api.h"
#include "ghost/ghost_ievtconsumer.h"

class GHOST_CallbackEventConsumer : public GHOST_IntEventConsumer {
public:
	/** Constructor.
	* \param eventCallback The call-back routine invoked.
	* \param userData The data passed back through the call-back routine.
	*/
	GHOST_CallbackEventConsumer ( GHOST_EventCallbackProcPtr eventCallback , GHOST_UserDataPtr userData );

	// Consumer.
	~GHOST_CallbackEventConsumer ( ) = default;

	/**
	* This method is called by the system when it has events to dispatch.
	* \param event: The event that can be handled or ignored.
	* \return Indication as to whether the event was handled. */
	bool ProcessEvent ( GHOST_IntEvent *evt );
protected:
	GHOST_EventCallbackProcPtr mEventCallback; // The call-back routine invoked.
	GHOST_UserDataPtr mUserData; //  The data passed back through the call-back routine. 
};
