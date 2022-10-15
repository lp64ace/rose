#pragma once

#include "ghost/ghost_ievent.h"

class GHOST_Event : public GHOST_IntEvent {
public:
	GHOST_Event ( GHOST_TEventType type , GHOST_TEventDataPtr data , uint64_t time , GHOST_IntWindow *window )
		: mType ( type ) , mData ( data ) , mTime ( time ) , mWindow ( window ) {
	}

	~GHOST_Event ( ) {
		delete mData;
	}

	// Get the type of the event.
	inline GHOST_TEventType GetType ( ) const { return this->mType; }

	// Get the data associated with the event.
	inline GHOST_TEventDataPtr GetData ( ) const { return this->mData; }

	// Get the time stamp of the event.
	inline uint64_t GetTime ( ) const { return this->mTime; }

	// Get the window associated with the event.
	inline GHOST_IntWindow *GetWindow ( ) const { return this->mWindow; }
private:
	GHOST_TEventType mType;
	GHOST_TEventDataPtr mData;
	uint64_t mTime;
	GHOST_IntWindow *mWindow;
};
