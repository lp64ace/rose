#ifndef GTK_WIN32_WINDOW_HH
#define GTK_WIN32_WINDOW_HH

#include "intern/gtk_window.hh"

#define NOMINMAX 1
#define WINDOWS_LEAN_AND_MEAN 1

#include <windows.h>

class GTKManagerWin32;

class GTKWindowWin32 final : public GTKWindowInterface {
	friend LRESULT CALLBACK WindowProcedure(HWND hwnd, unsigned int message, WPARAM wparam, LPARAM lparam);

	WNDCLASS windowclass;
	HWND hwnd;
	HDC device;
	HCURSOR custom_cursor;

	int state;

public:
	GTKWindowWin32(GTKManagerWin32 *manager);
	~GTKWindowWin32();

	bool Create(GTKWindowInterface *parent, const char *name, int width, int height, int state);

	void Minimize();
	void Maximize();
	void Show();
	void Hide();

	void UpdateCursorCustomShape(const char *bitmap, const char *mask, int width, int height, int x, int y);
	void UpdateCursor(bool visible, int cursor);

	int GetState(void) const;
	void GetPos(int *x, int *y) const;
	void GetSize(int *w, int *h) const;

	HWND GetHandle();
	HDC GetDeviceHandle();

	void ThemeRefresh();

protected:
	GTKRenderInterface *AllocateRender(int render);
};

#endif
