#include "oswin.hh"

#include <chrono>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <thread>

namespace rose::tiny_window {

void tWindowManager::Shutdown() {
	while (this->windows_.size()) {
		auto &window = this->windows_.back();
		/** The window will be removed from the list here. */
		this->ShutdownWindow(window);
	}
}

double tWindowManager::GetElapsedTime() const {
	auto now = std::chrono::system_clock::now();

	return std::chrono::duration<double>(now - this->start_).count();
}

tWindow *tWindowManager::AddWindow(WindowSetting settings) {
	if (settings.name == nullptr) {
		return nullptr;
	}
	tWindow *window = new tWindow(settings);
	if (!window->Init(this)) {
		delete window;
		return nullptr;
	}
	this->windows_.push_back(window);
	return window;
}
void tWindowManager::ShutdownWindow(tWindow *window) {
	std::vector<tWindow *>::iterator itr;

	for (itr = this->windows_.begin(); itr != this->windows_.end(); itr++) {
		if (*itr == window) {
			break;
		}
	}

	if (itr == this->windows_.end()) {
		return;
	}

	this->windows_.erase(itr);
	delete window;
}

FormatSetting *tWindowManager::GetClosestFormat(const FormatSetting *desired) const {
	unsigned int absent, lowestAbsent = UINT_MAX;
	unsigned int colorDiff, lowestColorDiff = UINT_MAX;
	unsigned int extraDiff, lowestExtraDiff = UINT_MAX;
	FormatSetting *closest = nullptr;
	for (FormatSetting *format : formats_) {
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

int tWindowManager::GetGamepadCount(void) const {
	return this->gamepads_.size();
}

const Gamepad *tWindowManager::GetGamepad(int index) const {
	if (index < this->gamepads_.size()) {
		return &this->gamepads_[index];
	}
	return NULL;
}

void tWindowManager::Sleep(int ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

bool tWindow::ShouldClose() const {
	return this->should_close_;
}

}  // namespace rose::tiny_window
