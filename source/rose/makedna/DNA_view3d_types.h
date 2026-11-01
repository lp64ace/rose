#ifndef DNA_VIEW3D_TYPES_H
#define DNA_VIEW3D_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RegionView3D {
	/** GL_PROJECTION matrix. */
	float winmat[4][4];
	float wininv[4][4];
	/** GL_MODELVIEW matrix. */
	float viewmat[4][4];
	float viewinv[4][4];
	/** GL_PROJECTION * GL_MODELVIEW matrix */
	float persmat[4][4];
	float persinv[4][4];
	
	/** View rotation, must be kept normalized. */
	float viewquat[4];
	float viewloc[3];
} RegionView3D;

enum {
	V3D_ORIENT_GLOBAL = 0,
	V3D_ORIENT_LOCAL = 1,
	V3D_ORIENT_VIEW = 2,
};

enum {
	/* center of the bounding box */
	V3D_AROUND_CENTER_BOUNDS = 0,
};

#ifdef __cplusplus
}
#endif

#endif // DNA_VIEW3D_TYPES_H
