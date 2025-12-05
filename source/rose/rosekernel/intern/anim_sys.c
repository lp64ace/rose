#include "MEM_guardedalloc.h"

#include "KER_action.h"
#include "KER_anim_data.h"
#include "KER_anim_sys.h"
#include "KER_idtype.h"
#include "KER_fcurve.h"
#include "KER_mesh.h"
#include "KER_scene.h"

#include "RNA_access.h"
#include "RNA_types.h"

#include "LIB_math_rotation.h"
#include "LIB_math_vector.h"
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

void KER_anim_vector_evaluate_fcurves(PathResolvedRNA resolved, FCurve **fcurves, int totcurve, float ctime, float r_vec[4]) {
	ROSE_assert(totcurve <= 4);

	bool is_quat = false;
	if (STREQ(RNA_property_identifier(resolved.property), "quaternion")) {
		is_quat = true;
	}

	for (int index = 0; index < totcurve; index++) {
		FCurve *fcurve = fcurves[index];

		const int array_index = fcurve->index;
		resolved.index = array_index;
		r_vec[array_index] = KER_fcurve_evaluate(&resolved, fcurve, ctime);
	}

	if (is_quat && totcurve < 4) {
		/* This quaternion was incompletely keyed, so the result is a mixture of the unit quaternion
		 * and values from FCurves. This means that it's almost certainly no longer of unit length. */
		normalize_qt(r_vec);
	}
}

ROSE_INLINE int KER_anim_evaluate_fcurves_optimize(PathResolvedRNA resolved, FCurve **fcurves, int totcurve, float ctime, int index) {
	if (RNA_property_type(resolved.property) != PROP_FLOAT) {
		return 0;
	}

	float inlinebuf[4];

	int totarray = RNA_property_array_length(&resolved.ptr, resolved.property);
	if (1 < totarray && totarray <= 4) {
		int sequencial = 1;

		/**
		 * This optimization is faster than the cache optimization but it 
		 * requires that the indexed properties are sequencial to one another
		 */
		while (sequencial < totarray && index + sequencial < totcurve) {
			if (!STREQ(fcurves[index]->path, fcurves[index + sequencial]->path)) {
				break;
			}

			sequencial++;
		}

		if (sequencial <= 1) {
			return 0;
		}

		RNA_property_float_get_array(&resolved.ptr, resolved.property, inlinebuf);
		KER_anim_vector_evaluate_fcurves(resolved, &fcurves[index], sequencial, ctime, inlinebuf);
		RNA_property_float_set_array(&resolved.ptr, resolved.property, inlinebuf);

		return sequencial - 1;
	}

	return 0;
}

void KER_anim_evaluate_fcurves(PointerRNA pointer, FCurve **fcurves, int totcurve, float ctime) {
	PathResolvedRNA resolved;
	for (int index = 0; index < totcurve; index++) {
		FCurve *fcurve = fcurves[index];

		if (KER_animsys_rna_path_resolve(&pointer, fcurve->path, fcurve->index, &resolved)) {
			int skip = KER_anim_evaluate_fcurves_optimize(resolved, fcurves, totcurve, ctime, index);

			/**
			 * We didn't manage to optimize the writting procedure for the property, 
			 * use the old fashion way!
			 */
			if (skip == 0) {
				const float curval = KER_fcurve_evaluate(&resolved, fcurve, ctime);

				KER_anim_write_to_rna_path(&resolved, curval, false);
			}

			index += skip;
		}
	}
}

void KER_anim_evaluate_action(PointerRNA pointer, Action *action, int slot, float ctime) {
	ActionChannelBag *bag;
	if ((bag = KER_action_channelbag_for_action_slot_ex(action, slot))) {
		KER_anim_evaluate_fcurves(pointer, bag->fcurves, bag->totcurve, ctime);
	}
}

void KER_anim_evaluate_and_apply_action(PointerRNA pointer, Action *action, int slot, float ctime) {
	KER_anim_evaluate_action(pointer, action, slot, ctime);
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

void KER_animsys_eval_animdata(Scene *scene, ID *id) {
	AnimData *adt = KER_animdata_from_id(id);

	KER_animsys_evaluate_animdata(id, adt, KER_scene_frame(scene), ADT_RECALC_ANIM);
}

bool KER_animsys_rna_path_resolve_ex(PointerRNA *ptr, const char *path, int index, PathResolvedRNA *result, GHash *cache) {
	if (path == NULL) {
		return false;
	}

	PathResolvedRNA *cached = NULL;
	if (!cache || !(cached = LIB_ghash_lookup(cache, path))) {
		/** The path is not the same as the already resolved one, resolve the property again! */
		if (!RNA_path_resolve_property(ptr, path, &result->ptr, &result->property)) {
			fprintf(stderr, "[Kernel] Invalid path, ID = '%s', '%s[%d]'.\n", ptr->owner ? ptr->owner->name + 2 : "<No ID>", path, index);
			return false;
		}

		if (cache) {
			cached = MEM_mallocN(sizeof(PathResolvedRNA), "PathResolvedRNA");

			cached->property = result->property;
			cached->ptr = result->ptr;

			LIB_ghash_insert(cache, path, cached);
		}
	}
	else if (cache) {
		result->property = cached->property;
		result->ptr = cached->ptr;
	}

	if (ptr->owner == NULL || !RNA_property_animateable(&result->ptr, result->property)) {
		return false;
	}

	int length = RNA_property_array_length(&result->ptr, result->property);
	if (length && index >= length) {
		fprintf(stderr, "[Kernel] Invalid array index, ID = '%s', '%s[%d]', array length is %d.\n", ptr->owner ? ptr->owner->name + 2 : "<No ID>", path, index, length - 1);
		return false;
	}

	result->index = length ? index : -1;
	return true;
}

bool KER_animsys_rna_path_resolve(PointerRNA *ptr, const char *path, int index, PathResolvedRNA *result) {
	return KER_animsys_rna_path_resolve_ex(ptr, path, index, result, NULL);
}
