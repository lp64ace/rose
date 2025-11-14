#ifndef GTK_X11_WINDOW_HH
#define GTK_X11_WINDOW_HH

#include "intern/gtk_window.hh"

#include <X11/Xlib.h>

class GTKWindowX11 : public GTKWindowInterface {
    Display *display;
    Window window;
	
	int state;
	
	XVisualInfo *visual_info;
	
public:
	GTKWindowX11(GTKManagerX11 *manager);
	~GTKWindowX11();

	bool Create(GTKWindowInterface *parent, const char *name, int width, int height, int state);

	void Minimize();
	void Maximize();
	void Show();
	void Hide();

	int GetState(void) const;
	void GetPos(int *x, int *y) const;
	void GetSize(int *w, int *h) const;

	Window GetHandle();

public:
	/** These atoms are needed to change window states via the extended window manager */
	Atom AtomState;			   /**< Atom for the state of the window */
	Atom AtomHidden;		   /**< Atom for the current hidden state of the window */
	Atom AtomFullScreen;	   /**< Atom for the full screen state of the window */
	Atom AtomMaxHorz;		   /**< Atom for the maximized horizontally state of the window */
	Atom AtomMaxVert;		   /**< Atom for the maximized vertically state of the window */
	Atom AtomClose;			   /**< Atom for closing the window */
	Atom AtomActive;		   /**< Atom for the active window */
	Atom AtomDemandsAttention; /**< Atom for when the window demands attention */
	Atom AtomFocused;		   /**< Atom for the focused state of the window */
	Atom AtomCardinal;		   /**< Atom for cardinal coordinates */
	Atom AtomIcon;			   /**< Atom for the icon of the window */
	Atom AtomHints;			   /**< Atom for the window decorations */

	Atom AtomWindowType;		/**< Atom for the type of window */
	Atom AtomWindowTypeDesktop; /**< Atom for the desktop window type */
	Atom AtomWindowTypeSplash;	/**< Atom for the splash screen window type */
	Atom AtomWindowTypeNormal;	/**< Atom for the normal splash screen window type */

	Atom AtomAllowedActions;	 /**< Atom for allowed window actions */
	Atom AtomActionResize;		 /**< Atom for allowing the window to be resized */
	Atom AtomActionMinimize;	 /**< Atom for allowing the window to be minimized */
	Atom AtomActionShade;		 /**< Atom for allowing the window to be shaded */
	Atom AtomActionMaximizeHorz; /**< Atom for allowing the window to be maximized horizontally */
	Atom AtomActionMaximizeVert; /**< Atom for allowing the window to be maximized vertically */
	Atom AtomActionClose;		 /**< Atom for allowing the window to be closed */

	Atom AtomDesktopGeometry; /**< Atom for Desktop Geometry */

#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
	XIC xic;

	bool InitXIC(GTKManagerX11 *manager);
#endif

protected:
	GTKRenderInterface *AllocateRender(int render) {
		return NULL;
	}

    friend class GTKManagerX11;
};

#endif
