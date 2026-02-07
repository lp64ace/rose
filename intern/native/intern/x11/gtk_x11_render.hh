#ifndef GTK_X11_RENDER_HH
#define GTK_X11_RENDER_HH

#include "intern/gtk_render.hh"

#include <X11/X.h>
#include <X11/Xlib.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>

class GTKWindowX11;
class GTKManagerX11;

class GTKRenderXGL final : public GTKRenderInterface {
	GLXContext context;
	
	XVisualInfo *visual_info;
	
public:
	GTKRenderXGL(GTKWindowX11 *window);
	~GTKRenderXGL();

	bool Create(RenderSetting setting);
	bool MakeCurrent(void);
	bool SwapBuffers(void);
	bool SwapInterval(int interval);

	static GLXFBConfig XPixelConfigInit(GTKManagerX11 *manager, RenderSetting setting);
	
};

#endif
