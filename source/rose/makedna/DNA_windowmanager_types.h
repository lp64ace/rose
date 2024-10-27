#ifndef DNA_WINDOWMANAGER_TYPES_H
#define DNA_WINDOWMANAGER_TYPES_H

#include "DNA_listbase.h"
#include "DNA_ID.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wmWindow {
	struct wmWindow *prev, *next;
	
	/**
	 * This is the handle of the backend system that we are using, 
	 * this handle does not belong to us but is the linking part between TinyWindow and WindowManager.
	 */
	void *handle;
	void *context;
	
	struct wmWindow *parent;
	
	int posx;
	int posy;
	int sizex;
	int sizey;
} wmWindow;

typedef struct WindowManager {
	ID id;
	
	struct wmWindow *windrawable;
	
	/**
	 * This is the handle of the backend system that we are using, 
	 * this handle does not belong to us but is the linking part between TinyWindow and WindowManager.
	 */
	void *handle;
	
	/** A list of all #wmWindow links that are allocated. */
	ListBase windows;
} WindowManager;

#ifdef __cplusplus
}
#endif

#endif	// DNA_WINDOWMANAGER_TYPES_H
