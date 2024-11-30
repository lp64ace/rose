#ifndef TINY_WINDOW_HH
#define TINY_WINDOW_HH

#include "MEM_guardedalloc.h"

#include <chrono>
#include <functional>
#include <string>
#include <vector>

#if defined(WIN32)
#	define NOMINMAX 1
#	define WINDOWS_LEAN_AND_MEAN 1
#	include <windows.h>
#	include <Xinput.h>
#	pragma comment(lib, "winmm.lib")
#	pragma comment(lib, "xinput.lib")
#	include <GL/glew.h>
#	include <GL/gl.h>
#	include <GL/wglew.h>
#endif

#if defined(LINUX)
#	include <X11/X.h>
#	include <X11/Xlib.h>
#	include <X11/XKBlib.h>
#	include <X11/Xatom.h>
#	include <X11/keysym.h>
#	include <X11/Xutil.h>
#	include <X11/Xresource.h>
#	include <X11/Xlocale.h>
/** OpenGL include files */
#	include <GL/glew.h>
#	include <GL/gl.h>
#	include <GL/glx.h>
#	include <GL/glxext.h>
#endif

#ifdef WITH_X11_XINPUT
/** I fucking hate linux GUI application <3 */
#	include <X11/extensions/XInput2.h>
#	define USE_XINPUT_HOTPLUG
#endif

#ifdef __cplusplus
/* Useful to port C code using enums to C++ where enums are strongly typed.
 * To use after the enum declaration. */
/* If any enumerator `C` is set to say `A|B`, then `C` would be the max enum value. */
#	define ENUM_OPERATORS(_enum_type, _max_enum_value)                              \
		extern "C++" {                                                               \
		inline constexpr _enum_type operator|(_enum_type a, _enum_type b) {          \
			return (_enum_type)(uint64_t(a) | uint64_t(b));                          \
		}                                                                            \
		inline constexpr _enum_type operator&(_enum_type a, _enum_type b) {          \
			return (_enum_type)(uint64_t(a) & uint64_t(b));                          \
		}                                                                            \
		inline constexpr _enum_type operator~(_enum_type a) {                        \
			return (_enum_type)(~uint64_t(a) & (2 * uint64_t(_max_enum_value) - 1)); \
		}                                                                            \
		inline _enum_type &operator|=(_enum_type &a, _enum_type b) {                 \
			return a = (_enum_type)(uint64_t(a) | uint64_t(b));                      \
		}                                                                            \
		inline _enum_type &operator&=(_enum_type &a, _enum_type b) {                 \
			return a = (_enum_type)(uint64_t(a) & uint64_t(b));                      \
		}                                                                            \
		inline _enum_type &operator^=(_enum_type &a, _enum_type b) {                 \
			return a = (_enum_type)(uint64_t(a) ^ uint64_t(b));                      \
		}                                                                            \
		} /* extern "C++" */
#else
/* Output nothing. */
#	define ENUM_OPERATORS(_type, _max)
#endif

namespace rose::tiny_window {

class tWindowManager;
class tWindow;

struct WindowSetting {
	/** The name for the window, this has to be a static string. */
	const char *name = nullptr;

	/** Drawing buffer settings, used for creating the window context. */
	struct {
		int color_bits = 8;
		int depth_bits = 24;
		int stencil_bits = 8;
		int accum_bits = 8;

		bool srgb = false;
	};

	/** The width in pixels of the desired window's client area. */
	int width = 800;
	/** The height in pixels of the desired window's client area. */
	int height = 450;

	/** Drawing context capabilities and settings. */
	struct {
		int major = 4;
		int minor = 3;
	};
};

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

struct Gamepad {
	enum Button {
		face_top = 0,
		face_left,
		face_right,
		face_bottom,
		start,
		select,
		dpad_top,
		dpad_left,
		dpad_right,
		dpad_bottom,
		left_stick,
		right_stick,
		left_shoulder,
		right_shoulder,
		special1,
		special2,
		last = special2,
	};

	struct {
		float left = 0.0f;
		float right = 0.0f;
	} triggers;

	float lstick[2] = {0.5f, 0.5f};
	float rstick[2] = {0.5f, 0.5f};
	bool buttons[Button::last + 1] = {};

	bool wireless = false;
};

enum class State {
	normal,
	maximized,
	minimized,
	fullscreen,
};

using MoveEventFn = std::function<void(tWindow *window, int x, int y)>;
using ResizeEventFn = std::function<void(tWindow *window, unsigned int cwidth, unsigned int cheight)>;
using ActivateEventFn = std::function<void(tWindow *window, bool activate)>;
using MouseEventFn = std::function<void(tWindow *window, int x, int y, double time)>;
using WheelEventFn = std::function<void(tWindow *window, int dx, int dy, double time)>;
using ButtonDownEventFn = std::function<void(tWindow *window, int key, int x, int y, double time)>;
using ButtonUpEventFn = std::function<void(tWindow *window, int key, int x, int y, double time)>;
using KeyDownEventFn = std::function<void(tWindow *window, int key, bool repeat, char utf8[4], double time)>;
using KeyUpEventFn = std::function<void(tWindow *window, int key, double time)>;
using DestroyEventFn = std::function<void(tWindow *window)>;

struct MonitorSetting {
	int sizex = 0;
	int sizey = 0;
	int bpp = 0;
	int frequency = 0;
};

struct tMonitor {
	MonitorSetting *current = NULL;

	std::string device;
	std::string monitor;
	std::string name;

	bool primary = false;

	std::vector<MonitorSetting *> settings;

#if defined(WIN32)
	HMONITOR monitor_handle_ = NULL;
#endif
};

class tWindowManager {
	MEM_CXX_CLASS_ALLOC_FUNCS("tWindowManager");
	friend class tWindow;

private:
	/** In order to create a new tWindowManager call #Init instead! */
	tWindowManager();

public:
	/** Attempt to create a new window manager with the optional specified application name. */
	static tWindowManager *Init(const char *application);

	/**
	 * Shutdown the window manager and completely free all the allocated resources.
	 * This method does not deallocate the #tWindowManager itself.
	 */
	void Shutdown();
	bool HasEventsWaiting();
	void Poll();

	double GetElapsedTime() const;

	/** Create and append a new window to our window management list. */
	tWindow *AddWindow(WindowSetting settings);
	/** This will destroy the specified window and remove it from our window management list. */
	void ShutdownWindow(tWindow *window);

	FormatSetting *GetClosestFormat(const FormatSetting *desired) const;

	int GetGamepadCount(void) const;
	const Gamepad *GetGamepad(int index) const;

	bool SetClipboard(const char *buffer, unsigned int length, bool selection) const;
	bool GetClipboard(char **buffer, unsigned int *length, bool selection) const;

	static void Sleep(int ms);

private:
	std::vector<tWindow *> windows_;
	std::vector<tMonitor *> monitors_;
	std::vector<FormatSetting *> formats_;

	std::chrono::high_resolution_clock::time_point start_;

	std::vector<Gamepad> gamepads_;

#if defined(WIN32)
	struct {
		WNDCLASS window_class_ = {};
		HWND window_handle_ = nullptr;
		HDC device_handle_ = nullptr;
		HGLRC rendering_handle_ = nullptr;
		HINSTANCE instance_handle_ = nullptr;
	} dummy;

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

	bool has_swap_interval_control_ = false;
	bool has_srgb_framebuffer_support_ = false;

	HKL keyboard_layout_ = {};

	static LRESULT CALLBACK WindowProcedure(HWND windowHandle, unsigned int winMessage, WPARAM wordParam, LPARAM longParam);
#endif
#if defined(LINUX)
	Display *display_ = nullptr;
	XkbDescRec *xkb_descr_ = nullptr;

	int keycode_last_repeat_key_ = -1;

	PFNGLXCREATECONTEXTATTRIBSARBPROC glxCreateContextAttribsARB;
	PFNGLXMAKECONTEXTCURRENTPROC glxMakeContextCurrent;
	PFNGLXSWAPINTERVALEXTPROC glxSwapIntervalEXT;
	PFNGLXCHOOSEFBCONFIGPROC glxChooseFBConfig;

	void EventProcedure(XEvent *evt);
#endif
#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
	XIM xim_ = {};

	bool InitXIM();
#endif

public:
	MoveEventFn MoveEvent;
	ResizeEventFn ResizeEvent;
	ActivateEventFn ActivateEvent;
	MouseEventFn MouseEvent;
	WheelEventFn WheelEvent;
	ButtonDownEventFn ButtonDownEvent;
	ButtonUpEventFn ButtonUpEvent;
	KeyDownEventFn KeyDownEvent;
	KeyUpEventFn KeyUpEvent;
	DestroyEventFn DestroyEvent;

private:
#if defined(WIN32)
	tWindow *GetWindowByHandle(HWND handle) const;

	int GetLegacyPFD(const FormatSetting *desired, HDC hdc);
	int FillGamepad(XINPUT_STATE state, int index);
	int PollGamepads();

	bool GetScreenInfo();
	bool InitKeyboardLayout();
	bool InitDummy();
	bool InitExtensions();
	bool InitGamepad();

	void ClearGamepad();
	void ClearExtensions();
	void ClearDummy();
	void ClearKeyboardLayout();
	void ClearScreenInfo();

	bool ExtensionSupported(const char *extensionName);
#endif
#if defined(LINUX)
	tWindow *GetWindowByHandle(Window handle) const;

	bool InitExtensions();
	bool InitKeyboardDescription();

	void ClearKeyboardDescription();
	void ClearExtensions();
#endif

public:
	/** We do not use virtual functions since they introduce unwanted overhead! */
	~tWindowManager();
};

enum class Decorator {
	none = 0,
	titlebar = 1 << 0,
	icon = 1 << 1,
	border = 1 << 2,
	minimize_btn = 1 << 3,
	maximize_btn = 1 << 4,
	destroy_btn = 1 << 5,
	sizeable = 1 << 6,
};

ENUM_OPERATORS(Decorator, Decorator::sizeable);

#if defined(LINUX)

enum class LinuxDecorator {
	none = 0,
	border = 1 << 0,
	move = 1 << 1,
	minimize = 1 << 2,
	maximize = 1 << 3,
	close = 1 << 4,
};

ENUM_OPERATORS(LinuxDecorator, LinuxDecorator::close);

enum class Hint {
	function = (1 << 0),
	decorator = (1 << 1),
};

ENUM_OPERATORS(Hint, Hint::decorator);

#endif

enum class Style {
	bare,
	normal,
	popup,
};

class tWindow {
	MEM_CXX_CLASS_ALLOC_FUNCS("tWindow");
	friend class tWindowManager;

private:
	tWindow(WindowSetting settings);
	/** Use #tWindowManager->ShudownWindow instead! */
	~tWindow();

	/** Attempt to create a new window with the specified window settings. */
	bool Init(tWindowManager *manager);
	bool InitContext(tWindowManager *manager);
#if defined(WIN32)
	bool InitPixelFormat();
#endif
#if defined(LINUX)
	bool InitPixelConfig(tWindowManager *manager);
	bool InitAtoms();
#endif

public:
	void SetWindowStyle(Style style);
	void SetWindowDecoration(Decorator decoration, bool add);
	void SetTitleBar(const char *title);
	State GetState() const;

	void Show();
	void Hide();

	bool MakeContextCurrent();
	bool SetSwapInterval(int interval);
	bool GetSwapInterval(int *interval);
	bool SwapBuffers();

	bool ShouldClose() const;

public:
	int posx = 0;
	int posy = 0;
	int sizex = 0;
	int sizey = 0;
	int clientx = 0;
	int clienty = 0;

private:
	WindowSetting settings;

	/** A flag indicating that the window should close, set when the close button is pressed. */
	bool should_close_ = false;
	/** A flag indicating that the window has a valid context, set when the context is created. */
	bool context_created_ = false;
	/** A flag indicating that the window has a current context, set when the context is made current. */
	bool context_current_ = false;
	bool active_;

	State state_ = State::normal;

#if defined(WIN32)
	HWND window_handle_ = nullptr;
	HDC device_handle_ = nullptr;
	HGLRC rendering_handle_ = nullptr;
	HPALETTE palette_handle_ = nullptr;
	HINSTANCE instance_handle_ = nullptr;

	PIXELFORMATDESCRIPTOR pixel_descriptor_ = {};
	WNDCLASS window_class_ = {};
#endif
#if defined(LINUX)
	Display *display_ = nullptr;
	Window window_handle_ = {};
	GLXContext rendering_handle_ = {};
	GLXFBConfig pixel_config_ = {};

	LinuxDecorator linux_decorations_ = LinuxDecorator::none;
	XVisualInfo *visual_info_ = nullptr;
	XSetWindowAttributes window_attributes_ = {};

	int *visual_attribs_ = nullptr;

	char *clipboard_[2] = {nullptr, nullptr};

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
#endif
#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
	XIC xic_;

	bool InitXIC(tWindowManager *manager);
#endif
};

}  // namespace rose::tiny_window

#endif	// TINY_WINDOW_HH
