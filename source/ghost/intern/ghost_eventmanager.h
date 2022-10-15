#pragma once

#include <deque>
#include <vector>

#include "ghost/ghost_ievtconsumer.h"

#include "ghost_event.h"

class GHOST_EventManager {
public:
	// Constructor.
	GHOST_EventManager ( );

	// Destructor.
	~GHOST_EventManager ( );

	/** Returns the number of events currently on the stack.
	* \return The number of events on the stack. */
	uint32_t GetNumEvents ( ) const;

	/** Returns the number of events currently on the stack of a specific type.
	* \param type The type of event to count.
	* \return The number of events on the stack of the given type. */
	uint32_t GetNumEvents ( GHOST_TEventType type ) const;

	/** Pushes an event on the stack.
	* To dispatch it call any of the dispatch methods.
	* Do not delete the event
	* \param event The event to push on the stack.
	* \return Indication of success. */
	GHOST_TStatus PushEvent ( GHOST_IntEvent *event );

	// Dispatches the given event directly, bypassing the event stack.
	void DispatchEvent ( GHOST_IntEvent *event );

	/** Dispatches the event at the back of the stack. 
	* The event will be removed from the stack. */
	void DispatchEvent ( );

	/** Dispatches all events on the stack.
	* The event stack will be empty afterwards. */
	void DispatchEvents ( );

	/** Adds a consumer to the list of event consumers.
	* \param consumer The consumer added to the list.
	* \return Indication as to whether addition has succeeded.
	*/
	GHOST_TStatus AddConsumer ( GHOST_IntEventConsumer *consumer );

	/** Removes a consumer from the list of event consumers.
	* \param consumer The consumer removed from the list.
	* \return Indication as to whether removal has succeeded. */
	GHOST_TStatus RemoveConsumer ( GHOST_IntEventConsumer *consumer );
	
	/** Removes all events for a window from the stack.
	* \param windo The window to remove events for. */
	void RemoveWindowEvents ( GHOST_IntWindow *window );
	
	/** Removes all events of a certain type from the stack.
	* The window parameter is optiional. 
	* If non-null, the routine will remove events only associated with that window.
	* \param type The type of events to be removed.
	* \param window The window to remove events for. */
	void RemoveTypeEvents ( GHOST_TEventType type , GHOST_IntWindow *window = NULL );
protected:
	//! Removes all events from the stack.
	void DisposeEvents ( );

	typedef std::deque<GHOST_IntEvent *> TEventStack; // A stack with events.
	typedef std::vector<GHOST_IntEventConsumer *> TConsumerVector; // A vector with event consumers.

	TEventStack mEvents;
	TEventStack mHandledEvents;

	TConsumerVector mConsumers;
};
