#include "MEM_guardedalloc.h"

#include "KER_action.h"
#include "KER_armature.h"
#include "KER_idtype.h"
#include "KER_idprop.h"
#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_main.h"
#include "KER_mesh.h"
#include "KER_object.h"
#include "KER_object_deform.h"
#include "KER_scene.h"

#include "LIB_ghash.h"
#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_math_vector.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"

/* -------------------------------------------------------------------- */
/** \name Armature Edit Routines
 * \{ */

void vec_roll_to_mat3_normalized(const float nor[3], const float roll, float r_mat[3][3]) {
	/**
	 * Given `v = (v.x, v.y, v.z)` our (normalized) bone vector, we want the rotation matrix M
	 * from the Y axis (so that `M * (0, 1, 0) = v`).
	 * - The rotation axis a lays on XZ plane, and it is orthonormal to v,
	 *   hence to the projection of v onto XZ plane.
	 * - `a = (v.z, 0, -v.x)`
	 *
	 * We know a is eigenvector of M (so M * a = a).
	 * Finally, we have w, such that M * w = (0, 1, 0)
	 * (i.e. the vector that will be aligned with Y axis once transformed).
	 * We know w is symmetric to v by the Y axis.
	 * - `w = (-v.x, v.y, -v.z)`
	 *
	 * Solving this, we get (x, y and z being the components of v):
	 * <pre>
	 *     ┌ (x^2 * y + z^2) / (x^2 + z^2),   x,   x * z * (y - 1) / (x^2 + z^2) ┐
	 * M = │  x * (y^2 - 1)  / (x^2 + z^2),   y,    z * (y^2 - 1)  / (x^2 + z^2) │
	 *     └ x * z * (y - 1) / (x^2 + z^2),   z,   (x^2 + z^2 * y) / (x^2 + z^2) ┘
	 * </pre>
	 *
	 * This is stable as long as v (the bone) is not too much aligned with +/-Y
	 * (i.e. x and z components are not too close to 0).
	 *
	 * Since v is normalized, we have `x^2 + y^2 + z^2 = 1`,
	 * hence `x^2 + z^2 = 1 - y^2 = (1 - y)(1 + y)`.
	 *
	 * This allows to simplifies M like this:
	 * <pre>
	 *     ┌ 1 - x^2 / (1 + y),   x,     -x * z / (1 + y) ┐
	 * M = │                -x,   y,                   -z │
	 *     └  -x * z / (1 + y),   z,    1 - z^2 / (1 + y) ┘
	 * </pre>
	 *
	 * Written this way, we see the case v = +Y is no more a singularity.
	 * The only one
	 * remaining is the bone being aligned with -Y.
	 *
	 * Let's handle
	 * the asymptotic behavior when bone vector is reaching the limit of y = -1.
	 * Each of the four corner elements can vary from -1 to 1,
	 * depending on the axis a chosen for doing the rotation.
	 * And the "rotation" here is in fact established by mirroring XZ plane by that given axis,
	 * then inversing the Y-axis.
	 * For sufficiently small x and z, and with y approaching -1,
	 * all elements but the four corner ones of M will degenerate.
	 * So let's now focus on these corner elements.
	 *
	 * We rewrite M so that it only contains its four corner elements,
	 * and combine the `1 / (1 + y)` factor:
	 * <pre>
	 *                    ┌ 1 + y - x^2,        -x * z ┐
	 * M* = 1 / (1 + y) * │                            │
	 *                    └      -x * z,   1 + y - z^2 ┘
	 * </pre>
	 *
	 * When y is close to -1, computing 1 / (1 + y) will cause severe numerical instability,
	 * so we use a different approach based on x and z as inputs.
	 * We know `y^2 = 1 - (x^2 + z^2)`, and `y < 0`, hence `y = -sqrt(1 - (x^2 + z^2))`.
	 *
	 * Since x and z are both close to 0, we apply the binomial expansion to the second order:
	 * `y = -sqrt(1 - (x^2 + z^2)) = -1 + (x^2 + z^2) / 2 + (x^2 + z^2)^2 / 8`, which allows
	 * eliminating the problematic `1` constant.
	 *
	 * A first order expansion allows simplifying to this, but second order is more precise:
	 * <pre>
	 *                        ┌  z^2 - x^2,  -2 * x * z ┐
	 * M* = 1 / (x^2 + z^2) * │                         │
	 *                        └ -2 * x * z,   x^2 - z^2 ┘
	 * </pre>
	 *
	 * P.S. In the end, this basically is a heavily optimized version of Damped Track +Y.
	 */

	const float SAFE_THRESHOLD = 6.1e-3f;	  /* Theta above this value has good enough precision. */
	const float CRITICAL_THRESHOLD = 2.5e-4f; /* True singularity if XZ distance is below this. */
	const float THRESHOLD_SQUARED = CRITICAL_THRESHOLD * CRITICAL_THRESHOLD;

	const float x = nor[0];
	const float y = nor[1];
	const float z = nor[2];

	float theta = 1.0f + y;				   /* Remapping Y from [-1,+1] to [0,2]. */
	const float theta_alt = x * x + z * z; /* Squared distance from origin in x,z plane. */
	float rMatrix[3][3], bMatrix[3][3];

	/* Determine if the input is far enough from the true singularity of this type of
	 * transformation at (0,-1,0), where roll becomes 0/0 undefined without a limit.
	 *
	 * When theta is close to zero (nor is aligned close to negative Y Axis),
	 * we have to check we do have non-null X/Z components as well.
	 * Also, due to float precision errors, nor can be (0.0, -0.99999994, 0.0) which results
	 * in theta being close to zero. This will cause problems when theta is used as divisor.
	 */
	if (theta > SAFE_THRESHOLD || theta_alt > THRESHOLD_SQUARED) {
		/* nor is *not* aligned to negative Y-axis (0,-1,0). */

		bMatrix[0][1] = -x;
		bMatrix[1][0] = x;
		bMatrix[1][1] = y;
		bMatrix[1][2] = z;
		bMatrix[2][1] = -z;

		if (theta <= SAFE_THRESHOLD) {
			/* When nor is close to negative Y axis (0,-1,0) the theta precision is very bad,
			 * so recompute it from x and z instead, using the series expansion for `sqrt`. */
			theta = theta_alt * 0.5f + theta_alt * theta_alt * 0.125f;
		}

		bMatrix[0][0] = 1 - x * x / theta;
		bMatrix[2][2] = 1 - z * z / theta;
		bMatrix[2][0] = bMatrix[0][2] = -x * z / theta;
	}
	else {
		/* nor is very close to negative Y axis (0,-1,0): use simple symmetry by Z axis. */
		unit_m3(bMatrix);
		bMatrix[0][0] = bMatrix[1][1] = -1.0;
	}

	/* Make Roll matrix */
	axis_angle_normalized_to_mat3(rMatrix, nor, roll);

	/* Combine and output result */
	mul_m3_m3m3(r_mat, rMatrix, bMatrix);
}

void vec_roll_to_mat3(const float vec[3], const float roll, float r_mat[3][3]) {
	float nor[3];

	normalize_v3_v3(nor, vec);
	vec_roll_to_mat3_normalized(nor, roll, r_mat);
}

void mat3_to_vec_roll(const float mat[3][3], float r_vec[3], float *r_roll) {
	if (r_vec) {
		copy_v3_v3(r_vec, mat[1]);
	}

	if (r_roll) {
		mat3_vec_to_roll(mat, mat[1], r_roll);
	}
}

void mat3_vec_to_roll(const float mat[3][3], const float vec[3], float *r_roll) {
	float vecmat[3][3], vecmatinv[3][3], rollmat[3][3], q[4];

	/* Compute the orientation relative to the vector with zero roll. */
	vec_roll_to_mat3(vec, 0.0f, vecmat);
	invert_m3_m3(vecmatinv, vecmat);
	mul_m3_m3m3(rollmat, vecmatinv, mat);

	/* Extract the twist angle as the roll value. */
	mat3_to_quat(q, rollmat);

	*r_roll = quat_split_swing_and_twist(q, 1, NULL, NULL);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Armature Creation
 * \{ */

Armature *KER_armature_add(Main *main, const char *name) {
	return KER_id_new(main, ID_AR, name);
}

Armature *KER_armature_from_object(Object *object) {
	if (object->type == OB_ARMATURE) {
		return (Armature *)object->data;
	}
	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Armature Pose
 * \{ */

ROSE_STATIC size_t rebuild_pose_bone(Pose *pose, Bone *bone, PoseChannel *parchannel, size_t counter, Bone **r_last_visited_bone_p) {
	PoseChannel *pchannel = KER_pose_channel_ensure(pose, bone->name);

	pchannel->bone = bone;
	pchannel->parent = parchannel;

	/* We ensure the current pchan is immediately after the one we just generated/updated in the
	 * previous call to `rebuild_pose_bone`.
	 *
	 * It may be either the parent, the previous sibling, or the last
	 * (grand-(grand-(...)))-child (as processed by the recursive, depth-first nature of this
	 * function) of the previous sibling.
	 *
	 * NOTE: In most cases there is nothing to do here, but pose list may get out of order when some
	 * bones are added, removed or moved in the armature data. */
	PoseChannel *pchan_prev = pchannel->prev;
	const Bone *last_visited_bone = *r_last_visited_bone_p;
	if ((pchan_prev == NULL && last_visited_bone != NULL) || (pchan_prev != NULL && pchan_prev->bone != last_visited_bone)) {
		pchan_prev = last_visited_bone != NULL ? KER_pose_channel_find_name(pose, last_visited_bone->name) : NULL;
		LIB_remlink(&pose->channelbase, pchannel);
		LIB_insertlinkafter(&pose->channelbase, pchan_prev, pchannel);
	}

	*r_last_visited_bone_p = pchannel->bone;
	counter++;

	for (bone = (Bone *)(bone->childbase.first); bone; bone = bone->next) {
		counter = rebuild_pose_bone(pose, bone, pchannel, counter, r_last_visited_bone_p);
		/* for quick detecting of next bone in chain, only b-bone uses it now */
		if (bone->flag & BONE_CONNECTED) {
			pchannel->child = KER_pose_channel_find_name(pose, bone->name);
		}
	}

	return counter;
}

void KER_pose_clear_pointers(Pose *pose) {
	LISTBASE_FOREACH(PoseChannel *, pchannel, &pose->channelbase) {
		pchannel->bone = NULL;
		pchannel->child = NULL;
	}
}

void KER_pose_remap_bone_pointers(Armature *armature, Pose *pose) {
	LISTBASE_FOREACH(PoseChannel *, pchannel, &pose->channelbase) {
		pchannel->bone = KER_armature_find_bone_name(armature, pchannel->name);
	}
}

void KER_pose_channels_clear_with_null_bone(Pose *pose, const bool do_id_user) {
	LISTBASE_FOREACH_MUTABLE(PoseChannel *, pchan, &pose->channelbase) {
		if (pchan->bone == NULL) {
			KER_pose_channel_free_ex(pchan, do_id_user);
			KER_pose_channels_hash_free(pose);
			LIB_freelinkN(&pose->channelbase, pchan);
		}
	}
}

void KER_pose_rebuild(Main *main, Object *object, Armature *armature, bool do_id_user) {
	Pose *pose;
	size_t counter = 0;

	if (object->pose == NULL) {
		object->pose = MEM_callocN(sizeof(Pose), "Pose");
	}
	pose = object->pose;

	KER_pose_clear_pointers(pose);

	Bone *prev_bone = NULL;
	LISTBASE_FOREACH(Bone *, bone, &armature->bonebase) {
		counter = rebuild_pose_bone(pose, bone, NULL, counter, &prev_bone);
	}

	KER_pose_channels_clear_with_null_bone(pose, do_id_user);
	KER_pose_channels_hash_ensure(pose);

	pose->flag &= ~POSE_RECALC;
	pose->flag |= POSE_WAS_REBUILT;

	if (main != NULL) {
		DEG_relations_tag_update(main);
	}
}

void KER_pose_channel_index_rebuild(Pose *pose) {
	MEM_SAFE_FREE(pose->channels);
	const size_t num_channels = LIB_listbase_count(&pose->channelbase);
	pose->channels = MEM_mallocN(sizeof(PoseChannel *) * num_channels, "Pose::channels");

	size_t index = 0;
	LISTBASE_FOREACH_INDEX(PoseChannel *, pchannel, &pose->channelbase, index) {
		pose->channels[index] = pchannel;
	}
}

void KER_pose_eval_init(Depsgraph *depsgraph, Scene *scene, Object *object) {
	Pose *pose = object->pose;
	ROSE_assert(pose != NULL);
	ROSE_assert(object->type == OB_ARMATURE);
	ROSE_assert(object->pose != NULL);
	ROSE_assert((object->pose->flag & POSE_RECALC) == 0);

	invert_m4_m4(object->runtime.world_to_object, object->runtime.object_to_world);

	ROSE_assert(pose->channels != NULL || LIB_listbase_is_empty(&pose->channelbase));
}

void KER_pose_eval_init_ik(Depsgraph *depsgraph, Scene *scene, Object *object) {
	ROSE_assert(object->type == OB_ARMATURE);

	/* construct the IK tree (standard IK) */
	// RIK_init_tree(object, 0.0f);
}

ROSE_INLINE PoseChannel *pose_pchannel_get_indexed(Object *object, size_t index) {
	Pose *pose = object->pose;
	ROSE_assert(pose != NULL);
	ROSE_assert(pose->channels != NULL);
	ROSE_assert(index >= 0);
	ROSE_assert(index < MEM_allocN_length(pose->channels) / sizeof(PoseChannel *));
	return pose->channels[index];
}

void KER_pose_eval_bone(Depsgraph *depsgraph, Scene *scene, Object *object, size_t index) {
	const Armature *armature = (Armature *)object->data;

	if (armature->ebonebase != NULL) {
		return;
	}

	PoseChannel *pchannel = pose_pchannel_get_indexed(object, index);
	ROSE_assert(object->type == OB_ARMATURE);

	if (pchannel->flag & POSE_IKTREE) {
		/* pass */
	}
	else {
		if ((pchannel->flag & POSE_DONE) == 0) {
			/* TODO(lp64ace): Use time source node for time. */
			float ctime = KER_scene_frame_get(scene);
			KER_pose_where_is_bone(depsgraph, scene, object, pchannel, ctime);
		}
	}
}

static void pose_channel_flush_to_orig_if_needed(struct Depsgraph *depsgraph, struct Object *object, PoseChannel *pchan) {
	if (!DEG_is_active(depsgraph)) {
		return;
	}
	const Armature *armature = (Armature *)object->data;
	if (armature->ebonebase != NULL) {
		return;
	}
	PoseChannel *pchan_orig = pchan->runtime.orig_pchannel;
	copy_m4_m4(pchan_orig->pose_mat, pchan->pose_mat);
	copy_m4_m4(pchan_orig->chan_mat, pchan->chan_mat);
}

void KER_pose_bone_done(Depsgraph *depsgraph, Object *object, size_t index) {
	const Armature *armature = (Armature *)object->data;

	if (armature->ebonebase != NULL) {
		return;
	}

	PoseChannel *pchannel = pose_pchannel_get_indexed(object, index);
	ROSE_assert(object->type == OB_ARMATURE);

	float imat[4][4];
	if (pchannel->bone) {
		invert_m4_m4(imat, pchannel->bone->arm_mat);
		mul_m4_m4m4(pchannel->chan_mat, pchannel->pose_mat, imat);
	}
	pose_channel_flush_to_orig_if_needed(depsgraph, object, pchannel);
}

void KER_pose_eval_cleanup(Depsgraph *depsgraph, Scene *scene, Object *object) {
	Pose *pose = object->pose;
	ROSE_assert(pose != NULL);
	UNUSED_VARS_NDEBUG(pose);
	ROSE_assert(object->type == OB_ARMATURE);
	/* Release the IK tree. */
	// RIK_release_tree(scene, object, ctime);
}

void KER_pose_eval_done(Depsgraph *depsgraph, Object *object) {
	Pose *pose = object->pose;
	ROSE_assert(pose != NULL);
	UNUSED_VARS_NDEBUG(pose);
	ROSE_assert(object->type == OB_ARMATURE);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Armature Bone List
 * \{ */

size_t KER_armature_bonelist_count(const ListBase *lb) {
	size_t i = 0;
	LISTBASE_FOREACH(Bone *, bone, lb) {
		i += 1 + KER_armature_bonelist_count(&bone->childbase);
	}

	return i;
}

void KER_armature_bonelist_free(ListBase *lb, const bool do_id_user) {
	LISTBASE_FOREACH(Bone *, bone, lb) {
		if (bone->prop) {
			IDP_FreeProperty_ex(bone->prop, do_id_user);
		}
		KER_armature_bonelist_free(&bone->childbase, do_id_user);
	}

	LIB_freelistN(lb);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Armature Transform Copy
 * \{ */

static void copy_bone_transform(Bone *bone_dst, const Bone *bone_src) {
	copy_v3_v3(bone_dst->head, bone_src->head);
	copy_v3_v3(bone_dst->tail, bone_src->tail);

	copy_m3_m3(bone_dst->mat, bone_src->mat);

	copy_v3_v3(bone_dst->arm_head, bone_src->arm_head);
	copy_v3_v3(bone_dst->arm_tail, bone_src->arm_tail);

	copy_m4_m4(bone_dst->arm_mat, bone_src->arm_mat);
}

void KER_armature_copy_bone_transforms(Armature *armature_dst, const Armature *armature_src) {
	Bone *bone_dst = (Bone *)armature_dst->bonebase.first;
	const Bone *bone_src = (const Bone *)armature_src->bonebase.first;
	while (bone_dst != NULL) {
		ROSE_assert(bone_src != NULL);
		copy_bone_transform(bone_dst, bone_src);
		bone_dst = bone_dst->next;
		bone_src = bone_src->next;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Armature Bone Find by Name
 *
 * Using fast #GHash lookups when available.
 * \{ */

static Bone *get_named_bone_bonechildren(ListBase *lb, const char *name) {
	LISTBASE_FOREACH(Bone *, bone, lb) {
		if (STREQ(bone->name, name)) {
			return bone;
		}

		Bone *rbone = get_named_bone_bonechildren(&bone->childbase, name);
		if (rbone) {
			return rbone;
		}
	}

	return NULL;
}

Bone *KER_armature_find_bone_name(Armature *arm, const char *name) {
	if (!arm) {
		return NULL;
	}

	if (arm->bonehash) {
		return (Bone *)(LIB_ghash_lookup(arm->bonehash, name));
	}

	return get_named_bone_bonechildren(&arm->bonebase, name);
}

static void armature_bone_from_name_insert_recursive(GHash *bone_hash, ListBase *lb) {
	LISTBASE_FOREACH(Bone *, bone, lb) {
		LIB_ghash_insert(bone_hash, bone->name, bone);
		armature_bone_from_name_insert_recursive(bone_hash, &bone->childbase);
	}
}

/**
 * Create a (name -> bone) map.
 *
 * \note typically #bPose.chanhash us used via #BKE_pose_channel_find_name
 * this is for the cases we can't use pose channels.
 */
static GHash *armature_bone_from_name_map(Armature *arm) {
	const int bones_count = KER_armature_bonelist_count(&arm->bonebase);
	GHash *bone_hash = LIB_ghash_str_new_ex(__func__, bones_count);
	armature_bone_from_name_insert_recursive(bone_hash, &arm->bonebase);
	return bone_hash;
}

void KER_armature_bone_hash_make(Armature *arm) {
	if (!arm->bonehash) {
		arm->bonehash = armature_bone_from_name_map(arm);
	}
}

void KER_armature_bone_hash_free(Armature *arm) {
	if (arm->bonehash) {
		LIB_ghash_free(arm->bonehash, NULL, NULL);
		arm->bonehash = NULL;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Bone Matrix Calculation API
 * \{ */

/* Transformation inherited from the parent bone. These matrices apply the effects of
 * HINGE/NO_SCALE/NO_LOCAL_LOCATION options over the pchan loc/rot/scale transformations. */
typedef struct BoneParentTransform {
	float rotscale_mat[4][4]; /* parent effect on rotation & scale pose channels */
	float loc_mat[4][4];	  /* parent effect on location pose channel */
	float post_scale[3];	  /* additional scale to apply with post-multiply */
} BoneParentTransform;

void KER_bone_offset_matrix_get(const Bone *bone, float offs_bone[4][4]) {
	ROSE_assert(bone->parent != NULL);

	/* Bone transform itself. */
	copy_m4_m3(offs_bone, bone->mat);

	/* The bone's root offset (is in the parent's coordinate system). */
	copy_v3_v3(offs_bone[3], bone->head);

	/* Get the length translation of parent (length along y axis). */
	offs_bone[3][1] += bone->parent->length;
}

void KER_bone_parent_transform_calc_from_matrices(int bone_flag, int inherit_scale_mode, const float offs_bone[4][4], const float parent_arm_mat[4][4], const float parent_pose_mat[4][4], BoneParentTransform *r_bpt) {
	copy_v3_fl(r_bpt->post_scale, 1.0f);

	if (parent_pose_mat) {
		const bool use_rotation = true;
		const bool full_transform = true;

		/* Compose the rotscale matrix for this bone. */
		if (full_transform) {
			/* Parent pose rotation and scale. */
			mul_m4_m4m4(r_bpt->rotscale_mat, parent_pose_mat, offs_bone);
		}
		else {
			float tmat[4][4], tscale[3];

			/* If using parent pose rotation: */
			if (use_rotation) {
				copy_m4_m4(tmat, parent_pose_mat);

				/* Normalize the matrix when needed. */
				switch (inherit_scale_mode) {
					case BONE_INHERIT_SCALE_FULL:
						/* Keep scale and shear. */
						break;

					default:
						ROSE_assert_unreachable();
				}
			}
			/* If removing parent pose rotation: */
			else {
				copy_m4_m4(tmat, parent_arm_mat);

				/* Copy the parent scale when needed. */
				switch (inherit_scale_mode) {
					case BONE_INHERIT_SCALE_FULL:
						/* Ignore effects of shear. */
						mat4_to_size(tscale, parent_pose_mat);
						rescale_m4(tmat, tscale);
						break;

					default:
						ROSE_assert_unreachable();
				}
			}

			mul_m4_m4m4(r_bpt->rotscale_mat, tmat, offs_bone);
		}

		/* Compose the loc matrix for this bone. */
		/* NOTE: That version does not modify bone's loc when HINGE/NO_SCALE options are set. */

		/* Those flags do not affect position, use plain parent transform space! */
		if (!full_transform) {
			mul_m4_m4m4(r_bpt->loc_mat, parent_pose_mat, offs_bone);
		}
		/* Else (i.e. default, usual case),
		 * just use the same matrix for rotation/scaling, and location. */
		else {
			copy_m4_m4(r_bpt->loc_mat, r_bpt->rotscale_mat);
		}
	}
	/* Root bones. */
	else {
		/* Rotation/scaling. */
		copy_m4_m4(r_bpt->rotscale_mat, offs_bone);
		/* Translation. */
		copy_m4_m4(r_bpt->loc_mat, r_bpt->rotscale_mat);
	}
}

void KER_bone_parent_transform_calc_from_pchan(const PoseChannel *pchan, BoneParentTransform *r_bpt) {
	const Bone *bone, *parbone;
	const PoseChannel *parchan;

	/* set up variables for quicker access below */
	bone = pchan->bone;
	parbone = bone->parent;
	parchan = pchan->parent;

	if (parchan) {
		float offs_bone[4][4];
		/* yoffs(b-1) + root(b) + bonemat(b). */
		KER_bone_offset_matrix_get(bone, offs_bone);

		KER_bone_parent_transform_calc_from_matrices(bone->flag, bone->inherit_scale_mode, offs_bone, parbone->arm_mat, parchan->pose_mat, r_bpt);
	}
	else {
		KER_bone_parent_transform_calc_from_matrices(bone->flag, bone->inherit_scale_mode, bone->arm_mat, NULL, NULL, r_bpt);
	}
}

void KER_bone_parent_transform_apply(const BoneParentTransform *bpt, const float inmat[4][4], float outmat[4][4]) {
	/* in case inmat == outmat */
	float tmploc[3];
	copy_v3_v3(tmploc, inmat[3]);

	mul_m4_m4m4(outmat, bpt->rotscale_mat, inmat);
	mul_v3_m4v3(outmat[3], bpt->loc_mat, tmploc);
	rescale_m4(outmat, bpt->post_scale);
}

void KER_armature_mat_bone_to_pose(const PoseChannel *pchannel, const float inmat[4][4], float outmat[4][4]) {
	BoneParentTransform bpt;

	KER_bone_parent_transform_calc_from_pchan(pchannel, &bpt);
	KER_bone_parent_transform_apply(&bpt, inmat, outmat);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Pose Solver
 * \{ */

void KER_pose_ensure(Main *main, Object *object, Armature *armature, bool do_id_user) {
	ROSE_assert(!ELEM(NULL, armature, object));
	if (object->type == OB_ARMATURE && ((object->pose == NULL) || (object->pose->flag & POSE_RECALC))) {
		ROSE_assert(GS(armature->id.name) == ID_AR);
		KER_pose_rebuild(main, object, armature, do_id_user);
	}
}

void KER_armature_where_is(Armature *armature) {
	/* hierarchical from root to children */
	LISTBASE_FOREACH(Bone *, bone, &armature->bonebase) {
		KER_armature_where_is_bone(bone, NULL, true);
	}
}

void KER_armature_where_is_bone(Bone *bone, const Bone *parbone, bool use_recursion) {
	float vec[3];

	/* Bone Space */
	sub_v3_v3v3(vec, bone->tail, bone->head);
	bone->length = len_v3(vec);
	vec_roll_to_mat3(vec, bone->roll, bone->mat);

	if (parbone) {
		float offs_bone[4][4];
		/* yoffs(b-1) + root(b) + bonemat(b) */
		KER_bone_offset_matrix_get(bone, offs_bone);

		/* Compose the matrix for this bone. */
		mul_m4_m4m4(bone->arm_mat, parbone->arm_mat, offs_bone);
	}
	else {
		copy_m4_m3(bone->arm_mat, bone->mat);
		copy_v3_v3(bone->arm_mat[3], bone->head);
	}

	/* and the children */
	if (use_recursion) {
		LISTBASE_FOREACH(Bone *, childbone, &bone->childbase) {
			KER_armature_where_is_bone(childbone, bone, use_recursion);
		}
	}
}

void KER_pose_where_is(Depsgraph *depsgraph, Scene *scene, Object *object) {
	if (object->type != OB_ARMATURE) {
		return;
	}

	Armature *armature = (Armature *)object->data;

	if (ELEM(NULL, armature)) {
		return;
	}

	KER_pose_ensure(NULL, object, armature, true);

	/* In edit-mode or rest-position we read the data from the bones. */
	if (armature->ebonebase) {
		LISTBASE_FOREACH(PoseChannel *, pchannel, &object->pose->channelbase) {
			Bone *bone = pchannel->bone;
			if (bone) {
				copy_m4_m4(pchannel->pose_mat, bone->arm_mat);
			}
		}
	}
	else {
		invert_m4_m4(object->runtime.world_to_object, object->runtime.object_to_world);

		LISTBASE_FOREACH(PoseChannel *, pchannel, &object->pose->channelbase) {
			if (!(pchannel->flag & POSE_DONE)) {
				KER_pose_where_is_bone(depsgraph, scene, object, pchannel, 0.0f);
			}
		}
	}

	float imat[4][4];

	/* calculating deform matrices */
	LISTBASE_FOREACH(PoseChannel *, pchannel, &object->pose->channelbase) {
		if (pchannel->bone) {
			invert_m4_m4(imat, pchannel->bone->arm_mat);
			mul_m4_m4m4(pchannel->chan_mat, pchannel->pose_mat, imat);
		}
	}
}

void KER_pose_where_is_bone(Depsgraph *depsgraph, Scene *scene, Object *object, PoseChannel *pchannel, float time) {
	KER_pose_channel_do_mat4(pchannel);

	/* Construct the posemat based on PoseChannels, that we do before applying constraints. */
	/* pose_mat(b) = pose_mat(b-1) * yoffs(b-1) * d_root(b) * bone_mat(b) * chan_mat(b) */
	KER_armature_mat_bone_to_pose(pchannel, pchannel->chan_mat, pchannel->pose_mat);
}

void KER_pose_channel_rot_to_mat3(const PoseChannel *pchan, float r_mat[3][3]) {
	/* rotations may either be quats, eulers (with various rotation orders), or axis-angle */
	if (pchan->rotmode == ROT_MODE_QUAT) {
		/* quats are normalized before use to eliminate scaling issues */
		float quat[4];

		/* NOTE: we now don't normalize the stored values anymore,
		 * since this was kind of evil in some cases but if this proves to be too problematic,
		 * switch back to the old system of operating directly on the stored copy. */
		normalize_qt_qt(quat, pchan->quat);
		quat_to_mat3(r_mat, quat);
	}
	else if (pchan->rotmode == ROT_MODE_AXISANGLE) {
		/* axis-angle - not really that great for 3D-changing orientations */
		axis_angle_to_mat3(r_mat, pchan->rotAxis, pchan->rotAngle);
	}
	else {
		/* Euler rotations (will cause gimbal lock,
		 * but this can be alleviated a bit with rotation orders) */
		eulO_to_mat3(r_mat, pchan->euler, pchan->rotmode);
	}
}

void KER_pose_channel_to_mat4(const PoseChannel *pchannel, float r_mat[4][4]) {
	float smat[3][3];
	float rmat[3][3];
	float tmat[3][3];

	/* get scaling matrix */
	size_to_mat3(smat, pchannel->scale);

	/* get rotation matrix */
	KER_pose_channel_rot_to_mat3(pchannel, rmat);

	/* calculate matrix of bone (as 3x3 matrix, but then copy the 4x4) */
	mul_m3_m3m3(tmat, rmat, smat);
	copy_m4_m3(r_mat, tmat);

	/* prevent action channels breaking chains */
	/* need to check for bone here, CONSTRAINT_TYPE_ACTION uses this call */
	if ((pchannel->bone == NULL) || !(pchannel->bone->flag & BONE_CONNECTED)) {
		copy_v3_v3(r_mat[3], pchannel->loc);
	}
}
void KER_pose_channel_do_mat4(PoseChannel *pchannel) {
	KER_pose_channel_to_mat4(pchannel, pchannel->chan_mat);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Armature Bone Flags
 * \{ */

bool KER_armature_bone_flag_test_recursive(const Bone *bone, int flag) {
	if (bone->flag & flag) {
		return true;
	}
	if (bone->parent) {
		return KER_armature_bone_flag_test_recursive(bone->parent, flag);
	}
	return false;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Armature Data-Block Functions
 * \{ */

ROSE_STATIC void armature_init_data(struct ID *id) {
	Armature *armature = (Armature *)id;
}

static void copy_bonechildren(Bone *bone_dst, const Bone *bone_src, const int flag) {
	Bone *bone_src_child, *bone_dst_child;

	if (bone_src->prop) {
		bone_dst->prop = IDP_DuplicateProperty_ex(bone_src->prop, flag);
	}

	/* Copy this bone's list */
	LIB_duplicatelist(&bone_dst->childbase, &bone_src->childbase);

	/* For each child in the list, update its children */
	for (bone_src_child = bone_src->childbase.first, bone_dst_child = bone_dst->childbase.first; bone_src_child; bone_src_child = bone_src_child->next, bone_dst_child = bone_dst_child->next) {
		bone_dst_child->parent = bone_dst;
		copy_bonechildren(bone_dst_child, bone_src_child, flag);
	}
}

void KER_armature_copy_data(Main *main, Armature *arm_dst, const Armature *arm_src, const int flag) {
	Bone *bone_src, *bone_dst;

	/* We never handle usercount here for own data. */
	const int flag_subdata = flag | LIB_ID_CREATE_NO_USER_REFCOUNT;

	arm_dst->bonehash = NULL;

	LIB_duplicatelist(&arm_dst->bonebase, &arm_src->bonebase);

	/* Duplicate the childrens' lists */
	bone_dst = arm_dst->bonebase.first;
	for (bone_src = arm_src->bonebase.first; bone_src; bone_src = bone_src->next) {
		bone_dst->parent = NULL;
		copy_bonechildren(bone_dst, bone_src, flag_subdata);
		bone_dst = bone_dst->next;
	}

	KER_armature_bone_hash_make(arm_dst);
}

ROSE_STATIC void armature_copy_data(Main *main, ID *id_dst, const ID *id_src, const int flag) {
	KER_armature_copy_data(main, id_dst, id_src, flag);
}

ROSE_STATIC void armature_free_data(struct ID *id) {
	Armature *armature = (Armature *)id;

	KER_armature_bone_hash_free(armature);
	KER_armature_bonelist_free(&armature->bonebase, false);

	if (armature->ebonebase) {
		KER_armature_bonelist_free(armature->ebonebase, false);
	}
	MEM_SAFE_FREE(armature->ebonebase);
}

ROSE_STATIC void armature_foreach_id_bone(Bone *bone, struct LibraryForeachIDData *data) {
	KER_LIB_FOREACHID_PROCESS_FUNCTION_CALL(data, IDP_foreach_property(bone->prop, IDP_TYPE_FILTER_ID, KER_lib_query_idpropertiesForeachIDLink_callback, data));

	LISTBASE_FOREACH(Bone *, curbone, &bone->childbase) {
		KER_LIB_FOREACHID_PROCESS_FUNCTION_CALL(data, armature_foreach_id_bone(curbone, data));
	}
}

ROSE_STATIC void armature_foreach_id_ebone(EditBone *ebone, struct LibraryForeachIDData *data) {
	KER_LIB_FOREACHID_PROCESS_FUNCTION_CALL(data, IDP_foreach_property(ebone->prop, IDP_TYPE_FILTER_ID, KER_lib_query_idpropertiesForeachIDLink_callback, data));

	LISTBASE_FOREACH(EditBone *, curbone, &ebone->childbase) {
		KER_LIB_FOREACHID_PROCESS_FUNCTION_CALL(data, armature_foreach_id_ebone(curbone, data));
	}
}

ROSE_STATIC void armature_foreach_id(ID *id, struct LibraryForeachIDData *data) {
	Armature *armature = (Armature *)id;

	LISTBASE_FOREACH(Bone *, bone, &armature->bonebase) {
		KER_LIB_FOREACHID_PROCESS_FUNCTION_CALL(data, armature_foreach_id_bone(bone, data));
	}

	if (armature->ebonebase) {
		LISTBASE_FOREACH(EditBone *, bone, armature->ebonebase) {
			KER_LIB_FOREACHID_PROCESS_FUNCTION_CALL(data, armature_foreach_id_ebone(bone, data));
		}
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Armature Data-Block Definition
 * \{ */

IDTypeInfo IDType_ID_AR = {
	.idcode = ID_AR,

	.filter = FILTER_ID_AR,
	.depends = 0,
	.index = INDEX_ID_AR,
	.size = sizeof(Armature),

	.name = "Armature",
	.name_plural = "Armatures",

	.flag = 0,

	.init_data = armature_init_data,
	.copy_data = armature_copy_data,
	.free_data = armature_free_data,

	.foreach_id = armature_foreach_id,

	.write = NULL,
	.read_data = NULL,
};

/** \} */
