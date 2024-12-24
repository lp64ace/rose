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
	/** TODO: Fix */
	return KER_object_add_only_object(main, type, name);
}
Object *KER_object_add_for_data(Main *main, struct Scene *scene, int type, const char *name, ID *data) {
	/** TODO: Fix */
	return KER_object_add_only_object(main, type, name);
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
