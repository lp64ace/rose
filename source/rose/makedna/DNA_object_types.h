#ifndef DNA_OBJECT_TYPES_H
#define DNA_OBJECT_TYPES_H

#include "DNA_ID.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The following illustates the orientation of the bounding box in local space.
 *
 * Z  Y
 * | /
 * |/
 * .------X
 *
 *    2----------6
 *   /|         /|
 *  / |        / |
 * 1----------5  |
 * |  |       |  |
 * |  3-------|--7
 * | /        | /
 * |/         |/
 * 0----------4
 */
typedef struct BoundBox {
	float vec[8][3];
	int flag;
} BoundBox;

/** #BoundBox->flag */
enum {
	BOUNDBOX_DIRTY = 1 << 0,
};

typedef struct ObjectRuntime {
	/** Axis aligned bound-box (in local-space). */
	struct BoundBox *bb;

	int local_collections_bits;

	float object_to_world[4][4];
	float world_to_object[4][4];
} ObjectRuntime;

typedef struct Object {
	ID id;
	
	struct DrawDataList drawdata;
	struct Object *parent;
	struct Object *track;
	
	int flag_visibility;
	int flag;
	int type;
	int partype;
	int rotmode;

	struct Collection *instance_collection;
	
	/** Pointer to objects data - an 'ID' or NULL. */
	void *data;
	
	float loc[3], dloc[3];
	/** Scale (can be negative). */
	float scale[3], dscale[3];
	/** Euler rotation. */
	float rot[3], drot[3];
	/** Quaternion rotation. */
	float quat[4], dquat[4];
	/** Final world-space matrix with constraints & animsys applied. */
	float obmat[4][4];
	
	/** Inverse result of parent, so that object doesn't 'stick' to parent. */
	float parentinv[4][4];
	/**
	 * Inverse matrix of 'obmat' for any other use than rendering!
	 *
	 * \note this isn't assured to be valid as with 'obmat',
	 * before using this value you should do: `invert_m4_m4(ob->imat, ob->obmat)`
	 */
	float invmat[4][4];
	
	ObjectRuntime runtime;
} Object;

/** #Object->flag_visibility */
enum {
	OB_HIDE_VIEWPORT = (1 << 0),
	OB_HIDE_RENDER = (1 << 1),
	OB_HIDE_SELECT = (1 << 2),
};

/** #Object->flag */
enum {
	OBJECT_SELECTED = (1 << 0),
};

/** #Object->type */
enum {
	OB_EMPTY,
	OB_MESH,
	OB_ARMATURE,
};

/** #Object->partype, first 4 bits are the type. */
enum {
	PARTYPE = (1 << 4) - 1,
	PAROBJECT = 0,
	PARSKEL = 4,
	PARBONE = 7,
};

/* #object->rotmode */
enum {
	/* quaternion rotations (default, and for older Rose versions) */
	ROT_MODE_QUAT = 0,

	/* euler rotations - keep in sync with enum in LIB_math.h */
	/** Rose 'default' (classic) - must be as 1 to sync with LIB_math_rotation.h defines */
	ROT_MODE_EUL = 1,
	ROT_MODE_XYZ = 1,
	ROT_MODE_XZY = 2,
	ROT_MODE_YXZ = 3,
	ROT_MODE_YZX = 4,
	ROT_MODE_ZXY = 5,
	ROT_MODE_ZYX = 6,
	/**
	 * NOTE: space is reserved here for 18 other possible
	 * euler rotation orders not implemented.
	 */
	/* axis angle rotations */
	ROT_MODE_AXISANGLE = -1,

	ROT_MODE_MIN = ROT_MODE_AXISANGLE, /* sentinel for negative value. */
	ROT_MODE_MAX = ROT_MODE_ZYX,
};

#ifdef __cplusplus
}
#endif

#endif // DNA_OBJECT_TYPES_H
