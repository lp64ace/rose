#ifndef GTK_WIN32_WINDOW_MANAGER_HH
#define GTK_WIN32_WINDOW_MANAGER_HH

#include "MEM_guardedalloc.h"

#include "intern/gtk_window_manager.hh"

#include "gtk_win32_window.hh"
#include "gtk_win32_render.hh"

#define NOMINMAX 1
#define WINDOWS_LEAN_AND_MEAN 1

#include <windows.h>
#include <xinput.h>
#include <gl/glew.h>
#include <gl/gl.h>
#include <gl/wglew.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "xinput.lib")

class GTKManagerWin32 final : public GTKManagerInterface {
	friend LRESULT CALLBACK WindowProcedure(HWND hwnd, unsigned int message, WPARAM wparam, LPARAM lparam);

	HKL keyboard_layout;

	std::vector<FormatSetting *> formats;

	struct {
		WNDCLASS windowclass;
		HWND window;
		HDC device;
		HGLRC render;
		HINSTANCE instance;
	} render_window_dummy;

public:
	PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = NULL;
	PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT = NULL;
	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = NULL;
	PFNWGLCHOOSEPIXELFORMATEXTPROC wglChoosePixelFormatEXT = NULL;
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;
	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;
	PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT = NULL;
	PFNWGLGETPIXELFORMATATTRIBFVARBPROC wglGetPixelFormatAttribfvARB = NULL;
	PFNWGLGETPIXELFORMATATTRIBFVEXTPROC wglGetPixelFormatAttribfvEXT = NULL;
	PFNWGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribivARB = NULL;
	PFNWGLGETPIXELFORMATATTRIBIVEXTPROC wglGetPixelFormatAttribivEXT = NULL;

public:
	GTKManagerWin32();
	~GTKManagerWin32();

	bool HasEvents();
	void Poll();

	bool SetClipboard(const char *buffer, unsigned int length, bool selection);
	bool GetClipboard(char **buffer, unsigned int *length, bool selection) const;

	GTKWindowWin32 *GetWindowByHandle(HWND hwnd);

	int GetLegacyPFD(const FormatSetting *format, HDC hdc);

private:
	bool InitKeyboardLayout();
	bool InitDummy();
	bool InitExtensions();

	void ExitExtensions();
	void ExitDummy();
	void ExitKeyboardLayout();

protected:
	inline GTKWindowInterface *AllocateWindow(GTKManagerInterface *manager) {
		return static_cast<GTKWindowInterface *>(MEM_new<GTKWindowWin32>("GTKWindowWin32", static_cast<GTKManagerWin32 *>(manager)));
	}
};

int ConvertInputKey(short win32_key, short scan_code, short extend);

#endif	// GTK_WIN32_WINDOW_MANAGER_HH
