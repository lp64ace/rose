#pragma once

#include "DNA_ID.h"
#include "DNA_listbase.h"

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

	int posx;
	int posy;
	int width;
	int height;

	struct GWindow *gwin;
	struct GPUContext *gpuctx;

	struct wmWindow *parent;
} wmWindow;

#ifdef __cplusplus
}
#endif
