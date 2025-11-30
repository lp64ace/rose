#include "MEM_guardedalloc.h"

#include "LIB_math_matrix.h"
#include "LIB_math_vector.h"
#include "LIB_listbase.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "KER_armature.h"
#include "KER_idprop.h"
#include "KER_main.h"
#include "KER_object.h"

#include "ED_armature.h"

ROSE_STATIC void make_ebone_list_recursive(ListBase *ebonebase, ListBase *bonebase, EditBone *parent) {
	EditBone *ebone;
	
	LISTBASE_FOREACH(Bone *, bone, bonebase) {
		ebone = (EditBone *)MEM_callocN(sizeof(EditBone), "editors::EditBone");

		LIB_strcpy(ebone->name, ARRAY_SIZE(ebone->name), bone->name);

		ebone->parent = parent;
		ebone->flag = bone->flag;
		
		copy_v3_v3(ebone->head, bone->head);
		copy_v3_v3(ebone->tail, bone->tail);
		ebone->roll = bone->roll;
		
		ebone->length = bone->length;
		ebone->weight = bone->weight;
		
		if (bone->prop) {
			ebone->prop = IDP_DuplicateProperty(bone->prop);
		}

		LIB_addtail(ebonebase, ebone);

		if (!LIB_listbase_is_empty(&bone->childbase)) {
			make_ebone_list_recursive(ebonebase, &bone->childbase, ebone);
		}
	}
}

ROSE_INLINE void make_ebone_list(ListBase *ebonebase, ListBase *bonebase) {
	ROSE_assert(!ebonebase->first && !ebonebase->last);

	make_ebone_list_recursive(ebonebase, bonebase, NULL);
}

void ED_armature_edit_new(Armature *armature) {
	ED_armature_edit_free(armature);
	armature->ebonebase = (ListBase *)MEM_callocN(sizeof(ListBase), "edbo armature");
	make_ebone_list(armature->ebonebase, &armature->bonebase);
}

ROSE_STATIC void ebone_free(Armature *armature, EditBone *ebone) {
	if (ebone->prop) {
		IDP_FreeProperty(ebone->prop);
	}

	LIB_freelinkN(armature->ebonebase, ebone);
}

ROSE_STATIC void armature_finalize_restpose(ListBase *bonelist, ListBase *editbonelist) {
	LISTBASE_FOREACH(Bone *, bone, bonelist) {
		/* Set bone's local head/tail.
		 * Note that it's important to use final parent's rest-pose (arm_mat) here,
		 * instead of setting those values from edit-bone's matrix (see #46010). */
		if (bone->parent) {
			float parmat_inv[4][4];

			invert_m4_m4(parmat_inv, bone->parent->arm_mat);

			/* Get the new head and tail */
			sub_v3_v3v3(bone->head, bone->arm_head, bone->parent->arm_tail);
			sub_v3_v3v3(bone->tail, bone->arm_tail, bone->parent->arm_tail);

			mul_m3_m4_v3(parmat_inv, bone->head);
			mul_m3_m4_v3(parmat_inv, bone->tail);
		}
		else {
			copy_v3_v3(bone->head, bone->arm_head);
			copy_v3_v3(bone->tail, bone->arm_tail);
		}

		/**
		 * Set local matrix and arm_mat (rest-pose).
		 * Do not recurse into children here, armature_finalize_restpose() is already recursive.
		 */
		KER_armature_where_is_bone(bone, bone->parent, false);

		/* Find the associated editbone */
		LISTBASE_FOREACH(EditBone *, ebone, editbonelist) {
			if (ebone->temp.bone == bone) {
				float premat[3][3];
				float postmat[3][3];
				float difmat[3][3];
				float imat[3][3];

				/* Get the ebone premat and its inverse. */
				ED_armature_ebone_to_mat3(ebone, premat);
				invert_m3_m3(imat, premat);

				/* Get the bone postmat. */
				copy_m3_m4(postmat, bone->arm_mat);

				mul_m3_m3m3(difmat, imat, postmat);

				bone->roll = -atan2f(difmat[2][0], difmat[2][2]);

				/* And set rest-position again. */
				KER_armature_where_is_bone(bone, bone->parent, false);
				break;
			}
		}

		/* Recurse into children... */
		armature_finalize_restpose(&bone->childbase, editbonelist);
	}
}

void ED_armature_edit_apply(Main *main, Armature *armature) {
	EditBone *ebone;
	EditBone *nebone;
	Bone *newbone;
	Object *object;

	KER_armature_bone_hash_free(armature);
	KER_armature_bonelist_free(&armature->bonebase, true);

	/* Remove zero sized bones, this gives unstable rest-poses. */
	const float bone_length_threshold = 0.000001f * 0.000001f;
	for (ebone = (EditBone *)(armature->ebonebase->first); ebone; ebone = nebone) {
		float len_sq = len_squared_v3v3(ebone->head, ebone->tail);
		nebone = ebone->next;
		if (len_sq <= bone_length_threshold) { /* FLT_EPSILON is too large? */
			/* Find any bones that refer to this bone */
			LISTBASE_FOREACH(EditBone *, fbone, armature->ebonebase) {
				if (fbone->parent == ebone) {
					fbone->parent = ebone->parent;
				}
			}
			ebone_free(armature, ebone);
		}
	}

	/* Copy the bones from the edit-data into the armature. */
	LISTBASE_FOREACH(EditBone *, ebone, armature->ebonebase) {
		newbone = MEM_callocN(sizeof(Bone), "editors::Bone");
		ebone->temp.bone = newbone; /* Associate the real Bones with the EditBones */

		LIB_strcpy(newbone->name, ARRAY_SIZE(newbone->name), ebone->name);
		newbone->arm_roll = ebone->roll;
		copy_v3_v3(newbone->arm_head, ebone->head);
		copy_v3_v3(newbone->arm_tail, ebone->tail);

		newbone->flag = ebone->flag;

		newbone->weight = ebone->weight;

		if (ebone->prop) {
			newbone->prop = IDP_DuplicateProperty(ebone->prop);
		}
	}

	/* Fix parenting in a separate pass to ensure ebone->bone connections are valid at this point.
	 * Do not set bone->head/tail here anymore,
	 * using EditBone data for that is not OK since our later fiddling with parent's arm_mat
	 * (for roll conversion) may have some small but visible impact on locations (#46010). */
	LISTBASE_FOREACH(EditBone *, ebone, armature->ebonebase) {
		newbone = ebone->temp.bone;
		if (ebone->parent) {
			newbone->parent = ebone->parent->temp.bone;
			LIB_addtail(&newbone->parent->childbase, newbone);
		}
		/*  ...otherwise add this bone to the armature's bonebase */
		else {
			LIB_addtail(&armature->bonebase, newbone);
		}
	}

	/* Finalize definition of rest-pose data (roll, bone_mat, arm_mat, head/tail...). */
	armature_finalize_restpose(&armature->bonebase, armature->ebonebase);

	KER_armature_bone_hash_make(armature);

	/* so all users of this armature should get rebuilt */
	LISTBASE_FOREACH(Object *, object, which_libbase(main, ID_OB)) {
		if (object->data == armature) {
			KER_pose_rebuild(main, object, armature, true);
		}
	}
}

void ED_armature_edit_free(Armature *armature) {
	if (armature->ebonebase) {
		if (!LIB_listbase_is_empty(armature->ebonebase)) {
			LISTBASE_FOREACH(EditBone *, ebone, armature->ebonebase) {
				if (ebone->prop) {
					IDP_FreeProperty(ebone->prop);
				}
			}
			LIB_freelistN(armature->ebonebase);
		}
	}
	MEM_SAFE_FREE(armature->ebonebase);
}

void ED_armature_ebone_to_mat3(EditBone *ebone, float r_mat[3][3]) {
	float delta[3], roll;

	/* Find the current bone matrix */
	sub_v3_v3v3(delta, ebone->tail, ebone->head);
	roll = ebone->roll;
	if (!normalize_v3(delta)) {
		/* Use the orientation of the parent bone if any. */
		const EditBone *ebone_parent = ebone->parent;
		if (ebone_parent) {
			sub_v3_v3v3(delta, ebone_parent->tail, ebone_parent->head);
			normalize_v3(delta);
			roll = ebone_parent->roll;
		}
	}

	vec_roll_to_mat3_normalized(delta, roll, r_mat);
}

void ED_armature_ebone_to_mat4(EditBone *ebone, float r_mat[4][4]) {
	float m3[3][3];

	ED_armature_ebone_to_mat3(ebone, m3);

	copy_m4_m3(r_mat, m3);
	copy_v3_v3(r_mat[3], ebone->head);
}

void ED_armature_ebone_from_mat3(EditBone *ebone, const float mat[3][3]) {
	float vec[3], roll;
	const float len = len_v3v3(ebone->head, ebone->tail);

	mat3_to_vec_roll(mat, vec, &roll);

	madd_v3_v3v3fl(ebone->tail, ebone->head, vec, len);
	ebone->roll = roll;
}

void ED_armature_ebone_from_mat4(EditBone *ebone, const float mat[4][4]) {
	float mat3[3][3];

	copy_m3_m4(mat3, mat);

	sub_v3_v3(ebone->tail, ebone->head);
	copy_v3_v3(ebone->head, mat[3]);
	add_v3_v3(ebone->tail, mat[3]);
	ED_armature_ebone_from_mat3(ebone, mat3);
}
