#pragma once

#include "ghost_ievent.h"

class GHOST_IntEventConsumer {
public:
	// Consumer.
	virtual ~GHOST_IntEventConsumer ( ) = default;

	/**
	* This method is called by the system when it has events to dispatch.
	* \param event: The event that can be handled or ignored.
	* \return Indication as to whether the event was handled. */
	virtual bool ProcessEvent ( GHOST_IntEvent *evt ) = 0;
};
