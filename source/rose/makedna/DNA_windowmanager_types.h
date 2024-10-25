#ifndef DNA_WINDOWMANAGER_TYPES_H
#define DNA_WINDOWMANAGER_TYPES_H

#include "DNA_listbase.h"
#include "DNA_ID.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WindowManager {
	ID id;
	
	/** A list of all #wmWindow links that are allocated. */
	ListBase windows;
} WindowManager;

#ifdef __cplusplus
}
#endif

#endif	// DNA_WINDOWMANAGER_TYPES_H