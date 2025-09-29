#include "gtk_render.hh"

GTKRenderInterface::GTKRenderInterface(GTKWindowInterface *window) : window(window) {
}

GTKRenderInterface::~GTKRenderInterface() {
	this->window = NULL;
}

GTKWindowInterface *GTKRenderInterface::GetWindowInterface() {
	return this->window;
}
