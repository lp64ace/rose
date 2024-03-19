#include "window.hh"

void WindowInterface::SetWindowCaptionRect(int xmin, int xmax, int ymin, int ymax) {
	_CaptionRect.left = xmin;
	_CaptionRect.right = xmax;
	_CaptionRect.top = ymin;
	_CaptionRect.bottom = ymax;
}

GSize WindowInterface::GetWindowSize() const {
	GRect rect;
	GSize size;

	GetWindowRect(&rect);

	size.x = rect.right - rect.left;
	size.y = rect.bottom - rect.top;

	return size;
}

GSize WindowInterface::GetClientSize() const {
	GRect rect;
	GSize size;

	GetClientRect(&rect);

	size.x = rect.right - rect.left;
	size.y = rect.bottom - rect.top;

	return size;
}

GPosition WindowInterface::GetPos() const {
	GRect rect;
	GPosition position;

	GetWindowRect(&rect);

	position.x = rect.left;
	position.y = rect.top;

	return position;
}

void WindowInterface::SetUserData(void *userdata) {
	_UserData = userdata;
}

void *WindowInterface::GetUserData(void) const {
	return _UserData;
}
