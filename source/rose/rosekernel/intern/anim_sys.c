#include "MEM_guardedalloc.h"

#include "KER_action.h"
#include "KER_anim_data.h"
#include "KER_anim_sys.h"
#include "KER_idtype.h"
#include "KER_fcurve.h"
#include "KER_mesh.h"

#include "RNA_access.h"
#include "RNA_types.h"

#include "LIB_utildefines.h"

#include <stdio.h>

bool KER_anim_read_from_rna_path(PathResolvedRNA *resolved, float *r_value) {
	PropertyRNA *property = resolved->property;
	PointerRNA *ptr = &resolved->ptr;
	int index = resolved->index;

	float orig;

	ROSE_assert(RNA_property_animateable(ptr, property) || ptr->owner == NULL);

	switch (RNA_property_type(property)) {
		case PROP_FLOAT: {
			if (index != -1) {
				const float coerce = RNA_property_float_get_index(ptr, property, index);
				orig = coerce;
			}
			else {
				const float coerce = RNA_property_float_get(ptr, property);
				orig = coerce;
			}
		} break;
		default:
			/** Nothing can be done here... so it is unsuccessful? */
			return false;
	}

	if (r_value != NULL) {
		*r_value = orig;
	}

	return true;
}

bool KER_anim_write_to_rna_path(PathResolvedRNA *resolved, float value, bool force) {
	PropertyRNA *property = resolved->property;
	PointerRNA *ptr = &resolved->ptr;
	int index = resolved->index;

	ROSE_assert(RNA_property_animateable(ptr, property) || ptr->owner == NULL);

	if (!force) {
		float old;
		if (!KER_anim_read_from_rna_path(resolved, &old)) {
			return false;
		}
		if (old == value) {
			return true;
		}
	}

	switch (RNA_property_type(property)) {
		case PROP_FLOAT: {
			float coerce = value;
			RNA_property_float_clamp(ptr, property, &coerce);
			if (index != -1) {
				RNA_property_float_set_index(ptr, property, index, coerce);
			}
			else {
				RNA_property_float_set(ptr, property, coerce);
			}
		} break;
		default:
			/** Nothing can be done here... so it is unsuccessful? */
			return false;
	}

	return true;
}

bool KER_anim_evaluate_fcurves(PointerRNA pointer, FCurve **fcurves, int totcurve, float ctime) {
	bool updated = false;

	for (int index = 0; index < totcurve; index++) {
		FCurve *fcurve = fcurves[index];

		PathResolvedRNA resolved;
		if (KER_animsys_rna_path_resolve(&pointer, fcurve->path, fcurve->index, &resolved)) {
			const float curval = KER_fcurve_evaluate(&resolved, fcurve, ctime);

			updated |= KER_anim_write_to_rna_path(&resolved, curval, false);
		}
	}

	return updated;
}

bool KER_anim_evaluate_action(PointerRNA pointer, Action *action, int slot, float ctime) {
	ActionChannelBag *bag;
	if ((bag = KER_action_channelbag_for_action_slot_ex(action, slot))) {
		return KER_anim_evaluate_fcurves(pointer, bag->fcurves, bag->totcurve, ctime);
	}

	return false;
}

bool KER_anim_evaluate_and_apply_action(PointerRNA pointer, Action *action, int slot, float ctime) {
	return KER_anim_evaluate_action(pointer, action, slot, ctime);
}

void KER_animsys_evaluate_animdata(ID *id, AnimData *adt, float ctime, int recalc) {
	if (ELEM(NULL, id, adt)) {
		return;
	}

	PointerRNA idptr = RNA_id_pointer_create(id);

	if (recalc & ADT_RECALC_ANIM) {
		if (adt->action) {
			if (KER_action_is_layered(adt->action)) {
				KER_anim_evaluate_and_apply_action(idptr, adt->action, adt->handle, ctime);
			}
			else {
				KER_anim_evaluate_action(idptr, adt->action, 0, ctime);
			}
		}
	}
}

void KER_animsys_eval_animdata(ID *id) {
	AnimData *adt = KER_animdata_from_id(id);

	KER_animsys_evaluate_animdata(id, adt, 0.0f, ADT_RECALC_ANIM);
}

bool KER_animsys_rna_path_resolve(PointerRNA *ptr, const char *path, int index, PathResolvedRNA *result) {
	if (path == NULL) {
		return false;
	}

	if (!RNA_path_resolve_property(ptr, path, &result->ptr, &result->property)) {
		// fprintf(stderr, "[Kernel] Invalid path, ID = '%s', '%s[%d]'.\n", ptr->owner ? ptr->owner->name + 2 : "<No ID>", path, index);
		return false;
	}

	if (ptr->owner == NULL || !RNA_property_animateable(&result->ptr, result->property)) {
		return false;
	}

	int length = RNA_property_array_length(&result->ptr, result->property);
	if (length && index >= length) {
		// fprintf(stderr, "[Kernel] Invalid array index, ID = '%s', '%s[%d]', array length is %d.\n", ptr->owner ? ptr->owner->name + 2 : "<No ID>", path, index, length - 1);
		return false;
	}

	result->index = length ? index : -1;
	return true;
}
