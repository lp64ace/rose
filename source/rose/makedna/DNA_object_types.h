#ifndef DNA_OBJECT_TYPES_H
#define DNA_OBJECT_TYPES_H

#include "DNA_ID.h"

#include "DNA_action_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DeformGroup {
	struct DeformGroup *prev, *next;

	char name[64];
} DeformGroup;

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
	/** Animation data (must be immediately after id for utilities to use it). */
	struct AnimData *adt;
	
	struct DrawDataList drawdata;
	struct Object *parent;
	struct Object *track;
	
	int flag_visibility;
	int flag;
	int type;
	int partype;
	int rotmode;

	struct Collection *instance_collection;
	
	struct Pose *pose;

	/** String describing sub-object info. */
	char parsubstr[64];

	/** Pointer to objects data - an 'ID' or NULL. */
	void *data;
	
	float loc[3], dloc[3];
	/** Scale (can be negative). */
	float scale[3], dscale[3];
	/** Euler rotation. */
	float rot[3], drot[3];
	/** Quaternion rotation. */
	float quat[4], dquat[4];
	/** Axis angle rotation (axis) */
	float rotAxis[3], drotAxis[3];
	/** Axis angle rotation (angle) */
	float rotAngle, drotAngle;
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

	ListBase modifiers;
	
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

#define OB_TYPE_SUPPORT_VGROUP(_type) (ELEM(_type, OB_MESH))

/** #Object->partype, first 4 bits are the type. */
enum {
	PARTYPE = (1 << 4) - 1,
	PAROBJECT = 0,
	PARSKEL = 4,
	PARBONE = 7,
};

#ifdef __cplusplus
}
#endif

#endif // DNA_OBJECT_TYPES_H
