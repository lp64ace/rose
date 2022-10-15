#pragma once

#include "ghost_types.h"
#include "ghost_iwindow.h"

class GHOST_IntEvent {
public:	
	virtual ~GHOST_IntEvent ( ) = default;

	// Get the type of the event.
	virtual GHOST_TEventType GetType ( ) const = 0;
	
	// Get the data associated with the event.
	virtual GHOST_TEventDataPtr GetData ( ) const = 0;
	
	// Get the time stamp of the event.
	virtual uint64_t GetTime ( ) const = 0;

	// Get the window associated with the event.
	virtual GHOST_IntWindow *GetWindow ( ) const = 0;
};
