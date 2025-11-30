#include "MEM_guardedalloc.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_string.h"

#include "KER_deform.h"
#include "KER_object_deform.h"

/* -------------------------------------------------------------------- */
/** \name Deform Vertex
 * \{ */

MDeformWeight *KER_defvert_find_index(const MDeformVert *dvert, const int defgroup) {
	if (dvert && defgroup >= 0) {
		MDeformWeight *dw = dvert->dw;
		unsigned int i;

		for (i = dvert->totweight; i != 0; i--, dw++) {
			if (dw->def_nr == defgroup) {
				return dw;
			}
		}
	}
	else {
		ROSE_assert(0);
	}

	return NULL;
}

MDeformWeight *KER_defvert_ensure_index(MDeformVert *dvert, int defgroup) {
	MDeformWeight *dw_new;

	/* do this check always, this function is used to check for it */
	if (!dvert || defgroup < 0) {
		ROSE_assert(0);
		return NULL;
	}

	dw_new = KER_defvert_find_index(dvert, defgroup);
	if (dw_new) {
		return dw_new;
	}

	dw_new = MEM_mallocN(sizeof(MDeformWeight) * (dvert->totweight + 1), "MDeformWeight");
	if (dvert->dw) {
		memcpy(dw_new, dvert->dw, sizeof(MDeformWeight) * dvert->totweight);
		MEM_freeN(dvert->dw);
	}
	dvert->dw = dw_new;
	dw_new += dvert->totweight;
	dw_new->weight = 0.0f;
	dw_new->def_nr = defgroup;
	/* Group index */

	dvert->totweight++;

	return dw_new;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Deform Group
 * \{ */

DeformGroup *KER_object_defgroup_new(Object *object, const char *name) {
	ROSE_assert(OB_TYPE_SUPPORT_VGROUP(object->type));

	DeformGroup *defgroup = MEM_callocN(sizeof(DeformGroup), "DeformGroup");

	LIB_strcpy(defgroup->name, ARRAY_SIZE(defgroup->name), name);

	ListBase *defbase = KER_object_defgroup_list_mutable(object);

	LIB_addtail(defbase, defgroup);
	KER_object_batch_cache_dirty_tag(object);

	return defgroup;
}

/** \} */
