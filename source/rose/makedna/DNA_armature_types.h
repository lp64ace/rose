#ifndef DNA_ARMATURE_TYPES_H
#define DNA_ARMATURE_TYPES_H

#include "DNA_ID.h"

struct GHash;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Bone {
	struct Bone *prev, *next;

	char name[64];

	struct IDProperty *prop;

	float mat[3][3];
	float head[3];
	float tail[3];
	float roll;

	int flag;

	float arm_roll;
	/** Head position in armature space. So should be the same as head in edit mode. */
	float arm_head[3];
	/** Tail position in armature space. So should be the same as tail in edit mode. */
	float arm_tail[3];
	/** Matrix: `(bone_mat(b)+head(b))*arm_mat(b-1)`, rest pose in armature space. */
	float arm_mat[4][4];

	float length;
	float weight;

	struct Bone *parent;

	struct ListBase childbase;
} Bone;

enum {
	/** object child will use relative transform (like deform) */
	BONE_RELATIVE_PARENTING = (1 << 0),
	BONE_CONNECTED = (1 << 1),
	BONE_NO_DEFORM = (1 << 2),
};

typedef struct Armature {
	ID id;
	struct AnimData *adt;

	struct GHash *bonehash;
	struct ListBase bonebase;

	/** #EditBone list (use an allocated pointer so the state can be checked). */
	ListBase *ebonebase;
} Armature;

#ifdef __cplusplus
}
#endif

#endif // DNA_ARMATURE_TYPES_H
