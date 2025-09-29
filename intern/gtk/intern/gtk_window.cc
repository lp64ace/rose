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
		
		RenderSetting default = RenderSetting();

		if (newrender != NULL && !newrender->Create(default)) {
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

GTKManagerInterface *GTKWindowInterface::GetManagerInterface() {
	return this->manager;
}

GTKRenderInterface *GTKWindowInterface::GetRenderInterface() {
	return this->render;
}
