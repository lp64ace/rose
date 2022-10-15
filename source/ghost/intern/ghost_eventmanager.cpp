#include "ghost_eventmanager.h"

GHOST_EventManager::GHOST_EventManager ( ) {
	
}

GHOST_EventManager::~GHOST_EventManager ( ) {
	DisposeEvents ( );

	for ( TConsumerVector::iterator itr = this->mConsumers.begin ( ); itr != this->mConsumers.end ( ); ++itr ) {
		GHOST_IntEventConsumer *consumer = ( *itr );
		delete consumer;
	}

	this->mConsumers.clear ( );
}

uint32_t GHOST_EventManager::GetNumEvents ( ) const {
	return ( uint32_t ) this->mEvents.size ( );
}

uint32_t GHOST_EventManager::GetNumEvents ( GHOST_TEventType type ) const {
	uint32_t numEvents = 0;
	for ( TEventStack::const_iterator p = this->mEvents.begin ( ); p != this->mEvents.end ( ); ++p ) {
		if ( ( *p )->GetType ( ) == type ) {
			numEvents++;
		}
	}
	return numEvents;
}

GHOST_TStatus GHOST_EventManager::PushEvent ( GHOST_IntEvent *event ) {
	GHOST_TStatus success;
	if ( this->mEvents.size ( ) < this->mEvents.max_size ( ) ) {
		this->mEvents.push_front ( event );
		success = GHOST_kSuccess;
	} else {
		success = GHOST_kFailure;
	}
	return success;
}

void GHOST_EventManager::DispatchEvent ( GHOST_IntEvent *event ) {
	for ( TConsumerVector::iterator iter = this->mConsumers.begin ( ); iter != this->mConsumers.end ( ); ++iter ) {
		( *iter )->ProcessEvent ( event );
	}
}

void GHOST_EventManager::DispatchEvent ( ) {
	GHOST_IntEvent *event = this->mEvents.back ( );
	this->mEvents.pop_back ( );
	this->mHandledEvents.push_back ( event );

	DispatchEvent ( event );
}

void GHOST_EventManager::DispatchEvents ( ) {
	while ( !this->mEvents.empty ( ) ) {
		DispatchEvent ( );
	}
	DisposeEvents ( );
}

GHOST_TStatus GHOST_EventManager::AddConsumer ( GHOST_IntEventConsumer *consumer ) {
	GHOST_TStatus success;

	TConsumerVector::const_iterator iter = std::find ( this->mConsumers.begin ( ) , this->mConsumers.end ( ) , consumer );
	if ( iter == this->mConsumers.end ( ) ) {
		this->mConsumers.push_back ( consumer );
		success = GHOST_kSuccess;
	} else {
		success = GHOST_kFailure;
	}
	return success;
}

GHOST_TStatus GHOST_EventManager::RemoveConsumer ( GHOST_IntEventConsumer *consumer ) {
	GHOST_TStatus success;

	TConsumerVector::const_iterator iter = std::find ( this->mConsumers.begin ( ) , this->mConsumers.end ( ) , consumer );
	if ( iter != this->mConsumers.end ( ) ) {
		this->mConsumers.erase ( iter );
		success = GHOST_kSuccess;
	} else {
		success = GHOST_kFailure;
	}
	return success;
}

void GHOST_EventManager::RemoveWindowEvents ( GHOST_IntWindow *window ) {
	TEventStack::iterator iter;
	iter = this->mEvents.begin ( );
	while ( iter != this->mEvents.end ( ) ) {
		GHOST_IntEvent *event = *iter;
		if ( event->GetWindow ( ) == window ) {
			delete event;
			this->mEvents.erase ( iter );
			iter = this->mEvents.begin ( );
		} else {
			++iter;
		}
	}
}

void GHOST_EventManager::RemoveTypeEvents ( GHOST_TEventType type , GHOST_IntWindow *window ) {
	TEventStack::iterator iter;
	iter = this->mEvents.begin ( );
	while ( iter != this->mEvents.end ( ) ) {
		GHOST_IntEvent *event = *iter;
		if ( ( event->GetType ( ) == type ) && ( !window || ( event->GetWindow ( ) == window ) ) ) {
			delete event;
			this->mEvents.erase ( iter );
			iter = this->mEvents.begin ( );
		} else {
			++iter;
		}
	}
}

void GHOST_EventManager::DisposeEvents ( ) {
	while ( this->mHandledEvents.empty ( ) == false ) {
		delete this->mHandledEvents [ 0 ];
		this->mHandledEvents.pop_front ( );
	}

	while ( this->mEvents.empty ( ) == false ) {
		delete this->mEvents [ 0 ];
		this->mEvents.pop_front ( );
	}
}
