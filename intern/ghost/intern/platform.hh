#pragma once

#include <deque>
#include <set>

#include "platform.h"

class WindowInterface;
class ContextInterface;

class PlatformInterface {
public:
	PlatformInterface();
	virtual ~PlatformInterface();

	/* -------------------------------------------------------------------- */
	/** \name Window Managing
	 * \{ */

	/**
	 * Create a window of the specified dimensions and title and install a new rendering context for it.
	 *
	 * \param parent The parent window (can be NULL in case it is the application window).
	 * \param width The width of the window client in pixels.
	 * \param height The height of the window client in pixels.
	 *
	 * \note The window is not a classic operating system window, it is a borderless window with no accessories since we are
	 * gonna handle all the widget rendering.
	 *
	 * \return Returns a handle to the window or NULL if a window could not be created.
	 */
	virtual WindowInterface *InitWindow(WindowInterface *parent, int width, int height) = 0;

	/** If the window is valid the window is destroyed and the associated memory is freed. */
	virtual void CloseWindow(WindowInterface *window) = 0;

	/* \} */

	/* -------------------------------------------------------------------- */
	/** \name Context Managing
	 * \{ */

	/**
	 * This method will attempt to create and install a context of the specified type for the window.
	 *
	 * Windows have a default context when they are created and all windows must have a valid context attached to them, so even
	 * if a context of the specified type could not be installed then we fallback to the default one, which can be obtained by
	 * calling #GetContext.
	 *
	 * \return Returns the newly installed context or NULL if the context could not be installed (the window will still have a
	 * valid context).
	 */
	virtual ContextInterface *InstallContext(WindowInterface *window, int type) = 0;

	/** This method will return the current context of the window. */
	virtual ContextInterface *GetContext(WindowInterface *window) = 0;

	/* \} */

	/* -------------------------------------------------------------------- */
	/** \name System Managing
	 * \{ */

	/** Process the events that the operating system generated. */
	virtual bool ProcessEvents(bool wait) = 0;

	/* \} */

public:
	/* -------------------------------------------------------------------- */
	/** \name Event Managing
	 * \{ */

	/**
	 * Subscribe a new function to receive event triggers. It is not guaranteed that a function if two functions `func1` and
	 * `func2` are registered in oreder that they will receive the events in order.
	 */
	void EventSubscribe(EventCallbackFn fn);

	/** Unsubscribe a function from receiving event triggers. */
	void EventUnsubscribe(EventCallbackFn fn);

	/** This will post an event to the message bus, the event will not be dispatched until #DipsatchEvents is called. */
	void PostEvent(WindowInterface *wnd, int type, void *evtdata);

	/** This will immidiately send the specified event to all the event subscribers. */
	void DispatchEvent(WindowInterface *wnd, int type, void *evtdata);
	
	/** Dispatch all the events that have been processed or posted. */
	void DispatchEvents();

	/* \} */

protected:
	/* -------------------------------------------------------------------- */
	/** \name Internal Event Posting
	 * \{ */

	void PostWindowSizeEvent(WindowInterface *wnd, int cx, int cy);
	void PostWindowMoveEvent(WindowInterface *wnd, int x, int y);
	void PostMouseMoveEvent(WindowInterface *wnd, int x, int y, int modifiers);
	void PostMouseWheelEvent(WindowInterface *wnd, int x, int y, int modifiers, int wheel);
	void PostMouseButtonEvent(WindowInterface *wnd, int type, int x, int y, int modifiers);
	void PostKeyEvent(WindowInterface *wnd, wchar_t v, int key, int scan, int prev, int transition);
	void PostKeyDownEvent(WindowInterface *wnd, wchar_t v, int key, int scan, int prev);
	void PostKeyUpEvent(WindowInterface *wnd, wchar_t v, int key, int scan);

	/* \} */

private:
	void DisposeEvents();

private:
	std::set<EventCallbackFn> _Subscribers;

	template<typename Wnd, typename Evt, typename Data> struct EventEntry {
		EventEntry(Wnd *, Evt, Data *);
		~EventEntry();

		Wnd *window;
		Evt type;
		Data *data;
	};

	typedef EventEntry<WindowInterface, int, void> DefEventEntry;

	std::deque<DefEventEntry *> _EvtQueue;
	std::deque<DefEventEntry *> _EvtHandled;
};
