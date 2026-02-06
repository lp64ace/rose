#include "MEM_guardedalloc.h"

#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_math_vector.h"
#include "LIB_string.h"

#include "KER_action.h"
#include "KER_armature.h"
#include "KER_camera.h"
#include "KER_derived_mesh.h"
#include "KER_idtype.h"
#include "KER_layer.h"
#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_main.h"
#include "KER_mesh.h"
#include "KER_modifier.h"
#include "KER_object.h"
#include "KER_scene.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_query.h"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Modifier Data-Block Functions
 * \{ */

ROSE_INLINE bool object_modifier_type_copy_check(int md_type) {
  return !ELEM(md_type, MODIFIER_TYPE_NONE);
}

bool KER_object_support_modifier_type_check(const Object *ob, int md_type) {
	const ModifierTypeInfo *mti = KER_modifier_get_info(md_type);

	/** Objects that reference mesh can always deform vertices! (obviously) */
	if (ELEM(ob->type, OB_MESH)) {
		return true;
	}

	return false;
}

bool KER_object_modifier_stack_copy(Object *ob_dst, const Object *ob_src, const bool do_copy_all, const int flag) {
	if (!LIB_listbase_is_empty(&ob_dst->modifiers)) {
		ROSE_assert_msg(false, "Trying to copy a modifier stack into an object having a non-empty modifier stack.");
		return false;
	}

	LISTBASE_FOREACH(const ModifierData *, md_src, &ob_src->modifiers) {
		if (!do_copy_all && !object_modifier_type_copy_check(md_src->type)) {
			continue;
		}
		if (!KER_object_support_modifier_type_check(ob_dst, md_src->type)) {
			continue;
		}

		ModifierData *md_dst = KER_modifier_copy_ex(md_src, flag);
		LIB_addtail(&ob_dst->modifiers, md_dst);
	}

	return true;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Data-Block Functions
 * \{ */

ROSE_STATIC void object_init_data(struct ID *id) {
	Object *ob = (Object *)id;
	
	ob->type = OB_EMPTY;

	// Transform

	ob->rotmode = ROT_MODE_QUAT;
	unit_qt(ob->quat);
	unit_qt(ob->dquat);

	copy_v3_fl(ob->scale, 1.0f);
	copy_v3_fl(ob->dscale, 1.0f);
}

ROSE_INLINE void copy_object_pose(Object *obnew, const Object *obold, const int flag) {
	obnew->pose = NULL;
	KER_pose_copy_data(&obnew->pose, obold->pose, flag);
}

ROSE_STATIC void object_copy_data(struct Main *main, struct ID *dst, const struct ID *src, int flag) {
	const Object *ob_src = (const Object *)src;
	Object *ob_dst = (Object *)dst;

	/* We never handle user-count here for own data. */
	const int flag_subdata = flag | LIB_ID_CREATE_NO_USER_REFCOUNT;

	if (ob_src->pose) {
		copy_object_pose(ob_dst, ob_src, flag_subdata);
		if (ob_src->type == OB_ARMATURE) {
			const bool do_pose_id_user = (flag & LIB_ID_CREATE_NO_USER_REFCOUNT) == 0;
			KER_pose_rebuild(main, ob_dst, (Armature *)ob_dst->data, do_pose_id_user);
		}
	}

	LIB_listbase_clear(&ob_dst->modifiers);
	KER_object_modifier_stack_copy(ob_dst, ob_src, true, flag_subdata);
}

ROSE_STATIC void object_free_data(struct ID *id) {
	Object *ob = (Object *)id;

	KER_object_free_modifiers(ob, LIB_ID_CREATE_NO_USER_REFCOUNT);

	if (ob->pose) {
		KER_pose_free_ex(ob->pose, false);
		ob->pose = NULL;
	}
}

ROSE_STATIC void library_foreach_modifiersForeachIDLink(void *user_data, Object *object, ID **id_pointer, const int cb_flag) {
	struct LibraryForeachIDData *data = (struct LibraryForeachIDData *)user_data;
	KER_LIB_FOREACHID_PROCESS_FUNCTION_CALL(data, KER_lib_query_foreachid_process(data, id_pointer, cb_flag));
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

	KER_LIB_FOREACHID_PROCESS_FUNCTION_CALL(data, KER_modifiers_foreach_ID_link(ob, library_foreach_modifiersForeachIDLink, data));
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
				mul_m4_m4m4(r_parentmat, par->obmat, tmat);
			}
			else {
				copy_m4_m4(r_parentmat, par->obmat);
			}

			break;
		}
		case PARBONE: {
			ob_parbone(ob, par, tmat);
			mul_m4_m4m4(r_parentmat, par->obmat, tmat);
		} break;

		case PARSKEL: {
			copy_m4_m4(r_parentmat, par->obmat);
		} break;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Transform
 * \{ */

void KER_object_parent_object_set(Object *object, Object *parent) {
	object->partype = PAROBJECT;
	object->parent = parent;

}
void KER_object_parent_bone_set(Object *object, Object *armature, const char *bonename) {
	object->partype = PARBONE;
	object->parent = armature;
	LIB_strcpy(object->parsubstr, ARRAY_SIZE(object->parsubstr), bonename);
}

void KER_object_scale_to_mat3(const Object *ob, float r_mat[3][3]) {
	float vec[3];
	mul_v3_v3v3(vec, ob->scale, ob->dscale);
	size_to_mat3(r_mat, vec);
}

void KER_object_rot_to_mat3(const Object *ob, float r_mat[3][3], bool use_drot) {
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

void KER_object_to_mat3(const Object *ob, float r_mat[3][3]) {
	float smat[3][3];
	float rmat[3][3];

	/* Scale. */
	KER_object_scale_to_mat3(ob, smat);

	/* Rotation. */
	KER_object_rot_to_mat3(ob, rmat, true);
	mul_m3_m3m3(r_mat, rmat, smat);
}

void KER_object_to_mat4(const Object *ob, float r_mat[4][4]) {
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
	return object->obmat;
}

const float (*KER_object_world_to_object(const Object *object))[4] {
	return object->invmat;
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

ROSE_STATIC void solve_parenting(const Object *object, Object *parent, float r_obmat[4][4], float r_originmat[3][3]) {
	float totmat[4][4];
	float tmat[4][4];
	float locmat[4][4];

	KER_object_to_mat4(object, locmat);

	KER_object_get_parent_matrix(object, parent, totmat);

	/* total */
	mul_m4_m4m4(tmat, totmat, object->parentinv);
	mul_m4_m4m4(r_obmat, tmat, locmat);

	if (r_originmat) {
		/* Usable `r_originmat`. */
		copy_m3_m4(r_originmat, tmat);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Runtime
 * \{ */

void KER_object_to_mesh_clear(Object *object) {
	if (object->runtime.temp_mesh_object == NULL) {
		return;
	}
	KER_id_free(NULL, object->runtime.temp_mesh_object);
	object->runtime.temp_mesh_object = NULL;
}

void KER_object_free_derived_caches(Object *ob) {
	MEM_SAFE_FREE(ob->runtime.bb);

	if (ob->runtime.data_eval != NULL) {
		if (ob->runtime.is_data_eval_owned) {
			ID *data_eval = ob->runtime.data_eval;
			if (GS(data_eval->name) == ID_ME) {
				KER_mesh_eval_delete((Mesh *)data_eval);
			}
			else {
				KER_libblock_free_data(data_eval, false);
				KER_libblock_free_datablock(data_eval, 0);
				MEM_freeN(data_eval);
			}
		}
		ob->runtime.data_eval = NULL;
	}
	if (ob->runtime.mesh_eval_deform != NULL) {
		Mesh *mesh_deform_eval = ob->runtime.mesh_eval_deform;
		KER_mesh_eval_delete(mesh_deform_eval);
		ob->runtime.mesh_eval_deform = NULL;
	}

	/**
	 * Restore initial pointer for copy-on-write data-blocks, object->data
	 * might be pointing to an evaluated data-block data was just freed above.
	 */
	if (ob->runtime.data_orig != NULL) {
		ob->data = ob->runtime.data_orig;
	}

	KER_object_to_mesh_clear(ob);
}

void KER_object_free_caches(Object *object) {
}

void KER_object_runtime_reset(Object *object) {
	memset(&object->runtime, 0, sizeof(object->runtime));
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Evaluation
 * \{ */

ThreadMutex mutex = ROSE_MUTEX_INITIALIZER;

void KER_object_eval_local_transform(Depsgraph *depsgraph, Object *ob) {
	KER_object_to_mat4(ob, ob->obmat);
}

void KER_object_eval_parent(Depsgraph *depsgraph, Object *ob) {
	Object *par = ob->parent;

	float totmat[4][4];
	float tmat[4][4];
	float locmat[4][4];

	copy_m4_m4(locmat, ob->obmat);

	/* get parent effect matrix */
	KER_object_get_parent_matrix(ob, par, totmat);
	
	/* total */
	mul_m4_m4m4(tmat, totmat, ob->parentinv);
	mul_m4_m4m4(ob->obmat, tmat, locmat);
}

void KER_object_eval_transform_final(Depsgraph *depsgraph, Object *ob) {
	/* Make sure inverse matrix is always up to date. This way users of it
	 * do not need to worry about recalculating it. */
	invert_m4_m4_safe(ob->invmat, ob->obmat);
}

void KER_object_handle_data_update(Depsgraph *depsgraph, Scene *scene, Object *ob) {
	switch (ob->type) {
		case OB_MESH: {
			CustomData_MeshMasks cddata_masks = CD_MASK_MESH;
			/* Custom attributes should not be removed automatically. They might be used by the render
			 * engine or scripts. They can still be removed explicitly using geometry nodes. */
			cddata_masks.vmask |= CD_MASK_PROP_ALL;
			cddata_masks.emask |= CD_MASK_PROP_ALL;
			cddata_masks.fmask |= CD_MASK_PROP_ALL;
			cddata_masks.pmask |= CD_MASK_PROP_ALL;
			cddata_masks.lmask |= CD_MASK_PROP_ALL;

			/* Also copy over normal layers to avoid recomputation. */
			cddata_masks.pmask |= CD_MASK_NORMAL;
			cddata_masks.vmask |= CD_MASK_NORMAL;

			KER_derived_mesh_make(depsgraph, scene, ob, &cddata_masks);
		} break;
		case OB_ARMATURE: {
			KER_pose_where_is(depsgraph, scene, ob);
		} break;
	}

	if (DEG_is_active(depsgraph)) {
		Object *object_orig = DEG_get_original_object(ob);
		KER_object_evaluated_geometry_bounds(ob, &object_orig->runtime.bb, true);
	}
}

void KER_object_eval_uber_data(Depsgraph *depsgraph, Scene *scene, Object *ob) {
	ROSE_assert(ob->type != OB_ARMATURE);
	KER_object_handle_data_update(depsgraph, scene, ob);
	KER_object_batch_cache_dirty_tag(ob);
}

void KER_object_eval_eval_base_flags(Depsgraph *depsgraph, Scene *scene, int view_layer_index, Object *object, int base_index, bool is_from_set) {
	ROSE_assert(view_layer_index >= 0);
	ViewLayer *view_layer = LIB_findlink(&scene->view_layers, view_layer_index);
	ROSE_assert(view_layer != NULL);
	ROSE_assert(view_layer->object_bases_array != NULL);
	ROSE_assert(base_index >= 0);
	ROSE_assert(base_index < MEM_allocN_length(view_layer->object_bases_array) / sizeof(Base *));
	Base *base = view_layer->object_bases_array[base_index];
	ROSE_assert(base->object == object);

	KER_base_eval_flags(base);
	/* Copy flags and settings from base. */
	object->flag_base = base->flag;
	if (is_from_set) {
		object->flag_base &= ~(BASE_SELECTED | BASE_SELECTABLE);
	}
}

void KER_object_sync_to_original(Depsgraph *depsgraph, Object *object) {
	if (!DEG_is_active(depsgraph)) {
		return;
	}
	Object *object_orig = DEG_get_original_object(object);
	/* Base flags. */
	object_orig->flag_base = object->flag_base;
	/* Transformation flags. */
	copy_m4_m4(object_orig->obmat, object->obmat);
	copy_m4_m4(object_orig->invmat, object->invmat);
	object_orig->flag = object->flag;

	/* Copy back error messages from modifiers. */
	for (ModifierData *md = object->modifiers.first, *md_orig = object_orig->modifiers.first; md != NULL && md_orig != NULL; md = md->next, md_orig = md_orig->next) {
		ROSE_assert(md->type == md_orig->type && STREQ(md->name, md_orig->name));
		MEM_SAFE_FREE(md_orig->error);
		if (md->error != NULL) {
			md_orig->error = LIB_strdupN(md->error);
		}
	}

	// TODO;
	// object_sync_boundbox_to_original(object_orig, object);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Modifiers
 * \{ */

void KER_object_free_modifiers(Object *object, const int flag) {
	for (ModifierData *md; md = (ModifierData *)LIB_pophead(&object->modifiers);) {
		KER_modifier_free_ex(md, flag);
	}

	KER_object_free_derived_caches(object);
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

	.flag = IDTYPE_FLAGS_DRAWDATA,

	.init_data = object_init_data,
	.copy_data = object_copy_data,
	.free_data = object_free_data,

	.foreach_id = object_foreach_id,

	.write = NULL,
	.read_data = NULL,
};

/** \} */
