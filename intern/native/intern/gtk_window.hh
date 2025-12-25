#ifndef GTK_WINDOW_HH
#define GTK_WINDOW_HH

#include "GTK_backend_type.h"

#include <stddef.h>

class GTKManagerInterface;
class GTKRenderInterface;

struct FormatSetting {
	int rbits = -1;
	int gbits = -1;
	int bbits = -1;
	int abits = -1;

	int dbits = 8;
	int sbits = 8;

	int arbits = -1;
	int agbits = -1;
	int abbits = -1;
	int aabits = -1;

	int aux = 0;
	int samplers = 0;

	bool has_stereo = false;
	bool has_double_buffer = true;
	bool has_pixel_rgb = true;

	int handle = 0;
};

class GTKWindowInterface {
	GTKManagerInterface *manager = NULL;
	GTKRenderInterface *render = NULL;
	int backend = GTK_WINDOW_RENDER_NONE;
	
public:
	GTKWindowInterface(GTKManagerInterface *manager);
	virtual ~GTKWindowInterface();
	
	virtual bool Create(GTKWindowInterface *parent, const char *name, int width, int height, int state) = 0;
	virtual bool Install(int backend);

	virtual void Minimize() = 0;
	virtual void Maximize() = 0;
	virtual void Show() = 0;
	virtual void Hide() = 0;

	virtual int GetState(void) const = 0;
	virtual void GetPos(int *x, int *y) const = 0;
	virtual void GetSize(int *w, int *h) const = 0;
	
	GTKManagerInterface *GetManagerInterface();
	GTKRenderInterface *GetRenderInterface();
	
protected:
	virtual GTKRenderInterface *AllocateRender(int render) {
		return NULL;
	}
	
};

#endif // GTK_WINDOW_HH
