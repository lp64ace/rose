#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_math_vector.h"

#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_main.h"
#include "KER_mesh.h"
#include "KER_object.h"

/* -------------------------------------------------------------------- */
/** \name Data-Block Functions
 * \{ */

ROSE_STATIC void object_init_data(struct ID *id) {
	Object *ob = (Object *)id;
	
	ob->type = OB_EMPTY;
}

ROSE_STATIC void object_free_data(struct ID *id) {
	Object *ob = (Object *)id;
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
		CASE_IDTYPE(ID_ME, OB_MESH);
	}
	
	ROSE_assert_msg(false, "[Kernel]: Bad id object-data type.");
	return -1;
	
#undef CASE_IDTYPE
}

ROSE_STATIC const char *get_obdata_defname(int type) {
#define CASE_OBTYPE(type, name) case type: return name

	switch (type) {
		CASE_OBTYPE(OB_ARMATURE, "Armature");
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
		CASE_OBTYPE(OB_MESH, KER_mesh_add(main, name));
		CASE_OBTYPE(OB_EMPTY, NULL);
	}
	
	ROSE_assert_msg(false, "[Kernel]: Bad object data type.");
	return NULL;
	
#undef CASE_OBTYPE
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

ROSE_STATIC void ob_parbone(Object *ob, Object *par, float r_mat[4][4]) {
	float vec[3];

	if (par->type != OB_ARMATURE) {
		unit_m4(r_mat);
		return;
	}

	ROSE_assert_unreachable();

	/** Find the bone and retrieve the bone transform. */
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
