#include "MEM_guardedalloc.h"

#include "win32_display.hh"

namespace ghost {

bool Win32DisplayManager::init() {
	return true;
}

int Win32DisplayManager::GetNumDisplays(void) const {
	return (int)::GetSystemMetrics(SM_CMONITORS);
}
int Win32DisplayManager::GetNumDisplaySettings(int display) const {
	DISPLAY_DEVICE device = {};
	device.cb = sizeof(DISPLAY_DEVICE);
	if(!::EnumDisplayDevices(nullptr, (DWORD)display, &device, 0)) {
		return 0;
	}
	
	int count = 0;
	
	DEVMODE mode = {};
	while(::EnumDisplaySettings(device.DeviceName, (DWORD)count, &mode)) {
		count++;
	}
	return count;
}
bool Win32DisplayManager::GetDisplaySetting(int display, int setting, WTKDisplaySetting *info) {
	DISPLAY_DEVICE device = {};
	device.cb = sizeof(DISPLAY_DEVICE);
	if(!::EnumDisplayDevices(nullptr, (DWORD)display, &device, 0)) {
		return false;
	}
	
	DEVMODE mode = {};
	if(!::EnumDisplaySettings(device.DeviceName, (DWORD)setting, &mode)) {
		return false;
	}
	
	info->xpixels = mode.dmPelsWidth;
	info->ypixels = mode.dmPelsHeight;
	info->bpp = mode.dmBitsPerPel;
	info->framerate = mode.dmDisplayFrequency;
	return true;
}

bool Win32DisplayManager::GetCurrentDisplaySetting(int display, WTKDisplaySetting *info) {
	return GetDisplaySetting(display, ENUM_CURRENT_SETTINGS, info);
}
bool Win32DisplayManager::SetCurrentDisplaySetting(int display, WTKDisplaySetting *info) {
	DISPLAY_DEVICE device = {};
	device.cb = sizeof(DISPLAY_DEVICE);
	if(!::EnumDisplayDevices(nullptr, (DWORD)display, &device, 0)) {
		return false;
	}
	
	int setting = 0;
	
	DEVMODE mode = {};
	while(::EnumDisplaySettings(device.DeviceName, (DWORD)setting++, &mode)) {
		if(mode.dmPelsWidth != info->xpixels && mode.dmPelsHeight != info->ypixels) {
			/** Resolution did not match, look for a different mode. */
			continue;
		}
		if(mode.dmBitsPerPel != info->bpp) {
			continue;
		}
		if(mode.dmDisplayFrequency != info->framerate) {
			/** Frequency did not match, look for a different mode. */
			continue;
		}
		
		LONG status = ::ChangeDisplaySettings(&mode, CDS_FULLSCREEN);
		if(status == DISP_CHANGE_SUCCESSFUL) {
			/** Failed to change, look for a different mode. */
			return true;
		}
	}
	
	return false;
}

} // namespace ghost
