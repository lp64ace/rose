#pragma once

#include <deque>
#include <set>
#include <chrono>

#include "platform.h"

class WindowInterface;
class ContextInterface;
class PlatformInterface;

extern const char *glib_application_name;
extern PlatformInterface *glib_platform;

class PlatformInterface {
public:
	PlatformInterface();
	virtual ~PlatformInterface();

	/* -------------------------------------------------------------------- */
	/** \name Time Utils
	 * \{ */

	double GetTime() const;

	/* \} */

	/* -------------------------------------------------------------------- */
	/** \name Generic Utils
	 * \{ */

	/** Returns the size of the main display. */
	virtual GSize GetScreenSize() const = 0;
	/** Returns the cursor position relative to the window specified. */
	virtual GPosition GetCursorPosition(WindowInterface *window) const = 0;
	/** Returns the window under the cursor location. */
	virtual WindowInterface *GetWindowUnderCursor(int x, int y) const = 0;

	/* \} */

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

	/** This function should return true if the specified window is registered in this platform. */
	virtual bool IsWindow(WindowInterface *window) = 0;

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
	void EventSubscribe(EventCallbackFn fn, void *userdata);

	/** Unsubscribe a function from receiving event triggers. */
	void EventUnsubscribe(EventCallbackFn fn);

	/** This will post an event to the message bus, the event will not be dispatched until #DipsatchEvents is called. */
	void PostEvent(WindowInterface *wnd, int type, void *evtdata);

	/** This will immidiately send the specified event to all the event subscribers. */
	void DispatchEvent(WindowInterface *wnd, int type, void *evtdata);

	/** Dispatch all the events that have been processed or posted. */
	void DispatchEvents();

	/** Returns the number of events waiting to be dispatched. */
	size_t NumEvents() const;

	/* \} */

public:
	/* -------------------------------------------------------------------- */
	/** \name Internal Event Posting
	 * \{ */

	void PostWindowSizeEvent(WindowInterface *wnd, int cx, int cy);
	void PostWindowMoveEvent(WindowInterface *wnd, int x, int y);
	void PostMouseMoveEvent(WindowInterface *wnd, int x, int y, int modifiers);
	void PostMouseWheelEvent(WindowInterface *wnd, int x, int y, int modifiers, int wheel);
	void PostMouseButtonEvent(WindowInterface *wnd, int type, int x, int y, int modifiers);
	void PostKeyDownEvent(WindowInterface *wnd, int key, bool repeat, int modifiers, const wchar_t *utf16);
	void PostKeyUpEvent(WindowInterface *wnd, int key, bool repeat, int modifiers, const wchar_t *utf16);
	void PostDestroyEvent(WindowInterface *wnd);

	void ClearWindowEvents(WindowInterface *wnd);

	/* \} */

private:
	void DisposeEvents();

private:
	struct EventCallbackEntry {
		EventCallbackEntry(EventCallbackFn fn, void *userdata);
		~EventCallbackEntry();

		EventCallbackFn func;
		void *userdata;

		bool operator<(const EventCallbackEntry &entry) const;
	};

	std::set<EventCallbackEntry> _Subscribers;

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
	
	std::chrono::high_resolution_clock::time_point _TimeBegin;
};
