#include "MEM_guardedalloc.h"

#include "gtk_render.hh"
#include "gtk_window.hh"

GTKWindowInterface::GTKWindowInterface(GTKManagerInterface *manager) : manager(manager) {
}

GTKWindowInterface::~GTKWindowInterface() {
	this->Install(GTK_WINDOW_RENDER_NONE);
}

bool GTKWindowInterface::Install(int backend) {
	if (this->backend != backend) {
		GTKRenderInterface *newrender = this->AllocateRender(backend);
		
		RenderSetting setting = RenderSetting();

		if (newrender != NULL && !newrender->Create(setting)) {
			MEM_delete<GTKRenderInterface>(this->render);
			this->render = NULL;
		}

		if (newrender != NULL || backend == GTK_WINDOW_RENDER_NONE) {
			MEM_delete<GTKRenderInterface>(this->render);
			
			this->render = newrender;
			this->backend = backend;
		}
		else {
			/**
			 * We did not manager to allocate a new render with the specified backend.
			 */
			return false;
		}
	}
	
	return true;
}

void GTKWindowInterface::SetCursorVisible(bool visible) {
	this->cursor_visible = visible;
}

void GTKWindowInterface::SetCursorCustomShape(const char *bitmap, const char *mask, int width, int height, int x, int y) {
	this->UpdateCursorCustomShape(bitmap, mask, width, height, x, y);
}

void GTKWindowInterface::SetCursorShape(int cursor) {
	this->cursor_shape = cursor;
}

int GTKWindowInterface::GetCursorShape(void) {
	return this->cursor_shape;
}

bool GTKWindowInterface::GetCursorVisibility(void) {
	return this->cursor_visible;
}

GTKManagerInterface *GTKWindowInterface::GetManagerInterface() {
	return this->manager;
}

GTKRenderInterface *GTKWindowInterface::GetRenderInterface() {
	return this->render;
}
