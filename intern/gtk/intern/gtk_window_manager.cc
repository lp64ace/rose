#include "MEM_guardedalloc.h"

#include "gtk_window_manager.hh"
#include "gtk_window.hh"
#include "gtk_render.hh"

GTKManagerInterface::GTKManagerInterface() {
	this->t0 = std::chrono::system_clock::now();
}

GTKManagerInterface::~GTKManagerInterface() {
	while (!this->windows.empty()) {
		GTKWindowInterface *window = this->windows.back();
		this->DestroyWindow(window);
	}
	/** This is not actually needed but it will just exit immediatelly! (see #DestroyRender) */
	while (!this->renders.empty()) {
		GTKRenderInterface *render = this->renders.back();
		this->DestroyRender(render);
	}
}

double GTKManagerInterface::Time(void) const {
	auto now = std::chrono::system_clock::now();

	return std::chrono::duration<double>(now - this->t0).count();
}

GTKWindowInterface *GTKManagerInterface::NewWindowEx(GTKWindowInterface *parent, const char *title, int width, int height, int backend, int state) {
	GTKWindowInterface *newwindow = this->AllocateWindow(this);
	if (!newwindow || !newwindow->Create(parent, title, width, height, state) || !newwindow->Install(backend)) {
		MEM_delete(newwindow);
		return NULL;
	}
	this->windows.push_back(newwindow);
	return newwindow;
}

GTKWindowInterface *GTKManagerInterface::NewWindow(GTKWindowInterface *parent, const char *title, int width, int height, int backend) {
	return this->NewWindowEx(parent, title, width, height, backend, GTK_STATE_RESTORED);
}

GTKRenderInterface *GTKManagerInterface::NewRender(int backend) {
	GTKWindowInterface *newwindow = this->NewWindowEx(NULL, "Render", 1920, 1080, backend, GTK_STATE_HIDDEN);
	if (!newwindow) {
		return NULL;
	}
	this->renders.push_back(newwindow->GetRenderInterface());
	return newwindow->GetRenderInterface();
}

void GTKManagerInterface::DestroyWindow(GTKWindowInterface *window) {
	do {
		/** This should only happen when calling #DestroyRender! */
		std::vector<GTKRenderInterface *>::iterator itr = std::find(this->renders.begin(), this->renders.end(), window->GetRenderInterface());
		if (itr != this->renders.end()) {
			this->renders.erase(itr);
			this->FreeRender(window->GetRenderInterface());
		}
	} while(false);
	do {
		std::vector<GTKWindowInterface *>::iterator itr = std::find(this->windows.begin(), this->windows.end(), window);
		if (itr != this->windows.end()) {
			this->windows.erase(itr);
			this->FreeWindow(window);
		}
	} while(false);
}

void GTKManagerInterface::DestroyRender(GTKRenderInterface *render) {
	this->DestroyWindow(render->GetWindowInterface());
}

const std::vector<GTKWindowInterface *> &GTKManagerInterface::GetWindows() const {
	return this->windows;
}

void GTKManagerInterface::FreeWindow(GTKWindowInterface *window) {
	MEM_delete<GTKWindowInterface>(window);
}

void GTKManagerInterface::FreeRender(GTKRenderInterface *render) {
	GTKWindowInterface *window = render->GetWindowInterface();
	if (window) {
		window->Install(GTK_WINDOW_RENDER_NONE);
	}
}

FormatSetting *GTKManagerInterface::GetClosestFormat(const std::vector<FormatSetting *> &list, const FormatSetting *desired) {
	unsigned int absent, lowestAbsent = UINT_MAX;
	unsigned int colorDiff, lowestColorDiff = UINT_MAX;
	unsigned int extraDiff, lowestExtraDiff = UINT_MAX;
	FormatSetting *closest = nullptr;

	for (FormatSetting *format : list) {
		if (desired->has_stereo && !format->has_stereo) {
			continue;
		}
		if (desired->has_double_buffer && !format->has_double_buffer) {
			continue;
		}

		absent = 0;
		if (desired->abits && !format->abits) {
			absent++;
		}
		if (desired->dbits && !format->dbits) {
			absent++;
		}
		if (desired->sbits && !format->sbits) {
			absent++;
		}
		if (desired->aux && format->aux < desired->aux) {
			absent += desired->aux - format->aux;
		}
		if (desired->samplers && format->samplers < desired->samplers) {
			absent += desired->samplers - format->samplers;
		}

		colorDiff = 0;
		if (desired->rbits != -1) {
			colorDiff += pow(desired->rbits - format->rbits, 2);
		}
		if (desired->gbits != -1) {
			colorDiff += pow(desired->gbits - format->gbits, 2);
		}
		if (desired->bbits != -1) {
			colorDiff += pow(desired->bbits - format->bbits, 2);
		}

		extraDiff = 0;
		if (desired->abits != -1) {
			colorDiff += pow(desired->abits - format->abits, 2);
		}
		if (desired->dbits != -1) {
			colorDiff += pow(desired->dbits - format->dbits, 2);
		}
		if (desired->sbits != -1) {
			colorDiff += pow(desired->sbits - format->sbits, 2);
		}
		if (desired->arbits != -1) {
			colorDiff += pow(desired->arbits - format->arbits, 2);
		}
		if (desired->agbits != -1) {
			colorDiff += pow(desired->agbits - format->agbits, 2);
		}
		if (desired->abbits != -1) {
			colorDiff += pow(desired->abbits - format->abbits, 2);
		}
		if (desired->aabits != -1) {
			colorDiff += pow(desired->aabits - format->aabits, 2);
		}
		if (desired->aux != -1) {
			colorDiff += pow(desired->aux - format->aux, 2);
		}
		if (desired->samplers != -1) {
			colorDiff += pow(desired->samplers - format->samplers, 2);
		}

		if (desired->has_pixel_rgb && !format->has_pixel_rgb) {
			extraDiff++;
		}

		if (absent < lowestAbsent) {
			closest = format;
		}
		else if (absent == lowestAbsent) {
			if ((colorDiff < lowestColorDiff) || (colorDiff == lowestColorDiff && extraDiff < lowestExtraDiff)) {
				closest = format;
			}
		}

		if (closest == format) {
			lowestAbsent = absent;
			lowestColorDiff = colorDiff;
			lowestExtraDiff = extraDiff;
		}
	}

	return closest;
}
