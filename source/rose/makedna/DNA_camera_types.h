#ifndef DNA_CAMERA_TYPES_H
#define DNA_CAMERA_TYPES_H

#include "DNA_ID.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Camera {
	ID id;
	/** Animation data (must be immediately after id for utilities to use it). */
	struct AnimData *adt;

	int type;
} Camera;

/* #Camera->type */
enum {
	CAM_PERSP = 0,
	CAM_ORTHO = 1,
};

#ifdef __cplusplus
}
#endif

#endif	// DNA_CAMERA_TYPES_H