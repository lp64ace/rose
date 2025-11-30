#ifndef GTK_RENDER_HH
#define GTK_RENDER_HH

#include <stddef.h>

class GTKWindowInterface;

struct RenderSetting {
	/** Drawing buffer settings, used for creating the window context. */
	struct {
		int color_bits = 8;
		int depth_bits = 24;
		int stencil_bits = 8;
		int accum_bits = 8;

		bool srgb = false;
	};

	/** Drawing context capabilities and settings. */
	struct {
		int major = 4;
		int minor = 3;
	};
};

class GTKRenderInterface {
	GTKWindowInterface *window = NULL;
	
public:
	GTKRenderInterface(GTKWindowInterface *window);
	virtual ~GTKRenderInterface();
	
	virtual bool Create(RenderSetting setting) = 0;
	virtual bool MakeCurrent(void) = 0;
	virtual bool SwapBuffers(void) = 0;
	virtual bool SwapInterval(int interval) = 0;
	
	GTKWindowInterface *GetWindowInterface();
};

#endif // GTK_RENDER_HH
