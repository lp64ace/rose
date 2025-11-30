#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_math_vector.h"

#include "KER_action.h"
#include "KER_armature.h"
#include "KER_camera.h"
#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_main.h"
#include "KER_mesh.h"
#include "KER_modifier.h"
#include "KER_object.h"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Data-Block Functions
 * \{ */

ROSE_STATIC void object_init_data(struct ID *id) {
	Object *ob = (Object *)id;
	
	ob->type = OB_EMPTY;

	// Transform

	ob->rotmode = ROT_MODE_EUL;
	copy_v3_fl(ob->scale, 1.0f);
	copy_v3_fl(ob->dscale, 1.0f);

	unit_m4(ob->runtime.object_to_world);
	unit_m4(ob->runtime.world_to_object);
}

ROSE_STATIC void object_free_data(struct ID *id) {
	Object *ob = (Object *)id;

	KER_object_free_modifiers(ob, LIB_ID_CREATE_NO_USER_REFCOUNT);

	if (ob->pose) {
		KER_pose_free_ex(ob->pose, false);
		ob->pose = NULL;
	}
}

ROSE_STATIC void object_foreach_id(struct ID *id, struct LibraryForeachIDData *data) {
	Object *ob = (Object *)id;
	
	/* object data special case */
	if (ob->type == OB_EMPTY) {
		/* empty can have nullptr. */
		KER_LIB_FOREACHID_PROCESS_ID(data, ob->data, IDWALK_CB_USER);
	}
	else {
		/* when set, this can't be nullptr */
		if (ob->data) {
			KER_LIB_FOREACHID_PROCESS_ID(data, ob->data, IDWALK_CB_USER | IDWALK_CB_NEVER_NULL);
		}
	}
	
	KER_LIB_FOREACHID_PROCESS_IDSUPER(data, ob->parent, IDWALK_CB_NEVER_SELF);
	KER_LIB_FOREACHID_PROCESS_IDSUPER(data, ob->track, IDWALK_CB_NEVER_SELF);
}

ROSE_STATIC void object_init(Object *ob, int type) {
	object_init_data(&ob->id);
	
	ob->type = type;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Creation
 * \{ */

ROSE_STATIC int KER_object_obdata_to_type(const ID *id) {
#define CASE_IDTYPE(type, obtype) case type: return obtype

	switch (GS(id->name)) {
		CASE_IDTYPE(ID_CA, OB_CAMERA);
		CASE_IDTYPE(ID_ME, OB_MESH);
		CASE_IDTYPE(ID_AR, OB_ARMATURE);
	}
	
	ROSE_assert_msg(false, "[Kernel]: Bad id object-data type.");
	return -1;
	
#undef CASE_IDTYPE
}

ROSE_STATIC const char *get_obdata_defname(int type) {
#define CASE_OBTYPE(type, name) case type: return name

	switch (type) {
		CASE_OBTYPE(OB_ARMATURE, "Armature");
		CASE_OBTYPE(OB_CAMERA, "Camera");
		CASE_OBTYPE(OB_MESH, "Mesh");
		CASE_OBTYPE(OB_EMPTY, "Empty");
	}
	
	ROSE_assert_msg(false, "[Kernel]: Bad object data type.");
	return "Empty";
	
#undef CASE_OBTYPE
}

ROSE_STATIC Object *KER_object_add_only_object(Main *main, int type, const char *name) {
	if (!name) {
		name = get_obdata_defname(type);
	}
	
	Object *ob = (Object *)KER_libblock_alloc(main, ID_OB, name, main ? 0 : LIB_ID_CREATE_NO_MAIN);
	object_init(ob, type);
	
	return ob;
}

Object *KER_object_add(Main *main, struct Scene *scene, int type, const char *name) {
	return KER_object_add_only_object(main, type, name);
}
Object *KER_object_add_for_data(Main *main, struct Scene *scene, int type, const char *name, ID *data, bool do_user) {
	Object *ob = (Object *)KER_object_add_only_object(main, type, name);
	ob->data = (void *)data;

	if (do_user) {
		id_us_add(data);
	}

	return ob;
}

void *KER_object_obdata_add_from_type(Main *main, int type, const char *name) {
	if (name == NULL) {
		name = get_obdata_defname(type);
	}
	
#define CASE_OBTYPE(type, fn) case type: return fn
	
	switch(type) {
		CASE_OBTYPE(OB_CAMERA, KER_camera_add(main, name));
		CASE_OBTYPE(OB_MESH, KER_mesh_add(main, name));
		CASE_OBTYPE(OB_ARMATURE, KER_armature_add(main, name));
		CASE_OBTYPE(OB_EMPTY, NULL);
	}
	
	ROSE_assert_msg(false, "[Kernel]: Bad object data type.");
	return NULL;
	
#undef CASE_OBTYPE
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Parent Object Transform
 * \{ */

ROSE_STATIC void ob_parbone(const Object *ob, Object *par, float r_mat[4][4]) {
	float vec[3];

	if (par->type != OB_ARMATURE) {
		unit_m4(r_mat);
		return;
	}

	const PoseChannel *pchan = KER_pose_channel_find_name(par->pose, ob->parsubstr);
	if (!pchan || !pchan->bone) {
		unit_m4(r_mat);
		return;
	}

	/* get bone transform */
	if (pchan->bone->flag & BONE_RELATIVE_PARENTING) {
		/* the new option uses the root - expected behavior, but differs from old... */
		/* XXX check on version patching? */
		copy_m4_m4(r_mat, pchan->chan_mat);
	}
	else {
		copy_m4_m4(r_mat, pchan->pose_mat);

		/* but for backwards compatibility, the child has to move to the tail */
		copy_v3_v3(vec, r_mat[1]);
		mul_v3_fl(vec, pchan->bone->length);
		add_v3_v3(r_mat[3], vec);
	}
}

void KER_object_get_parent_matrix(const Object *ob, Object *par, float r_parentmat[4][4]) {
	float tmat[4][4];

	switch (ob->partype & PARTYPE) {
		case PAROBJECT: {
			bool ok = false;

			if (ok) {
				mul_m4_m4m4(r_parentmat, KER_object_object_to_world(par), tmat);
			}
			else {
				copy_m4_m4(r_parentmat, KER_object_object_to_world(par));
			}

			break;
		}
		case PARBONE: {
			ob_parbone(ob, par, tmat);
			mul_m4_m4m4(r_parentmat, KER_object_object_to_world(par), tmat);
		} break;

		case PARSKEL: {
			copy_m4_m4(r_parentmat, KER_object_object_to_world(par));
		} break;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Transform
 * \{ */

void KER_object_scale_to_mat3(Object *ob, float r_mat[3][3]) {
	float vec[3];
	mul_v3_v3v3(vec, ob->scale, ob->dscale);
	size_to_mat3(r_mat, vec);
}

void KER_object_rot_to_mat3(Object *ob, float r_mat[3][3], bool use_drot) {
	float rmat[3][3], dmat[3][3];

	/**
	 * 'dmat' is the delta-rotation matrix, which will get (pre)multiplied
	 * with the rotation matrix to yield the appropriate rotation
	 */

	/* rotations may either be quats, eulers (with various rotation orders), or axis-angle */
	if (ob->rotmode > 0) {
		/* Euler rotations
		 * (will cause gimbal lock, but this can be alleviated a bit with rotation orders). */
		eulO_to_mat3(rmat, ob->rot, ob->rotmode);
		eulO_to_mat3(dmat, ob->drot, ob->rotmode);
	}
	else {
		/* quats are normalized before use to eliminate scaling issues */
		float tquat[4];

		normalize_qt_qt(tquat, ob->quat);
		quat_to_mat3(rmat, tquat);

		normalize_qt_qt(tquat, ob->dquat);
		quat_to_mat3(dmat, tquat);
	}

	/* combine these rotations */
	if (use_drot) {
		mul_m3_m3m3(r_mat, dmat, rmat);
	}
	else {
		copy_m3_m3(r_mat, rmat);
	}
}

void KER_object_to_mat3(Object *ob, float r_mat[3][3]) {
	float smat[3][3];
	float rmat[3][3];

	/* Scale. */
	KER_object_scale_to_mat3(ob, smat);

	/* Rotation. */
	KER_object_rot_to_mat3(ob, rmat, true);
	mul_m3_m3m3(r_mat, rmat, smat);
}

void KER_object_to_mat4(Object *ob, float r_mat[4][4]) {
	float tmat[3][3];

	KER_object_to_mat3(ob, tmat);

	copy_m4_m3(r_mat, tmat);

	add_v3_v3v3(r_mat[3], ob->loc, ob->dloc);
}

void KER_object_matrix_parent_get(Object *ob, Object *par, float r_mat[4][4]) {
	float mat[4][4];
	float vec[3];

	switch (ob->partype & PARTYPE) {
		case PAROBJECT: {
			copy_m4_m4(r_mat, par->obmat);
		} break;
		case PARBONE: {
			ob_parbone(ob, par, mat);
			mul_m4_m4m4(r_mat, par->obmat, mat);
		} break;
		case PARSKEL: {
			copy_m4_m4(r_mat, par->obmat);
		} break;
	}
}

void KER_object_matrix_local_get(Object *ob, float r_mat[4][4]) {
	if (ob->parent) {
		float par_imat[4][4];

		KER_object_matrix_parent_get(ob, ob->parent, par_imat);
		invert_m4(par_imat);
		mul_m4_m4m4(r_mat, par_imat, ob->obmat);
	}
	else {
		copy_m4_m4(r_mat, ob->obmat);
	}
}

const float (*KER_object_object_to_world(const Object *object))[4] {
	return object->runtime.object_to_world;
}

const float (*KER_object_world_to_object(const Object *object))[4] {
	return object->runtime.world_to_object;
}

void KER_object_mat3_to_rot(Object *object, float mat[3][3], bool use_compat) {
	switch (object->rotmode) {
		case ROT_MODE_QUAT: {
			float dquat[4];
			mat3_normalized_to_quat(object->quat, mat);
			normalize_qt_qt(dquat, object->dquat);
			invert_qt_normalized(dquat);
			mul_qt_qtqt(object->quat, dquat, object->quat);
			break;
		}
		case ROT_MODE_AXISANGLE: {
			float quat[4];
			float dquat[4];

			/* Without `drot` we could apply 'mat' directly. */
			mat3_normalized_to_quat(quat, mat);
			axis_angle_to_quat(dquat, object->rotAxis, object->drotAngle);
			invert_qt_normalized(dquat);
			mul_qt_qtqt(quat, dquat, quat);
			quat_to_axis_angle(object->rotAxis, &object->rotAngle, quat);
			break;
		}
		default: /* euler */
		{
			float quat[4];
			float dquat[4];

			/* Without `drot` we could apply 'mat' directly. */
			mat3_normalized_to_quat(quat, mat);
			eulO_to_quat(dquat, object->drot, object->rotmode);
			invert_qt_normalized(dquat);
			mul_qt_qtqt(quat, dquat, quat);
			/* End `drot` correction. */

			if (use_compat) {
				quat_to_compatible_eulO(object->rot, object->rot, object->rotmode, quat);
			}
			else {
				quat_to_eulO(object->rot, object->rotmode, quat);
			}
			break;
		}
	}
}

void KER_object_apply_mat4_ex(Object *object, const float mat[4][4], Object *parent, const float parentinv[4][4], bool use_compat) {
	float rot[3][3];

	if (parent != NULL) {
		float rmat[4][4], diff_mat[4][4], imat[4][4], parent_mat[4][4];

		KER_object_get_parent_matrix(object, parent, parent_mat);

		mul_m4_m4m4(diff_mat, parent_mat, parentinv);
		invert_m4_m4(imat, diff_mat);
		mul_m4_m4m4(rmat, imat, mat); /* get the parent relative matrix */

		/* same as below, use rmat rather than mat */
		mat4_to_loc_rot_size(object->loc, rot, object->scale, rmat);
	}
	else {
		mat4_to_loc_rot_size(object->loc, rot, object->scale, mat);
	}

	KER_object_mat3_to_rot(object, rot, use_compat);

	sub_v3_v3(object->loc, object->dloc);

	if (object->dscale[0] != 0.0f) {
		object->scale[0] /= object->dscale[0];
	}
	if (object->dscale[1] != 0.0f) {
		object->scale[1] /= object->dscale[1];
	}
	if (object->dscale[2] != 0.0f) {
		object->scale[2] /= object->dscale[2];
	}
}

void KER_object_apply_mat4(Object *object, const float mat[4][4], bool use_compat, bool use_parent) {
	KER_object_apply_mat4_ex(object, mat, use_parent ? object->parent : NULL, object->parentinv, use_compat);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Evaluation
 * \{ */

void KER_object_build_armature_bonebase(ListBase *bonebase) {
	LISTBASE_FOREACH(Bone *, bone, bonebase) {
		KER_object_build_armature_bonebase(&bone->childbase);
	}
}

void KER_object_build_armature(Armature *armature) {
	KER_object_build_armature_bonebase(&armature->bonebase);
}

void KER_object_build_rig(Object *object) {
	ROSE_assert(object->type == OB_ARMATURE);

	Armature *armature = (Armature *)object->data;

	/** Without an armature there is nothing we can do! */
	if (armature == NULL) {
		return;
	}

	KER_object_build_armature(armature);

	if (object->pose == NULL || (object->pose->flag & POSE_RECALC) != 0) {
		/* By definition, no need to tag depsgraph as dirty from here, so we can pass NULL main. */
		KER_pose_rebuild(NULL, object, armature, true);
	}
	if (object->pose != NULL) {
		KER_pose_channels_hash_ensure(object->pose);
		KER_pose_pchannel_index_rebuild(object->pose);
	}

	/**
	 * Pose Rig Graph
	 * ==============
	 *
	 * Pose Component:
	 * - Mainly used for referencing Bone components.
	 * - This is where the evaluation operations for init/exec/cleanup
	 *   (ik) solvers live, and are later hooked up (so that they can be
	 *   interleaved during runtime) with bone-operations they depend on/affect.
	 * - init_pose_eval() and cleanup_pose_eval() are absolute first and last
	 *   steps of pose eval process. ALL bone operations must be performed
	 *   between these two...
	 *
	 * Bone Component:
	 * - Used for representing each bone within the rig
	 * - Acts to encapsulate the evaluation operations (base matrix + parenting,
	 *   and constraint stack) so that they can be easily found.
	 * - Everything else which depends on bone-results hook up to the component
	 *   only so that we can redirect those to point at either the
	 *   post-IK/post-constraint/post-matrix steps, as needed.
	 */

	/* Pose eval context. */

	KER_pose_eval_init(object);
	KER_pose_eval_init_ik(object);
	KER_pose_eval_cleanup(object);
	KER_pose_eval_done(object);

	if (object->pose) {
		size_t index = 0;
		LISTBASE_FOREACH(PoseChannel *, pchannel, &object->pose->channelbase) {
			KER_pose_eval_bone(object, index);
			KER_pose_bone_done(object, index);
			index++;
		}
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Modifiers
 * \{ */

void KER_object_free_modifiers(Object *object, const int flag) {
	for (ModifierData *md; md = (ModifierData *)LIB_pophead(&object->modifiers);) {
		KER_modifier_free_ex(md, flag);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Cache
 * \{ */

void KER_object_batch_cache_dirty_tag(Object *object) {
	switch (object->type) {
		case OB_MESH: {
			KER_mesh_batch_cache_tag_dirty(object->data, KER_MESH_BATCH_DIRTY_ALL);
		} break;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Data-block Definition
 * \{ */

IDTypeInfo IDType_ID_OB = {
	.idcode = ID_OB,

	.filter = FILTER_ID_OB,
	.depends = 0,
	.index = INDEX_ID_OB,
	.size = sizeof(Object),

	.name = "Object",
	.name_plural = "Objects",

	.flag = IDTYPE_FLAGS_NO_COPY,

	.init_data = object_init_data,
	.copy_data = NULL,
	.free_data = object_free_data,

	.foreach_id = object_foreach_id,

	.write = NULL,
	.read_data = NULL,
};

/** \} */
