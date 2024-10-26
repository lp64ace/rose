#ifndef WIN32_PLATFORM_HH
#define WIN32_PLATFORM_HH

#include "intern/platform.hh"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ghost {

struct Win32Platform : public PlatformBase {
	bool init() override;
	bool poll() override;
	
	~Win32Platform();
	
	WNDCLASSEXA wndclass_;
	
	static const char *WndClassName;
	static LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

} // namespace ghost

#endif // WIN32_PLATFORM_HH
