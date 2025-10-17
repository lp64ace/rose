#ifndef KER_ARMATURE_H
#define KER_ARMATURE_H

#include "DNA_armature_types.h"

struct Armature;
struct Main;
struct Object;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Armature Edit Structures
 * \{ */

typedef struct EditBone {
	struct EditBone *prev, *next;

	char name[64];

	struct IDProperty *prop;

	float mat[3][3];
	float head[3];
	float tail[3];
	float roll;

	int flag;

	float length;
	float weight;

	struct EditBone *parent;

	struct ListBase childbase;

	/* Used to store temporary data */
	union {
		struct EditBone *ebone;
		struct Bone *bone;
	} temp;
} EditBone;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Armature Edit Routines
 * \{ */

/**
 * Calculates the rest matrix of a bone based on its vector and a roll around that vector.
 */
void vec_roll_to_mat3_normalized(const float nor[3], const float roll, float r_mat[3][3]);

/**
 * Computes vector and roll based on a rotation.
 * "mat" must contain only a rotation, and no scaling.
 */
void mat3_to_vec_roll(const float mat[3][3], float r_vec[3], float *r_roll);
/**
 * Computes roll around the vector that best approximates the matrix.
 * If `vec` is the Y vector from purely rotational `mat`, result should be exact.
 */
void mat3_vec_to_roll(const float mat[3][3], const float vec[3], float *r_roll);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Armature Creation
 * \{ */

struct Armature *KER_armature_add(struct Main *main, const char *name);
struct Armature *KER_armature_from_object(struct Object *object);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Armature Pose
 * \{ */

/**
 * Only after leave edit-mode, duplicating, validating older files, library syncing.
 *
 * \note pose->flag is set for it.
 *
 * \param main: May be NULL, only used to tag depsgraph as being dirty.
 */
void KER_pose_rebuild(struct Main *main, struct Object *ob, struct Armature *arm, bool do_id_user);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Armature Bone List
 * \{ */

size_t KER_armature_bonelist_count(const struct ListBase *lb);

void KER_armature_bonelist_free(struct ListBase *lb, const bool do_id_user);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Armature Bone Find by Name
 *
 * Using fast #GHash lookups when available.
 * \{ */

void KER_armature_bone_hash_make(struct Armature *arm);
void KER_armature_bone_hash_free(struct Armature *arm);

struct Bone *KER_armature_find_bone_name(struct Armature *arm, const char *name);

/** \} */

#ifdef __cplusplus
}
#endif

#endif