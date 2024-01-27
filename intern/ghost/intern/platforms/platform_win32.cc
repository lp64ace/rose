#include "MEM_alloc.h"

#include "platform_win32.hh"

static class WindowsPlatform *glib_windows_platform = nullptr;

WindowsPlatform::WindowsPlatform() {
}

WindowsPlatform::~WindowsPlatform() {
}

bool WindowsPlatform::IsValid() const {
	return true;
}

/* -------------------------------------------------------------------- */
/** \name Window Managing
 * \{ */

WindowInterface *WindowsPlatform::InitWindow(WindowInterface *parent, int width, int height) {
	return nullptr;
}

void WindowsPlatform::CloseWindow(WindowInterface *window) {
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Context Managing
 * \{ */

ContextInterface *WindowsPlatform::InstallContext(WindowInterface *window, int type) {
	return nullptr;
}

ContextInterface *WindowsPlatform::GetContext(WindowInterface *window) {
	return nullptr;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name System Managing
 * \{ */

bool WindowsPlatform::ProcessEvents(bool wait) {
	return false;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Public Platform Functions
 * \{ */

int InitPlatform(void) {
	if ((glib_windows_platform = MEM_new<WindowsPlatform>("glib::WindowPlatform")) == nullptr) {
		return 0xf00;
	}

	if (!glib_windows_platform->IsValid()) {
		ClosePlatform();
		return 0xf01;
	}

	return 0;
}

void ClosePlatform() {
	if (glib_windows_platform) {
		MEM_delete<WindowsPlatform>(glib_windows_platform);
		glib_windows_platform = nullptr;
	}
}

/* \} */
