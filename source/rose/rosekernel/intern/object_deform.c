#include "MEM_guardedalloc.h"

#include "LIB_listbase.h"

#include "KER_deform.h"
#include "KER_mesh.h"
#include "KER_object.h"
#include "KER_object_deform.h"

/* -------------------------------------------------------------------- */
/** \name Group List
 * \{ */

const ListBase *KER_id_defgroup_list_get(const ID *id) {
	switch (GS(id->name)) {
		case ID_ME: {
			return &((const Mesh *)id)->vertex_group_names;
		} break;
	}
	ROSE_assert_unreachable();
	return NULL;
}

ListBase *KER_id_defgroup_list_get_mutable(ID *id) {
	switch (GS(id->name)) {
		case ID_ME: {
			return &((Mesh *)id)->vertex_group_names;
		} break;
	}
	ROSE_assert_unreachable();
	return NULL;
}

const ListBase *KER_object_defgroup_list(const Object *object) {
	ROSE_assert(OB_TYPE_SUPPORT_VGROUP(object->type));
	return KER_id_defgroup_list_get((ID *)object->data);
}

ListBase *KER_object_defgroup_list_mutable(Object *object) {
	ROSE_assert(OB_TYPE_SUPPORT_VGROUP(object->type));
	return KER_id_defgroup_list_get_mutable((ID *)object->data);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Group creation
 * \{ */

DeformGroup *KER_object_defgroup_add(Object *object) {
	return KER_object_defgroup_add_name(object, "Group");
}

DeformGroup *KER_object_defgroup_add_name(Object *object, const char *name) {
	DeformGroup *defgroup;

	if (!object || !OB_TYPE_SUPPORT_VGROUP(object->type)) {
		return NULL;
	}

	defgroup = KER_object_defgroup_new(object, name);

	return defgroup;
}

/** \} */
