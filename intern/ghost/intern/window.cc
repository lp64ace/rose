#include "MEM_guardedalloc.h"

#include "window.hh"

namespace ghost {

bool WindowManagerBase::Close(struct WindowBase *window) {
	std::vector<WindowBase *>::iterator itr = std::find(this->windows_.begin(), this->windows_.end(), window);
	if (itr == this->windows_.end()) {
		return false;
	}

	this->windows_.erase(itr);

	MEM_delete<WindowBase>(window);
	return true;
}
bool WindowManagerBase::IsValid(struct WindowBase *window) {
	std::vector<WindowBase *>::iterator itr = std::find(this->windows_.begin(), this->windows_.end(), window);
	if (itr == this->windows_.end()) {
		return false;
	}
	return true;
}

WindowManagerBase::~WindowManagerBase() {
	while (!this->windows_.empty()) {
		WindowBase *window = this->windows_.back();

		this->windows_.pop_back();

		MEM_delete<WindowBase>(window);
	}
}

}  // namespace ghost
