#ifndef DNA_VIEW3D_TYPES_H
#define DNA_VIEW3D_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RegionView3D {
	/** GL_PROJECTION matrix. */
	float winmat[4][4];
	/** GL_MODELVIEW matrix. */
	float viewmat[4][4];
	
	/** View rotation, must be kept normalized. */
	float viewquat[4];
	float viewloc[3];
} RegionView3D;

#ifdef __cplusplus
}
#endif

#endif // DNA_VIEW3D_TYPES_H
