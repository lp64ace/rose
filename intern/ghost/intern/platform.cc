#include "MEM_alloc.h"

#include "platform.hh"

#include <assert.h>

const char *glib_application_name = "Rose";
PlatformInterface *glib_platform = nullptr;

PlatformInterface::PlatformInterface() {
}

PlatformInterface::~PlatformInterface() {
	DisposeEvents();
}

/* -------------------------------------------------------------------- */
/** \name Event Managing
 * \{ */

void PlatformInterface::EventSubscribe(EventCallbackFn fn, void *userdata) {
	_Subscribers.insert(EventCallbackEntry(fn, userdata));
}

void PlatformInterface::EventUnsubscribe(EventCallbackFn fn) {
	_Subscribers.erase(EventCallbackEntry(fn, NULL));
}

void PlatformInterface::PostEvent(WindowInterface *wnd, int type, void *evtdata) {
	if (_EvtQueue.size() < _EvtQueue.max_size()) {
		_EvtQueue.push_front(MEM_new<DefEventEntry>("Event", wnd, type, evtdata));
	}
}

void PlatformInterface::DispatchEvent(WindowInterface *wnd, int type, void *evtdata) {
	std::set<EventCallbackEntry>::const_iterator itr;

	for (itr = _Subscribers.begin(); itr != _Subscribers.end(); itr++) {
		const EventCallbackEntry &entry = (*itr);
		entry.func(reinterpret_cast<GWindow *>(wnd), type, evtdata, entry.userdata);
	}
}

void PlatformInterface::DispatchEvents() {
	while (_EvtQueue.empty() == false) {
		DefEventEntry *evt = _EvtQueue.back();

		_EvtQueue.pop_back();
		_EvtHandled.push_back(evt);

		DispatchEvent(evt->window, evt->type, evt->data);
	}

	DisposeEvents();
}

size_t PlatformInterface::NumEvents() const {
	return _EvtQueue.size();
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Internal Event Posting
 * \{ */

void PlatformInterface::PostWindowSizeEvent(WindowInterface *wnd, int cx, int cy) {
	EventSize *data = MEM_cnew<EventSize>("glib::SizeEvent");

	data->cx = cx;
	data->cy = cy;

	PostEvent(wnd, GLIB_EVT_SIZE, data);
}

void PlatformInterface::PostWindowMoveEvent(WindowInterface *wnd, int x, int y) {
	EventMove *data = MEM_cnew<EventMove>("glib::MoveEvent");

	data->x = x;
	data->y = y;

	PostEvent(wnd, GLIB_EVT_MOVE, data);
}

void PlatformInterface::PostMouseMoveEvent(WindowInterface *wnd, int x, int y, int modifiers) {
	EventMouse *data = MEM_cnew<EventMouse>("glib::MouseMoveEvent");

	data->x = x;
	data->y = y;

	data->modifiers = modifiers;

	PostEvent(wnd, GLIB_EVT_MOUSEMOVE, data);
}

void PlatformInterface::PostMouseWheelEvent(WindowInterface *wnd, int x, int y, int modifiers, int wheel) {
	EventMouse *data = MEM_cnew<EventMouse>("glib::MouseWheelEvent");

	data->x = x;
	data->y = y;

	data->modifiers = modifiers;
	data->wheel = wheel;

	PostEvent(wnd, GLIB_EVT_MOUSEMOVE, data);
}

void PlatformInterface::PostMouseButtonEvent(WindowInterface *wnd, int type, int x, int y, int modifiers) {
	/** We do not want to link roselib here so we use plain assert instead. */
	assert(GLIB_EVT_LMOUSEDOWN <= type && type <= GLIB_EVT_RMOUSEUP);

	EventMouse *data = MEM_cnew<EventMouse>("glib::MouseButtonEvent");

	data->x = x;
	data->y = y;

	data->modifiers = modifiers;

	PostEvent(wnd, type, data);
}

void PlatformInterface::PostKeyDownEvent(WindowInterface *wnd, int key, bool repeat, int modifiers, const wchar_t *utf16) {
	EventKey *data = MEM_cnew<EventKey>("glib::KeyDownEvent");

	data->key = key;
	data->repeat = repeat;
	data->modifiers = modifiers;

	memcpy(data->utf16, utf16, sizeof(wchar_t[2]));

	PostEvent(wnd, GLIB_EVT_KEYDOWN, data);
}

void PlatformInterface::PostKeyUpEvent(WindowInterface *wnd, int key, bool repeat, int modifiers, const wchar_t *utf16) {
	EventKey *data = MEM_cnew<EventKey>("glib::KeyUpEvent");

	data->key = key;
	data->repeat = repeat;
	data->modifiers = modifiers;

	memcpy(data->utf16, utf16, sizeof(wchar_t[3]));

	PostEvent(wnd, GLIB_EVT_KEYUP, data);
}

void PlatformInterface::PostDestroyEvent(WindowInterface *wnd) {
	DispatchEvent(wnd, GLIB_EVT_DESTROY, NULL);
}

void PlatformInterface::ClearWindowEvents(WindowInterface *window) {
	std::deque<DefEventEntry *>::iterator iter;
	for (iter = _EvtQueue.begin(); iter != _EvtQueue.end();) {
		const DefEventEntry *event = *iter;
		if (event->window == window) {
			MEM_delete<DefEventEntry>(event);
			_EvtQueue.erase(iter);
			iter = _EvtQueue.begin();
		}
		else {
			++iter;
		}
	}
}

/* \} */

void PlatformInterface::DisposeEvents() {
	while (_EvtHandled.empty() == false) {
		MEM_delete<DefEventEntry>(_EvtHandled.front());
		_EvtHandled.pop_front();
	}
	while (_EvtQueue.empty() == false) {
		MEM_delete<DefEventEntry>(_EvtQueue.front());
		_EvtQueue.pop_front();
	}
}

PlatformInterface::EventCallbackEntry::EventCallbackEntry(EventCallbackFn fn, void *userdata) : func(fn), userdata(userdata) {
}

PlatformInterface::EventCallbackEntry::~EventCallbackEntry() {
}

bool PlatformInterface::EventCallbackEntry::operator<(const EventCallbackEntry &entry) const {
	return this->func < entry.func;
}

template<typename Wnd, typename Evt, typename Data> PlatformInterface::EventEntry<Wnd, Evt, Data>::EventEntry(Wnd *window, Evt type, Data *data) : window(window), type(type), data(data) {
}

template<typename Wnd, typename Evt, typename Data> PlatformInterface::EventEntry<Wnd, Evt, Data>::~EventEntry() {
	MEM_SAFE_FREE(data);
}
