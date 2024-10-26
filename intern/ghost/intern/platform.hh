#ifndef GHOST_PLATFORM_HH
#define GHOST_PLATFORM_HH

namespace ghost {

struct DisplayManagerBase;
struct WindowManagerBase;

struct PlatformBase {
	virtual bool init() = 0;
	virtual bool poll() = 0;
	
	struct DisplayManagerBase *display_mngr_ = NULL;
	struct WindowManagerBase *window_mngr_ = NULL;
	
	/** Ensure that any derived properties are freed as well. */
	virtual ~PlatformBase() = default;
};

PlatformBase *InitMostSuitablePlatform(void);

} // namespace ghost

#endif // GHOST_PLATFORM_HH