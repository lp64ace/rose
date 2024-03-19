#pragma once

#include "DNA_ID.h"
#include "DNA_listbase.h"

#include "DNA_screen_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wmWindowManager {
	ID id;

	struct wmWindow *winactive;
	struct wmWindow *drawable;

	ListBase windows;
} wmWindowManager;

typedef struct wmWindow {
	struct wmWindow *prev, *next;

	struct GWindow *gwin;
	struct GPUContext *gpuctx;

	struct wmWindow *parent;
	
	struct WorkSpaceInstanceHook *workspace_hook;
	
	/** Global areas aren't part of the screen, but part of the widow directly. */
	ScrAreaMap global_areas;
	
	int winid;
	
	int posx;
	int posy;
	int width;
	int height;
} wmWindow;

#ifdef __cplusplus
}
#endif
