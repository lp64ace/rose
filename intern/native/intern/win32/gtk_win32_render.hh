#ifndef GTK_WIN32_RENDER_HH
#define GTK_WIN32_RENDER_HH

#include "intern/gtk_render.hh"

#define NOMINMAX 1
#define WINDOWS_LEAN_AND_MEAN 1

#include <windows.h>
#include <gl/glew.h>
#include <gl/wglew.h>

class GTKWindowWin32;
class GTKManagerWin32;

class GTKRenderWGL final : public GTKRenderInterface {
	HGLRC context;

public:
	GTKRenderWGL(GTKWindowWin32 *window);
	~GTKRenderWGL();

	bool Create(RenderSetting setting);
	bool MakeCurrent(void);
	bool SwapBuffers(void);
	bool SwapInterval(int interval);

protected:
	bool InitPixelFormat(GTKManagerWin32 *manager, GTKWindowWin32 *window, RenderSetting setting);
};

#endif
