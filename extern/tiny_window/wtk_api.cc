#include "wtk_api.h"

#include "tiny_window.hh"

WTKWindowManager *WTK_window_manager_new(void) {
	TinyWindow::windowManager *manager =  new TinyWindow::windowManager();
	return reinterpret_cast<WTKWindowManager *>(manager);
}

void WTK_window_manager_free(WTKWindowManager *vmanager) {
	TinyWindow::windowManager *manager = reinterpret_cast<TinyWindow::windowManager *>(vmanager);
	if(manager) {
		manager->ShutDown();
		delete manager;
	}
}
void WTK_window_manager_poll(WTKWindowManager *vmanager) {
	TinyWindow::windowManager *manager = reinterpret_cast<TinyWindow::windowManager *>(vmanager);
	manager->PollForEvents();
}

WTKWindow *WTK_create_window(WTKWindowManager *vmanager, const char *title, int width, int height) {
	TinyWindow::windowManager *manager = reinterpret_cast<TinyWindow::windowManager *>(vmanager);
	TinyWindow::windowSetting_t settings;
	settings.name = title;
	settings.resolution = TinyWindow::vec2_t<unsigned int>(width, height);
	settings.SetProfile(TinyWindow::profile_t::core);
	settings.currentState = TinyWindow::state_t::normal;
	settings.enableSRGB = true;
	
	return reinterpret_cast<WTKWindow *>(manager->AddWindow(settings));
}

bool WTK_window_should_close(WTKWindow *vwindow) {
	TinyWindow::tWindow *window = reinterpret_cast<TinyWindow::tWindow *>(vwindow);
	return window->shouldClose;
}

void WTK_window_free(WTKWindowManager *vmanager, WTKWindow *vwindow) {
	TinyWindow::windowManager *manager = reinterpret_cast<TinyWindow::windowManager *>(vmanager);
	TinyWindow::tWindow *window = reinterpret_cast<TinyWindow::tWindow *>(vwindow);
	manager->RemoveWindow(window);
	delete window;
}
