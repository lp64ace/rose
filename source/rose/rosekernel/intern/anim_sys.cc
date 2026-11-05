#include "MEM_guardedalloc.h"

#include "KER_action.h"
#include "KER_anim_data.h"
#include "KER_anim_sys.h"
#include "KER_idtype.h"
#include "KER_fcurve.h"
#include "KER_mesh.h"
#include "KER_nla.h"
#include "KER_scene.h"

#include "RNA_access.h"
#include "RNA_types.h"
#include "RNA_prototypes.h"

#include "LIB_ghash.h"
#include "LIB_math_rotation.h"
#include "LIB_math_vector.h"
#include "LIB_set.hh"
#include "LIB_utildefines.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_query.h"

#include "nla_private.hh"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name RNA Read/Write Utilities
 * \{ */

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

ROSE_STATIC bool animsys_construct_orig_pointer_rna(const PointerRNA *ptr, PointerRNA *ptr_orig) {
	*ptr_orig = *ptr;
	/**
	 * NOTE: nlastrip_evaluate_controls() creates PointerRNA with ID of nullptr. Technically, this is
	 * not a valid pointer, but there are exceptions in various places of this file which handles
	 * such pointers.
	 * We do special trickery here as well, to quickly go from evaluated to original NlaStrip.
	 * */
	if (ptr->owner == NULL) {
		if (ptr->type != &RNA_NlaStrip) {
			return false;
		}
		NlaStrip *strip = (static_cast<NlaStrip *>(ptr_orig->data));
		if (strip->original != NULL) {
			return false;
		}
		ptr_orig->data = strip->original;
	}
	else {
		ptr_orig->owner = ptr_orig->owner->orig_id;
		ptr_orig->data = ptr_orig->owner;
	}
	return true;
}

ROSE_STATIC void animsys_write_orig_anim_rna_array(PointerRNA *pointer, const char *path, const float *values) {
	PointerRNA ptr_orig;
	if (!animsys_construct_orig_pointer_rna(pointer, &ptr_orig)) {
		return;
	}
	PathResolvedRNA orig_anim_rna;
	if (KER_animsys_rna_path_resolve(&ptr_orig, path, 0, &orig_anim_rna)) {
		RNA_property_float_set_array(&orig_anim_rna.ptr, orig_anim_rna.property, values);
	}
}

ROSE_STATIC void animsys_write_orig_anim_rna(PointerRNA *pointer, const char *path, int index, float value) {
	PointerRNA ptr_orig;
	if (!animsys_construct_orig_pointer_rna(pointer, &ptr_orig)) {
		return;
	}
	PathResolvedRNA orig_anim_rna;
	if (KER_animsys_rna_path_resolve(&ptr_orig, path, index, &orig_anim_rna)) {
		KER_anim_write_to_rna_path(&orig_anim_rna, value, false);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name NLA Animation Data Evaluation
 * \{ */

// NlaEvalStrip

ROSE_STATIC float nla_strip_get_influence(NlaStrip *strip, float cframe) {
	/* sanity checks - normalize the blendin/out values? */
	strip->blend_in = fabsf(strip->blend_in);
	strip->blend_out = fabsf(strip->blend_out);

	/* result depends on where frame is in respect to blendin/out values */
	if (IS_EQF(strip->blend_in, 0.0f) == false && (cframe <= (strip->strip_start + strip->blend_in))) {
		/* there is some blend-in */
		return fabsf(cframe - strip->strip_start) / (strip->blend_in);
	}
	if (IS_EQF(strip->blend_out, 0.0f) == false && (cframe >= (strip->strip_end - strip->blend_out))) {
		/* there is some blend-out */
		return fabsf(strip->strip_end - cframe) / (strip->blend_out);
	}

	/* in the middle of the strip, we should be full strength */
	return 1.0f;
}

ROSE_STATIC void nla_strip_evaluate_controls(NlaStrip *strip, float ctime, const bool flush_to_original) {
	if (!LIB_listbase_is_empty(&strip->strips)) {
		PointerRNA ptr = RNA_pointer_create_discrete(NULL, &RNA_NlaStrip, strip);

		/* execute these settings as per normal */
		rose::Vector<FCurve *> fcurves;
		LISTBASE_FOREACH (FCurve *, fcurve, &strip->fcurves) {
			fcurves.append(fcurve);
		}

		KER_anim_evaluate_fcurves_ex(&ptr, fcurves.data(), fcurves.size(), ctime, flush_to_original);
	}

	/**
	 * analytically generate values for influence and time (if applicable)
	 * - we do this after the F-Curves have been evaluated to override the effects of those
	 *   in case the override has been turned off.
	 */
	if ((strip->flag & NLASTRIP_FLAG_USR_INFLUENCE) == 0) {
		strip->influence = nla_strip_get_influence(strip, ctime);
	}

	/* Bypass evaluation time computation if time mapping is disabled. */
	if ((strip->flag & NLASTRIP_FLAG_NO_TIME_MAP) != 0) {
		strip->time = ctime;
		return;
	}

	if ((strip->flag & NLASTRIP_FLAG_USR_TIME) == 0) {
		strip->time = nla_strip_get_frame(strip, ctime, NLATIME_CONVERT_EVAL);
	}

	/**
	 * If user can control the evaluation time (using F-Curves), consider the option which allows
	 * this time to be clamped to lie within extents of the action-clip, so that a steady changing
	 * rate of progress through several cycles of the clip can be achieved easily.
	 */
	/* NOTE: if we add any more of these special cases, we better group them up nicely... */
	if ((strip->flag & NLASTRIP_FLAG_USR_TIME) && (strip->flag & NLASTRIP_FLAG_USR_TIME_CYCLIC)) {
		strip->time = fmod(strip->time - strip->action_start, strip->action_end - strip->action_start);
	}
}

ROSE_STATIC NlaEvalStrip *nla_strips_ctime_get_strip(ListBase *estrips, ListBase *strips, int index, float ctime, const bool flush_to_original) {
	NlaStrip *strip = NULL;

	int side = NES_TIME_WITHIN;

	LISTBASE_FOREACH (NlaStrip *, itr, strips) {
		bool in_range = IN_RANGE_INCL(ctime, itr->strip_start, itr->strip_end);
		if (itr->flag & NLASTRIP_FLAG_NO_TIME_MAP) {
			switch (itr->extend_mode) {
				case NLASTRIP_EXTEND_HOLD: {
					in_range = true;
				} break;
				case NLASTRIP_EXTEND_HOLD_FORWARD: {
					in_range = ctime >= itr->strip_start;
				} break;
			}
		}

		if (in_range) {
			strip = itr;
			side = NES_TIME_WITHIN;
			break;
		}

		/* if time occurred before current strip... */
    	if (ctime < itr->strip_start) {
			if (itr == strips->first) {
				/* before first strip - only try to use it if it extends backwards in time too */
				if (itr->extend_mode == NLASTRIP_EXTEND_HOLD) {
					strip = itr;
				}

				/* side is 'before' regardless of whether there's a useful strip */
				side = NES_TIME_BEFORE;
			}
			else {
				/**
				 * Before next strip - previous strip has ended, but next hasn't begun,
				 * so blending mode depends on whether strip is being held or not...
				 * - only occurs when no transition strip added, otherwise the transition would have
				 *   been picked up above...
				 */
				if (itr->prev->extend_mode != NLASTRIP_EXTEND_NOTHING) {
					strip = itr->prev;
				}

				side = NES_TIME_AFTER;
			}

			break;
		}

		/* if time occurred after current strip... */
		if (ctime > itr->strip_end) {
			/* only if this is the last strip should we do anything, and only if that is being held */
			if (itr == strips->last) {
				if (itr->extend_mode != NLASTRIP_EXTEND_NOTHING) {
					strip = itr;
				}

				side = NES_TIME_AFTER;
				break;
			}

			/* otherwise, skip... as the 'before' case will catch it more elegantly! */
		}
	}

	if (strip == NULL || (strip->flag & NLASTRIP_FLAG_MUTED) != 0) {
		return NULL;
	}

	if (side == NES_TIME_BEFORE) {
		ctime = strip->strip_start;
	}
	if (side == NES_TIME_AFTER) {
		ctime = strip->strip_end;
	}

	nla_strip_evaluate_controls(strip, ctime, flush_to_original);
	if (strip->influence <= 0.0f) {
		return NULL;
	}

	switch (strip->type) {
		case NLASTRIP_TYPE_CLIP: {
			if (strip->action == NULL) {
				return NULL;
			}
		} break;
		case NLASTRIP_TYPE_TRANSITION: {
			if (ELEM(NULL, strip->prev, strip->next)) {
				return NULL;
			}

			nla_strip_evaluate_controls(strip->prev, strip->strip_start, flush_to_original);
			nla_strip_evaluate_controls(strip->next, strip->strip_end, flush_to_original);
		} break;
	}

	NlaEvalStrip *estrip = (NlaEvalStrip *)MEM_callocN(sizeof(NlaEvalStrip), "NlaEvalStrip");

	estrip->strip = strip;
	estrip->mode = side;
	estrip->index = index;
	estrip->time = strip->time;

	if (estrips) {
		LIB_addtail(estrips, estrip);
	}

	return estrip;
}

ROSE_STATIC NlaEvalStrip *nla_strips_ctime_get_strip_single(ListBase *estrips, NlaStrip *strip, float ctime, const bool flush_to_original) {
	ListBase lb;
	lb.first = lb.last = (void *)strip;

	return nla_strips_ctime_get_strip(estrips, &lb, -1, ctime, flush_to_original);
}

// NlaValidMask

/* Initialize a valid mask, allocating memory if necessary. */
ROSE_STATIC void nla_valid_mask_init(NlaValidMask *mask, int bits) {
	if (ROSE_BITMAP_SIZE(bits) > sizeof(mask->buffer)) {
		mask->ptr = ROSE_BITMAP_NEW(bits, "NlaValidMask");
	}
	else {
		mask->ptr = mask->buffer;
	}
}

/* Free allocated memory for the mask. */
ROSE_STATIC void nla_valid_mask_free(NlaValidMask *mask) {
	if (mask->ptr != mask->buffer) {
		MEM_freeN(mask->ptr);
	}
}

// NlaEvalChannelSnapshot

/* Allocate a new blending value snapshot for the channel. */
ROSE_STATIC NlaEvalChannelSnapshot *nla_eval_chan_snapshot_new(NlaEvalChannel *nec) {
	int length = nec->base_snapshot.length;

	size_t byte_size = sizeof(NlaEvalChannelSnapshot) + sizeof(float) * length;
	NlaEvalChannelSnapshot *nec_snapshot = static_cast<NlaEvalChannelSnapshot *>(MEM_callocN(byte_size, "NlaEvalChannelSnapshot"));

	nec_snapshot->channel = nec;
	nec_snapshot->length = length;
	nla_valid_mask_init(&nec_snapshot->blend_domain, length);
	nla_valid_mask_init(&nec_snapshot->remap_domain, length);

	return nec_snapshot;
}

/* Free a channel's blending value snapshot. */
ROSE_STATIC void nla_eval_chan_snapshot_free(NlaEvalChannelSnapshot *nec_snapshot) {
	ROSE_assert(!nec_snapshot->is_base);

	nla_valid_mask_free(&nec_snapshot->blend_domain);
	nla_valid_mask_free(&nec_snapshot->remap_domain);
	MEM_freeN(nec_snapshot);
}

/* Copy all data in the snapshot. */
ROSE_STATIC void nla_eval_chan_snapshot_copy(NlaEvalChannelSnapshot *dst, const NlaEvalChannelSnapshot *src) {
	ROSE_assert(dst->channel == src->channel);
	memcpy(dst->values, src->values, sizeof(float) * dst->length);
}

// NlaEvalSnapshot

/* Initialize a blending state snapshot structure. */
ROSE_STATIC void nla_eval_snapshot_init(NlaEvalSnapshot *snapshot, NlaEvalData *nlaeval, NlaEvalSnapshot *base) {
	snapshot->base = base;
	snapshot->size = ROSE_MAX(16, nlaeval->totchannels);
	snapshot->channels = (NlaEvalChannelSnapshot **)MEM_callocN(sizeof(NlaEvalChannelSnapshot *) * snapshot->size, "NlaEvalSnapshot::channels");
}

/* Retrieve the individual channel snapshot. */
ROSE_STATIC NlaEvalChannelSnapshot *nla_eval_snapshot_get(NlaEvalSnapshot *snapshot, int index) {
	return (index < snapshot->size) ? snapshot->channels[index] : NULL;
}

/* Ensure at least this number of slots exists. */
ROSE_STATIC void nla_eval_snapshot_ensure_size(NlaEvalSnapshot *snapshot, int size) {
	if (size > snapshot->size) {
		snapshot->size *= 2;
		CLAMP_MIN(snapshot->size, size);
		CLAMP_MIN(snapshot->size, 16);

		size_t byte_size = sizeof(*snapshot->channels) * snapshot->size;
		snapshot->channels = static_cast<NlaEvalChannelSnapshot **>(MEM_recallocN_id(snapshot->channels, byte_size, "NlaEvalSnapshot::channels"));
	}
}

/* Retrieve the address of a slot in the blending state snapshot for this channel (may realloc). */
ROSE_STATIC NlaEvalChannelSnapshot **nla_eval_snapshot_ensure_slot(NlaEvalSnapshot *snapshot, NlaEvalChannel *nec) {
	nla_eval_snapshot_ensure_size(snapshot, nec->owner->totchannels);
	return &snapshot->channels[nec->index];
}

/* Retrieve the blending snapshot for the specified channel, with fallback to base. */
ROSE_STATIC NlaEvalChannelSnapshot *nla_eval_snapshot_find_channel(NlaEvalSnapshot *snapshot, NlaEvalChannel *nec) {
	while (snapshot != nullptr) {
		NlaEvalChannelSnapshot *nec_snapshot = nla_eval_snapshot_get(snapshot, nec->index);
		if (nec_snapshot != nullptr) {
			return nec_snapshot;
		}
		snapshot = snapshot->base;
	}

	return &nec->base_snapshot;
}

/* Retrieve or create the channel value snapshot, copying from the other snapshot (or default values) */
ROSE_STATIC NlaEvalChannelSnapshot *nla_eval_snapshot_ensure_channel(NlaEvalSnapshot *snapshot, NlaEvalChannel *nec) {
  NlaEvalChannelSnapshot **slot = nla_eval_snapshot_ensure_slot(snapshot, nec);

	if (*slot == nullptr) {
		NlaEvalChannelSnapshot *base_snapshot, *nec_snapshot;

		nec_snapshot = nla_eval_chan_snapshot_new(nec);
		base_snapshot = nla_eval_snapshot_find_channel(snapshot->base, nec);

		nla_eval_chan_snapshot_copy(nec_snapshot, base_snapshot);

		*slot = nec_snapshot;
	}

  return *slot;
}

/* Free all memory owned by this blending snapshot structure. */
ROSE_STATIC void nla_eval_snapshot_free_data(NlaEvalSnapshot *snapshot) {
	if (snapshot->channels != NULL) {
		for (int i = 0; i < snapshot->size; i++) {
			NlaEvalChannelSnapshot *nec_snapshot = snapshot->channels[i];
			if (nec_snapshot != NULL) {
				nla_eval_chan_snapshot_free(nec_snapshot);
			}
		}

		MEM_freeN(snapshot->channels);
	}

	snapshot->base = NULL;
	snapshot->size = 0;
	snapshot->channels = NULL;
}

// NlaEvalChannel

/* Free memory owned by this evaluation channel. */
ROSE_STATIC void nla_eval_chan_free_data(NlaEvalChannel *nec) {
	nla_valid_mask_free(&nec->domain);
}

// NlaEvalData

ROSE_STATIC void nla_eval_init(NlaEvalData *nlaeval) {
	memset(nlaeval, 0, sizeof(NlaEvalData));

	nlaeval->path_hash = LIB_ghash_str_new("NlaEvalData::path_hash");
	nlaeval->key_hash = MEM_new<rose::Map<NlaEvalChannelKey, NlaEvalChannel *>>("NlaEvalData::key_hash");
}

ROSE_STATIC void nla_eval_free(NlaEvalData *nlaeval) {
	/* Delete base snapshot - its channels are part of NlaEvalChannel and shouldn't be freed. */
	MEM_SAFE_FREE(nlaeval->base_snapshot.channels);

	nla_eval_snapshot_free_data(&nlaeval->eval_snapshot);

	LISTBASE_FOREACH (NlaEvalChannel *, nec, &nlaeval->channels) {
		nla_eval_chan_free_data(nec);
	}

	LIB_freelistN(&nlaeval->channels);
	LIB_ghash_free(nlaeval->path_hash, NULL, NULL);
	MEM_delete(nlaeval->key_hash);
}

// 

ROSE_STATIC int nla_eval_chan_validate_index(const NlaEvalChannel *nec, int index) {
	if (nec->is_array) {
		if (index >= 0 && index < nec->base_snapshot.length) {
			return index;
		}

		return -1;
	}
	return 0;
}

ROSE_STATIC bool nla_eval_chan_validate_index_ex(const NlaEvalChannel *nec, const int array_index) {
	const int index = nla_eval_chan_validate_index(nec, array_index);

	if (index < 0) {
		ID *id = nec->key.ptr.owner;
		fprintf(stderr, "[Kernel] Invalid path, ID = '%s', '%s'.\n", id ? id->name + 2 : "<No ID>", nec->rna_path);
		return false;
	}
	return true;
}

/* Initialize default values for NlaEvalChannel from the property data. */
ROSE_STATIC void nla_eval_chan_get_default_values(NlaEvalChannel *nec, float *r_values) {
	PointerRNA *ptr = &nec->key.ptr;
	PropertyRNA *prop = nec->key.property;
	int length = nec->base_snapshot.length;

	/* Use unit quaternion for quaternion properties. */
	if (nec->mix == NEC_MIX_QUATERNION) {
		unit_qt(r_values);
		return;
	}
	/* Use all zero for Axis-Angle properties. */
	if (nec->mix == NEC_MIX_AXIS_ANGLE) {
		zero_v4(r_values);
		return;
	}

	if (RNA_property_array_check(prop)) {
		ROSE_assert(length == RNA_property_array_length(ptr, prop));
		bool *tmp_bool;
		int *tmp_int;

		switch (RNA_property_type(prop)) {
			case PROP_BOOLEAN: {
				tmp_bool = static_cast<bool *>(MEM_mallocN(sizeof(bool) * length, __func__));
				RNA_property_boolean_get_default_array(ptr, prop, tmp_bool);
				for (int i = 0; i < length; i++) {
					r_values[i] = float(tmp_bool[i]);
				}
				MEM_freeN(tmp_bool);
			} break;
			case PROP_INT: {
				tmp_int = static_cast<int *>(MEM_mallocN(sizeof(int) * length, __func__));
				RNA_property_int_get_default_array(ptr, prop, tmp_int);
				for (int i = 0; i < length; i++) {
					r_values[i] = float(tmp_int[i]);
				}
				MEM_freeN(tmp_int);
			} break;
			case PROP_FLOAT: {
				RNA_property_float_get_default_array(ptr, prop, r_values);
			} break;
			default: {
				memset(r_values, 0, sizeof(float) * length);
			} break;
		}
	}
	else {
		ROSE_assert(length == 1);

		switch (RNA_property_type(prop)) {
			case PROP_BOOLEAN:
				*r_values = float(RNA_property_boolean_get_default(ptr, prop));
				break;
			case PROP_INT:
				*r_values = float(RNA_property_int_get_default(ptr, prop));
				break;
			case PROP_FLOAT:
				*r_values = RNA_property_float_get_default(ptr, prop);
				break;
			case PROP_ENUM:
				*r_values = float(RNA_property_enum_get_default(ptr, prop));
				break;
			default:
				*r_values = 0.0f;
				break;
		}
	}

	/* Ensure multiplicative properties aren't reset to 0. */
	if (nec->mix == NEC_MIX_MULTIPLY) {
		for (int i = 0; i < length; i++) {
			if (r_values[i] == 0.0f) {
				r_values[i] = 1.0f;
			}
		}
	}
}

ROSE_STATIC char nla_eval_chan_detect_mix_mode(NlaEvalChannelKey *key, int length) {
	ePropertySubType subtype = RNA_property_subtype(key->property);

	if (subtype == PROP_QUATERNION && length == 4) {
		return NEC_MIX_QUATERNION;
	}
	if (subtype == PROP_AXISANGLE && length == 4) {
		return NEC_MIX_AXIS_ANGLE;
	}

	return NEC_MIX_ADD;
}

/* Verify that an appropriate NlaEvalChannel for this property exists. */
ROSE_STATIC NlaEvalChannel *nla_eval_chan_verify_key(NlaEvalData *nlaeval, const char *path, NlaEvalChannelKey *key) {
	return nlaeval->key_hash->lookup_or_add_cb(*key, [&]() {
		/* Create the channel. */
		bool is_array = RNA_property_array_check(key->property);
		int length = is_array ? RNA_property_array_length(&key->ptr, key->property) : 1;

		NlaEvalChannel *nec = static_cast<NlaEvalChannel *>(MEM_callocN(sizeof(NlaEvalChannel) + sizeof(float) * length, "NlaEvalChannel"));

		/* Initialize the channel. */
		nec->rna_path = path;
		new (&nec->key) NlaEvalChannelKey(*key);

		nec->owner = nlaeval;
		nec->index = nlaeval->totchannels++;
		nec->is_array = is_array;

		nec->mix = nla_eval_chan_detect_mix_mode(key, length);

		nla_valid_mask_init(&nec->domain, length);

		nec->base_snapshot.channel = nec;
		nec->base_snapshot.length = length;
		nec->base_snapshot.is_base = true;

		nla_eval_chan_get_default_values(nec, nec->base_snapshot.values);

		/* Store channel in data structures. */
		LIB_addtail(&nlaeval->channels, nec);

		*nla_eval_snapshot_ensure_slot(&nlaeval->base_snapshot, nec) = &nec->base_snapshot;

		return nec;
	});
}

ROSE_STATIC NlaEvalChannel *nla_eval_chan_verify(PointerRNA *pointer, NlaEvalData *nlaeval, const char *path) {
	if (path == NULL) {
		return NULL;
	}

	/* Lookup the path in the path based hash. */
	NlaEvalChannel **p_path_nec;
	bool found_path = LIB_ghash_ensure_p(nlaeval->path_hash, (void *)path, (void ***)&p_path_nec);

	if (found_path) {
		return *p_path_nec;
	}

	/* Cache NULL result for now. */
	*p_path_nec = NULL;

	NlaEvalChannelKey key{};

	if (!RNA_path_resolve_property(pointer, path, &key.ptr, &key.property)) {
		fprintf(stderr, "[Kernel] Invalid path, ID = '%s', '%s'.\n", pointer->owner ? pointer->owner->name + 2 : "<No ID>", path);
		return NULL;
	}

	/* Check that the property can be animated. */
	if (pointer->owner != NULL && !RNA_property_animateable(&key.ptr, key.property)) {
		return NULL;
	}

	NlaEvalChannel *nec = nla_eval_chan_verify_key(nlaeval, path, &key);

	if (nec->rna_path == NULL) {
		nec->rna_path = path;
	}

	return *p_path_nec = nec;
}

//

ROSE_STATIC void nla_snapshot_from_action(PointerRNA *ptr, NlaEvalData *channels, ListBase *modifiers, Action *action, int handle, const float time, NlaEvalSnapshot *r_snapshot) {
	ROSE_assert(modifiers == NULL || LIB_listbase_is_empty(modifiers));

	ActionChannelBag *bag = KER_action_channelbag_for_action_slot_ex(action, handle);
	for (const FCurve *fcurve : rose::Span<FCurve *>(bag->fcurves, bag->totcurve)) {
		NlaEvalChannel *nec = nla_eval_chan_verify(ptr, channels, fcurve->path);

		/* Invalid path or property cannot be animated. */
		if (nec == nullptr) {
			continue;
		}

		if (!nla_eval_chan_validate_index_ex(nec, fcurve->index)) {
			continue;
		}

		NlaEvalChannelSnapshot *necs = nla_eval_snapshot_ensure_channel(r_snapshot, nec);

		const float curval = KER_fcurve_evaluate(NULL, fcurve, time);
		necs->values[fcurve->index] = curval;

		if (nec->mix == NEC_MIX_QUATERNION) {
			LIB_bitmap_set_all(necs->blend_domain.ptr, true, 4);
		}
		else {
			ROSE_BITMAP_ENABLE(necs->blend_domain.ptr, fcurve->index);
		}
	}
}

/**
 * Blend the lower nla stack value and upper strip value of a channel according to mode and influence.
 */
ROSE_STATIC float nla_blend_value(const int blendmode, const float lower_value, const float strip_value, const float influence) {
	/* Optimization: no need to try applying if there is no influence. */
	if (IS_EQF(influence, 0.0f)) {
		return lower_value;
	}

	/* Perform blending. */
	switch (blendmode) {
		/* Simply add the scaled value on to the stack. */
		case NLASTRIP_MODE_ADD:
			return lower_value + (strip_value * influence);
		/* Simply subtract the scaled value from the stack. */
		case NLASTRIP_MODE_SUBTRACT:
			return lower_value - (strip_value * influence);
		/* Multiply the scaled value with the stack. */
		case NLASTRIP_MODE_MULTIPLY:
			return influence * (lower_value * strip_value) + (1 - influence) * lower_value;

		case NLASTRIP_MODE_COMBINE:
			ROSE_assert_msg(0, "combine mode");

		default:
			/**
			 * Do linear interpolation. The influence of the accumulated data
			 * (elsewhere, that is called `dstwegiht`) is 1 - influence,
			 * since the strip's influence is `srcweight`.
			 */
			return lower_value * (1.0f - influence) + (strip_value * influence);
	}
}

/* Blend the lower nla stack value and upper strip value of a channel according to mode and
 * influence. */
ROSE_STATIC float nla_combine_value(const int mix_mode, float base_value, const float lower_value, const float strip_value, const float influence) {
	/* Optimization: No need to try applying if there is no influence. */
	if (IS_EQF(influence, 0.0f)) {
		return lower_value;
	}

	/* Perform blending */
	switch (mix_mode) {
    	case NEC_MIX_ADD:
    	case NEC_MIX_AXIS_ANGLE:
      		return lower_value + (strip_value - base_value) * influence;

		case NEC_MIX_MULTIPLY:
			if (IS_EQF(base_value, 0.0f)) {
				base_value = 1.0f;
			}
			return lower_value * powf(strip_value / base_value, influence);

		default:
			ROSE_assert_msg(0, "invalid mix mode");
			return lower_value;
	}
}

/**
 * Accumulate quaternion channels for Combine mode according to influence.
 * \returns `blended_value = lower_values @ strip_values^infl`
 */
ROSE_STATIC void nla_combine_quaternion(const float lower_values[4], const float strip_values[4], const float influence, float r_blended_value[4]) {
	float tmp_lower[4], tmp_strip_values[4];

	normalize_qt_qt(tmp_lower, lower_values);
	normalize_qt_qt(tmp_strip_values, strip_values);

	pow_qt_fl_normalized(tmp_strip_values, influence);
	mul_qt_qtqt(r_blended_value, tmp_lower, tmp_strip_values);
}

/* Assert that the channels given can be blended or combined together. */
ROSE_STATIC void nla_eval_chan_assert_blend_or_combine_compatible( const NlaEvalChannelSnapshot *lower_necs, const NlaEvalChannelSnapshot *upper_necs, const NlaEvalChannelSnapshot *blended_necs) {
	UNUSED_VARS_NDEBUG(lower_necs, upper_necs, blended_necs);
	ROSE_assert(!ELEM(nullptr, lower_necs, blended_necs));
	ROSE_assert(upper_necs == nullptr || lower_necs->length == upper_necs->length);
	ROSE_assert(lower_necs->length == blended_necs->length);
}

/**
 * Check each remap domain of blended values individually in case animator had a non-combine NLA
 * strip with a subset of quaternion channels and remapping through any of them failed and thus
 * potentially has undefined values.
 *
 * \returns true if case occurred and handled. Returns false if case didn't occur.
 */
ROSE_STATIC bool nlaevalchan_combine_quaternion_handle_undefined_blend_values(NlaEvalChannelSnapshot *blended_necs, NlaEvalChannelSnapshot *upper_or_lower_necs) {
	for (int j = 0; j < 4; j++) {
		if (!ROSE_BITMAP_TEST_BOOL(blended_necs->remap_domain.ptr, j)) {
			LIB_bitmap_set_all(upper_or_lower_necs->remap_domain.ptr, false, 4);
			return true;
		}
	}

	return false;
}

/* Assert that the channels given can be blended or combined together as a quaternion. */
ROSE_STATIC void nla_eval_chan_assert_blend_or_combine_compatible_quaternion( NlaEvalChannelSnapshot *lower_necs, NlaEvalChannelSnapshot *upper_necs, NlaEvalChannelSnapshot *blended_necs) {
	nla_eval_chan_assert_blend_or_combine_compatible(lower_necs, upper_necs, blended_necs);
	ROSE_assert(lower_necs->length == 4);
}

ROSE_STATIC void nla_eval_chan_copy_values(NlaEvalChannelSnapshot *dst, const NlaEvalChannelSnapshot *src) {
	memcpy(dst->values, src->values, src->length * sizeof(float));
}

/**
 * Copies from lower necs to blended necs if upper necs is nullptr or has zero influence.
 * \return true if copied.
 */
ROSE_STATIC bool nla_eval_chan_blend_or_combine_try_copy_from_lower(const NlaEvalChannelSnapshot *lower_necs, const NlaEvalChannelSnapshot *upper_necs, const float upper_influence, NlaEvalChannelSnapshot *r_blended_necs) {
	const bool has_influence = !IS_EQF(upper_influence, 0.0f);
	if (upper_necs != nullptr && has_influence) {
		return false;
	}

	nla_eval_chan_copy_values(r_blended_necs, lower_necs);
	return true;
}

/**
 * Copies to lower necs from blended necs if upper necs is nullptr or has zero influence. If
 * successful, copies blended_necs remap domains to lower_necs.
 *
 * Does not check upper value blend domains.
 *
 * \return true if copied.
 */
static bool nlae_val_chan_blend_or_combine_try_copy_to_lower(NlaEvalChannelSnapshot *blended_necs, NlaEvalChannelSnapshot *upper_necs, const float upper_influence, NlaEvalChannelSnapshot *r_lower_necs) {
	const bool has_influence = !IS_EQF(upper_influence, 0.0f);
	if (upper_necs != nullptr && has_influence) {
		return false;
	}

	nla_eval_chan_copy_values(r_lower_necs, blended_necs);

	/* Must copy remap domain to handle case where some blended values are out of domain. */
	LIB_bitmap_copy_all(r_lower_necs->remap_domain.ptr, blended_necs->remap_domain.ptr, r_lower_necs->length);

	return true;
}

/**
 * Based on blendmode, blend lower necs with upper necs into blended necs.
 *
 * Each upper value's blend domain determines whether to blend or to copy directly
 * from lower.
 */
ROSE_STATIC void nla_eval_chan_blend_value(NlaEvalChannelSnapshot *lower_necs, NlaEvalChannelSnapshot *upper_necs, const int upper_blendmode, const float upper_influence, NlaEvalChannelSnapshot *r_blended_necs) {
	nla_eval_chan_assert_blend_or_combine_compatible(lower_necs, upper_necs, r_blended_necs);
	if (nla_eval_chan_blend_or_combine_try_copy_from_lower(lower_necs, upper_necs, upper_influence, r_blended_necs)) {
		return;
	}

	const int length = lower_necs->length;
	for (int j = 0; j < length; j++) {
		if (!ROSE_BITMAP_TEST_BOOL(upper_necs->blend_domain.ptr, j)) {
			r_blended_necs->values[j] = lower_necs->values[j];
			continue;
		}

		r_blended_necs->values[j] = nla_blend_value(upper_blendmode, lower_necs->values[j], upper_necs->values[j], upper_influence);
	}
}

/**
 * Based on mix-mode, provided by one the necs,
 * combines lower necs with upper necs into blended necs.
 *
 * Each upper value's blend domain determines whether to blend or to copy directly from lower.
 */
ROSE_STATIC void nla_eval_chan_combine_value(NlaEvalChannelSnapshot *lower_necs, NlaEvalChannelSnapshot *upper_necs, const float upper_influence, NlaEvalChannelSnapshot *r_blended_necs) {
	nla_eval_chan_assert_blend_or_combine_compatible(lower_necs, upper_necs, r_blended_necs);
	if (nla_eval_chan_blend_or_combine_try_copy_from_lower(lower_necs, upper_necs, upper_influence, r_blended_necs)) {
		return;
	}

	/* Assumes every base is the same. */
	float *base_values = lower_necs->channel->base_snapshot.values;
	const int length = lower_necs->length;
	const char mix_mode = lower_necs->channel->mix;

	for (int j = 0; j < length; j++) {
		if (!ROSE_BITMAP_TEST_BOOL(upper_necs->blend_domain.ptr, j)) {
			r_blended_necs->values[j] = lower_necs->values[j];
			continue;
		}

    	r_blended_necs->values[j] = nla_combine_value(mix_mode, base_values[j], lower_necs->values[j], upper_necs->values[j], upper_influence);
	}
}

/**
 * Quaternion combines lower necs with upper necs into blended necs.
 *
 * Each upper value's blend domain determines whether to blend or to copy directly
 * from lower.
 */
ROSE_STATIC void nla_eval_chan_combine_quaternion(NlaEvalChannelSnapshot *lower_necs, NlaEvalChannelSnapshot *upper_necs, const float upper_influence, NlaEvalChannelSnapshot *r_blended_necs) {
	nla_eval_chan_assert_blend_or_combine_compatible_quaternion(lower_necs, upper_necs, r_blended_necs);
	if (nla_eval_chan_blend_or_combine_try_copy_from_lower(lower_necs, upper_necs, upper_influence, r_blended_necs)) {
		return;
	}

	/** No need to check per index. We limit to all or nothing combining for quaternions. */
	if (!ROSE_BITMAP_TEST_BOOL(upper_necs->blend_domain.ptr, 0)) {
		nla_eval_chan_copy_values(r_blended_necs, lower_necs);
		return;
	}

	nla_combine_quaternion(lower_necs->values, upper_necs->values, upper_influence, r_blended_necs->values);
}

/**
 * Based on blend-mode and mix-mode, blend lower necs with upper necs into blended necs.
 *
 * Each upper value's blend domain determines whether to blend or to copy directly
 * from lower.
 *
 * \param lower_necs: Never nullptr.
 * \param upper_necs: Can be nullptr.
 * \param upper_blendmode: Enum value in eNlaStrip_Blend_Mode.
 * \param upper_influence: Value in range [0, 1].
 * \param upper_necs: Never nullptr.
 */
ROSE_STATIC void nla_eval_chan_blend_or_combine(NlaEvalChannelSnapshot *lower_necs, NlaEvalChannelSnapshot *upper_necs, const int blend_mode, const float influence, NlaEvalChannelSnapshot *r_blended_necs) {
	ROSE_assert(r_blended_necs != NULL && r_blended_necs->channel != NULL);

	switch (blend_mode) {
		case NLASTRIP_MODE_COMBINE: {
			switch (r_blended_necs->channel->mix) {
				case NEC_MIX_QUATERNION: {
					nla_eval_chan_combine_quaternion(lower_necs, upper_necs, influence, r_blended_necs);
				} break;
				case NEC_MIX_ADD:
				case NEC_MIX_AXIS_ANGLE:
				case NEC_MIX_MULTIPLY: {
					nla_eval_chan_combine_value(lower_necs, upper_necs, influence, r_blended_necs);
				} break;
				default: {
					ROSE_assert_msg(0, "Mix mode should've been handled");
				} break;
			}
		} return;
		case NLASTRIP_MODE_ADD:
		case NLASTRIP_MODE_SUBTRACT:
		case NLASTRIP_MODE_MULTIPLY:
		case NLASTRIP_MODE_REPLACE: {
			nla_eval_chan_blend_value(lower_necs, upper_necs, blend_mode, influence, r_blended_necs);
		} return;
		default:
			ROSE_assert_msg(0, "Blend mode should've been handled");
	}
}

void nla_snapshot_enable_all_blend_domain(NlaEvalSnapshot *snapshot) {
	for (int i = 0; i < snapshot->size; i++) {
		NlaEvalChannelSnapshot *necs = nla_eval_snapshot_get(snapshot, i);
		if (necs == nullptr) {
			continue;
		}

		LIB_bitmap_set_all(necs->blend_domain.ptr, true, necs->length);
	}
}

ROSE_STATIC void nla_snapshot_ensure_channels(NlaEvalData *eval_data, NlaEvalSnapshot *snapshot) {
	LISTBASE_FOREACH (NlaEvalChannel *, nec, &eval_data->channels) {
		nla_eval_snapshot_ensure_channel(snapshot, nec);
	}
}


ROSE_STATIC void nla_snapshot_blend(NlaEvalData *eval_data, NlaEvalSnapshot *lower_snapshot, NlaEvalSnapshot *upper_snapshot, const short upper_blendmode, const float upper_influence, NlaEvalSnapshot *r_blended_snapshot) {
	nla_eval_snapshot_ensure_size(r_blended_snapshot, eval_data->totchannels);

	LISTBASE_FOREACH (NlaEvalChannel *, nec, &eval_data->channels) {
		NlaEvalChannelSnapshot *upper_necs = nla_eval_snapshot_get(upper_snapshot, nec->index);
		NlaEvalChannelSnapshot *lower_necs = nla_eval_snapshot_get(lower_snapshot, nec->index);
		if (upper_necs == NULL && lower_necs == NULL) {
			continue;
		}

		if (lower_necs == NULL) {
			lower_necs = nla_eval_snapshot_find_channel(lower_snapshot->base, nec);
		}

		NlaEvalChannelSnapshot *result_necs = nla_eval_snapshot_ensure_channel(r_blended_snapshot, nec);

		nla_eval_chan_blend_or_combine(lower_necs, upper_necs, upper_blendmode, upper_influence, result_necs);
	}
}

void nla_snapshot_blend_strip(PointerRNA *ptr, NlaEvalData *channels, ListBase *modifiers, NlaEvalStrip *nes, NlaEvalSnapshot *snapshot, float ctime, const bool flush_to_original) {
	nla_strip_evaluate(STRIP_EVAL_BLEND, ptr, channels, modifiers, nes, snapshot, ctime, flush_to_original);
}

ROSE_STATIC void nla_strip_evaluate_actionclip(eNlaStripEvaluateMode mode, PointerRNA *ptr, NlaEvalData *channels, ListBase *modifiers, NlaEvalStrip *nes, NlaEvalSnapshot *snapshot) {
	NlaStrip *strip = nes->strip;

	if (strip == NULL) {
		return;
	}

	if (strip->action == NULL) {
		fprintf(stderr, "[Kernel] NLA-Strip Eval Error: Strip \'%s\' has no Action.\n", strip->name);
		return;
	}

	ListBase tmp;
	LIB_listbase_clear(&tmp);

	ROSE_assert(modifiers == NULL || LIB_listbase_is_empty(modifiers));

	switch (mode) {
		case STRIP_EVAL_BLEND: {
			NlaEvalSnapshot strip_snapshot;
      		nla_eval_snapshot_init(&strip_snapshot, channels, NULL);
			nla_snapshot_from_action(ptr, channels, &tmp, strip->action, strip->handle, strip->time, &strip_snapshot);
			nla_snapshot_blend(channels, snapshot, &strip_snapshot, strip->blend_mode, strip->influence, snapshot);
			nla_eval_snapshot_free_data(&strip_snapshot);
		} break;
		case STRIP_EVAL_BLEND_GET_INVERTED_LOWER_SNAPSHOT: {
			NlaEvalSnapshot strip_snapshot;
      		nla_eval_snapshot_init(&strip_snapshot, channels, NULL);
			nla_snapshot_from_action(ptr, channels, &tmp, strip->action, strip->handle, strip->time, &strip_snapshot);
			ROSE_assert_unreachable();
			nla_eval_snapshot_free_data(&strip_snapshot);
		} break;
		case STRIP_EVAL_NOBLEND: {
			nla_snapshot_from_action(ptr, channels, &tmp, strip->action, strip->handle, strip->time, snapshot);
		} break;
	}
}

ROSE_STATIC void nla_strip_evaluate_transition(eNlaStripEvaluateMode mode, PointerRNA *ptr, NlaEvalData *channels, ListBase *modifiers, NlaEvalStrip *nes, NlaEvalSnapshot *snapshot, const float ftime, const bool flush_to_original) {
	NlaStrip *s1 = nes->strip->prev, *s2 = nes->strip->next;

	ListBase tmp;
	LIB_listbase_clear(&tmp);

	ROSE_assert(modifiers == NULL || LIB_listbase_is_empty(modifiers));

	NlaEvalSnapshot snapshot1, snapshot2;
	NlaEvalStrip tmp_nes;

	switch (mode) {
		case STRIP_EVAL_BLEND: {
			tmp_nes = *nes;

			tmp_nes.mode = NES_TIME_TRANSITION_START;
			tmp_nes.strip = s1;
			tmp_nes.time = s1->time;

			nla_eval_snapshot_init(&snapshot1, channels, snapshot);
			nla_snapshot_blend_strip(ptr, channels, &tmp, &tmp_nes, &snapshot1, ftime, flush_to_original);

			/**
			 * Replace \a snapshot2 nullptr channels with base or default values so all channels blend.
			 */
			nla_snapshot_ensure_channels(channels, &snapshot2);
			/** Mark all \a snapshot2 channel's values to blend. */
			nla_snapshot_enable_all_blend_domain(&snapshot2);
			nla_snapshot_blend(channels, &snapshot1, &snapshot2, NLASTRIP_MODE_REPLACE, nes->time, snapshot);
			nla_eval_snapshot_free_data(&snapshot1);
			nla_eval_snapshot_free_data(&snapshot2);
		} break;
		case STRIP_EVAL_BLEND_GET_INVERTED_LOWER_SNAPSHOT: {
			ROSE_assert_unreachable();
		} break;
		case STRIP_EVAL_NOBLEND: {
			ROSE_assert_msg(0, "This case shouldn't occur. Transitions are assumed to not reference other transitions.");
		} break;
	}
}

void nla_strip_evaluate(eNlaStripEvaluateMode mode, PointerRNA *ptr, NlaEvalData *channels, ListBase *modifiers, NlaEvalStrip *nes, NlaEvalSnapshot *snapshot, float ctime, const bool flush_to_original) {
	NlaStrip *strip = nes->strip;

	if (strip->flag & NLASTRIP_FLAG_EDIT_TOUCHED) {
		return;
	}
	strip->flag |= NLASTRIP_FLAG_EDIT_TOUCHED;

	switch (strip->type) {
		case NLASTRIP_TYPE_CLIP:
			nla_strip_evaluate_actionclip(mode, ptr, channels, modifiers, nes, snapshot);
			break;
		case NLASTRIP_TYPE_TRANSITION:
			nla_strip_evaluate_transition(mode, ptr, channels, modifiers, nes, snapshot, ctime, flush_to_original);
			break;
	}

	strip->flag &= ~NLASTRIP_FLAG_EDIT_TOUCHED;
}

void nla_data_flush_channels(PointerRNA *pointer, NlaEvalData *channels, NlaEvalSnapshot *snapshot, const bool flush_to_original) {
	/* sanity checks */
	if (channels == NULL) {
		return;
	}

	/* for each channel with accumulated values, write its value on the property it affects */
	LISTBASE_FOREACH (NlaEvalChannel *, nec, &channels->channels) {
		/**
		 * The bitmask is set for all channels touched by NLA due to the domain() function.
		 * Channels touched by current set of evaluated strips will have a snapshot channel directly
		 * from the evaluation snapshot.
		 *
		 * This function falls back to the default value if the snapshot channel doesn't exist.
		 * Thus channels, touched by NLA but not by the current set of evaluated strips, will be
		 * reset to default. If channel not touched by NLA then it's value is unchanged.
		 */
		NlaEvalChannelSnapshot *nec_snapshot = nla_eval_snapshot_find_channel(snapshot, nec);

		PathResolvedRNA rna;
		rna.ptr = nec->key.ptr;
		rna.property = nec->key.property;
		rna.index = -1;

		for (int i = 0; i < nec_snapshot->length; i++) {
			if (ROSE_BITMAP_TEST(nec->domain.ptr, i)) {
				float value = nec_snapshot->values[i];
				if (nec->is_array) {
					rna.index = i;
				}
				KER_anim_write_to_rna_path(&rna, value, false);
				if (flush_to_original) {
					animsys_write_orig_anim_rna(pointer, nec->rna_path, rna.index, value);
				}
			}
		}
	}
}

using ActionAndSlot = std::pair<Action *, int>;
using ActionAndSlotSet = rose::Set<ActionAndSlot>;

ROSE_STATIC void nla_eval_domain_action(PointerRNA *pointer, NlaEvalData *channels, Action *act, const int slot_handle, ActionAndSlotSet &touched_actions) {
	const ActionAndSlot action_and_slot(act, slot_handle);
	if (!touched_actions.add(action_and_slot)) {
		return;
	}

	ActionChannelBag *bag = KER_action_channelbag_for_action_slot_ex(act, slot_handle);
	for (const FCurve *fcu : rose::Span<FCurve *>(bag->fcurves, bag->totcurve)) {
		NlaEvalChannel *nec = nla_eval_chan_verify(pointer, channels, fcu->path);

		if (nec != nullptr) {
		/* For quaternion properties, enable all sub-channels. */
			if (nec->mix == NEC_MIX_QUATERNION) {
				LIB_bitmap_set_all(nec->domain.ptr, true, 4);
				continue;
			}

			int idx = nla_eval_chan_validate_index(nec, fcu->index);

			if (idx >= 0) {
				ROSE_BITMAP_ENABLE(nec->domain.ptr, idx);
			}
		}
	}
}

ROSE_STATIC void nla_eval_domain_strips(PointerRNA *pointer, NlaEvalData *channels, ListBase *strips, ActionAndSlotSet &touched_actions) {
	LISTBASE_FOREACH (NlaStrip *, strip, strips) {
		/* Check strip's action. */
		if (strip->action) {
			nla_eval_domain_action(pointer, channels, strip->action, strip->handle, touched_actions);
		}

		/* Check sub-strips (if meta-strips). */
		nla_eval_domain_strips(pointer, channels, &strip->strips, touched_actions);
	}
}

ROSE_STATIC void animsys_evaluate_nla_domain(PointerRNA *pointer, NlaEvalData *channels, AnimData *adt) {
	ActionAndSlotSet touched_actions;

	/* Include domain of Action Track. */
	if (adt->action) {
		nla_eval_domain_action(pointer, channels, adt->action, adt->handle, touched_actions);
	}

	/* NLA Data - Animation Data for Strips */
	LISTBASE_FOREACH (NlaTrack *, track, &adt->nla_tracks) {
		if (!KER_nla_track_is_enabled(adt, track)) {
			continue;
		}
		nla_eval_domain_strips(pointer, channels, &track->strips, touched_actions);
	}
}

ROSE_STATIC void animsys_create_action_track_strip(const AnimData *adt, const bool keyframing_to_strip, NlaStrip *r_action_strip) {
	memset(r_action_strip, 0, sizeof(NlaStrip));

	r_action_strip->action = adt->action;
	r_action_strip->handle = adt->handle;

	if (adt->action) {
		KER_action_slot_get_frame_range(adt->action, adt->handle, &r_action_strip->action_start, &r_action_strip->action_end);
	}
	else {
		r_action_strip->action_start = 0.0f;
		r_action_strip->action_end = 0.0f;
	}
	KER_nla_clip_length_ensure_nonzero(&r_action_strip->action_start, &r_action_strip->action_end);
	r_action_strip->strip_start = r_action_strip->action_start;
	r_action_strip->strip_end = r_action_strip->action_end;

	r_action_strip->blend_mode = adt->action_blend_mode;
	r_action_strip->extend_mode = adt->action_extend_mode;
	r_action_strip->influence = adt->action_influence;

	/* Must set NLASTRIP_FLAG_USR_INFLUENCE, or else the default setting overrides, and influence doesn't work. */
	r_action_strip->flag |= NLASTRIP_FLAG_USR_INFLUENCE | NLASTRIP_FLAG_NO_TIME_MAP;

	const bool solo = (adt->flag & ADT_NLA_SOLO_TRACK) != 0;
	const bool actionstrip_evaluated = r_action_strip->action && !solo;
	if (!actionstrip_evaluated) {
		r_action_strip->flag |= NLASTRIP_FLAG_MUTED;
	}

	/** If we're keyframing, then we must allow keyframing outside fcurve bounds. */
	if (keyframing_to_strip) {
		r_action_strip->extend_mode = NLASTRIP_EXTEND_HOLD;
	}
}

/**
 * Check for special case of non-pushed action being evaluated with no NLA influence (off and no
 * strips evaluated) nor NLA interference (ensure NLA not soloing).
 */
ROSE_STATIC bool is_action_track_evaluated_without_nla(const AnimData *adt, const bool any_strip_evaluated) {
	if (adt->action == NULL) {
		return false;
	}

	if (any_strip_evaluated) {
		return false;
	}

	if ((adt->flag & ADT_NLA_SOLO_TRACK) != 0){ 
		return false;
	}

	/** Allow action track to evaluate as if there isn't any NLA data. */
	return true;
}

/**
 * NLA Evaluation function - values are calculated and stored in temporary "NlaEvalChannels"
 * \param[out] echannels: Evaluation channels with calculated values
 */
ROSE_STATIC bool animsys_evaluate_nla_for_flush(NlaEvalData *echannels, PointerRNA *pointer, const AnimData *adt, float ctime, bool flush_to_original) {
	ListBase estrips;
	NlaEvalStrip *nes;

	LIB_listbase_clear(&estrips);

	bool has_strips = false;

	int track_index = 0;
	LISTBASE_FOREACH_INDEX (NlaTrack *, track, &adt->nla_tracks, track_index) {
		if (!KER_nla_track_is_enabled(adt, track)) {
			continue;
		}

		if (!LIB_listbase_is_empty(&track->strips)) {
			has_strips |= true;
		}

		if ((nes = nla_strips_ctime_get_strip(&estrips, &track->strips, track_index, ctime, flush_to_original))) {
			nes->track = track;
		}

		if (nes) {
			nes->track = track;
		}
	}

	if (is_action_track_evaluated_without_nla(adt, has_strips)) {
		LIB_freelistN(&estrips);
		return false;
	}

	NlaStrip action_strip;
	animsys_create_action_track_strip(adt, false, &action_strip);
	nla_strips_ctime_get_strip_single(&estrips, &action_strip, ctime, flush_to_original);

	/* Per strip, evaluate and accumulate on top of existing channels. */
	LISTBASE_FOREACH (NlaEvalStrip *, nes, &estrips) {
		nla_snapshot_blend_strip(pointer, echannels, NULL, nes, &echannels->eval_snapshot, ctime, flush_to_original);
	}

	/* Free temporary evaluation data that's not used elsewhere. */
	LIB_freelistN(&estrips);

	return true;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Animation Data Evaluation
 * \{ */

void KER_anim_vector_evaluate_fcurves(PathResolvedRNA resolved, FCurve **fcurves, int totcurve, float ctime, float r_vec[4]) {
	ROSE_assert(totcurve <= 4);

	for (int index = 0; index < totcurve; index++) {
		FCurve *fcurve = fcurves[index];

		const int array_index = fcurve->index;
		resolved.index = array_index;
		r_vec[array_index] = KER_fcurve_evaluate(&resolved, fcurve, ctime);
	}

	if (totcurve < 4 && STREQ(RNA_property_identifier(resolved.property), "quaternion")) {
		/* This quaternion was incompletely keyed, so the result is a mixture of the unit quaternion
		 * and values from FCurves. This means that it's almost certainly no longer of unit length. */
		normalize_qt(r_vec);
	}
}

ROSE_INLINE int KER_anim_evaluate_fcurves_optimize(PathResolvedRNA resolved, FCurve **fcurves, int totcurve, float ctime, int index, bool flush_to_original) {
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

		if (flush_to_original) {
			animsys_write_orig_anim_rna_array(&resolved.ptr, fcurves[index]->path, inlinebuf);
		}

		return sequencial - 1;
	}

	return 0;
}

void KER_anim_evaluate_fcurves_ex(PointerRNA *pointer, FCurve **fcurves, int totcurve, float ctime, bool flush_to_original) {
	PathResolvedRNA resolved;
	for (int index = 0; index < totcurve; index++) {
		FCurve *fcurve = fcurves[index];

		if (KER_animsys_rna_curve_resolve(pointer, fcurve, &resolved)) {
			int skip = KER_anim_evaluate_fcurves_optimize(resolved, fcurves, totcurve, ctime, index, flush_to_original);

			/**
			 * We didn't manage to optimize the writting procedure for the property, 
			 * use the old fashion way!
			 */
			if (skip == 0) {
				const float curval = KER_fcurve_evaluate(&resolved, fcurve, ctime);

				KER_anim_write_to_rna_path(&resolved, curval, false);
				if (flush_to_original) {
					animsys_write_orig_anim_rna(pointer, fcurve->path, fcurve->index, curval);
				}
			}

			index += skip;
		}
	}
}

void KER_anim_evaluate_fcurves(PointerRNA *pointer, FCurve **fcurves, int totcurve, float ctime) {
	KER_anim_evaluate_fcurves_ex(pointer, fcurves, totcurve, ctime, false);
}

void KER_anim_evaluate_action(PointerRNA *pointer, Action *action, int slot, float ctime) {
	ActionChannelBag *bag;
	if ((bag = KER_action_channelbag_for_action_slot_ex(action, slot))) {
		KER_anim_evaluate_fcurves(pointer, bag->fcurves, bag->totcurve, ctime);
	}
}

void KER_anim_evaluate_and_apply_action(PointerRNA *pointer, Action *action, int slot, float ctime) {
	KER_anim_evaluate_action(pointer, action, slot, ctime);
}

/**
 * NLA Evaluation function (mostly for use through KER_animsys_eval_animdata)
 * - All channels that will be affected are not cleared anymore. Instead, we just evaluate into
 *   some temp channels, where values can be accumulated in one go.
 *
 * \return whether any NLA tracks were evaluated at all.
 */
bool KER_animsys_calculate_nla(PointerRNA *pointer, AnimData *adt, float ctime, const bool flush_to_original) {
	NlaEvalData echannels;

	nla_eval_init(&echannels);

	/* evaluate the NLA stack, obtaining a set of values to flush */
	const bool did_evaluate_something = animsys_evaluate_nla_for_flush(&echannels, pointer, adt, ctime, flush_to_original);
	if (did_evaluate_something) {
		/* reset any channels touched by currently inactive actions to default value */
		animsys_evaluate_nla_domain(pointer, &echannels, adt);
		/* flush effects of accumulating channels in NLA to the actual data they affect */
		nla_data_flush_channels(pointer, &echannels, &echannels.eval_snapshot, flush_to_original);
	}

	nla_eval_free(&echannels);

	return did_evaluate_something;
}

void KER_animsys_evaluate_animdata(ID *id, AnimData *adt, float ctime, int recalc) {
	if (ELEM(NULL, id, adt)) {
		return;
	}

	PointerRNA idptr = RNA_id_pointer_create(id);

	if (recalc & ADT_RECALC_ANIM) {
		/** Evaluate NLA data */
		bool did_nla_evaluate_anything = false;
		if ((adt->nla_tracks.first) && (adt->flag & ADT_NLA_EVAL_OFF) == 0) {
			did_nla_evaluate_anything = KER_animsys_calculate_nla(&idptr, adt, ctime, true);
		}

		if (!did_nla_evaluate_anything && adt->action != NULL) {
			Action *action = adt->action;

			/**
			 * Action time, the local time within the action that we are to compute!
			 */
			const float aduration = action->frame_end - action->frame_start;
			const float atime = fmod(ctime - adt->stime, aduration);
			const float time = action->frame_start + atime;
			
			if (KER_action_is_layered(adt->action)) {
				KER_anim_evaluate_and_apply_action(&idptr, adt->action, adt->handle, time);
			}
			else {
				KER_anim_evaluate_action(&idptr, adt->action, 0, time);
			}
		}
	}
}

void KER_animsys_eval_animdata(Depsgraph *depsgraph, ID *id) {
	float ctime = DEG_get_ctime(depsgraph);
	AnimData *adt = KER_animdata_from_id(id);

	KER_animsys_evaluate_animdata(id, adt, ctime, ADT_RECALC_ANIM);
}

bool KER_animsys_rna_path_resolve(PointerRNA *pointer, const char *path, int index, PathResolvedRNA *result) {
	if (path == NULL) {
		return false;
	}

	/** The path is not the same as the already resolved one, resolve the property again! */
	if (!RNA_path_resolve_property(pointer, path, &result->ptr, &result->property)) {
		fprintf(stderr, "[Kernel] Invalid path, ID = '%s', '%s[%d]'.\n", pointer->owner ? pointer->owner->name + 2 : "<No ID>", path, index);
		return false;
	}

	if (pointer->owner == NULL || !RNA_property_animateable(&result->ptr, result->property)) {
		return false;
	}

	int length = RNA_property_array_length(&result->ptr, result->property);
	if (length && index >= length) {
		fprintf(stderr, "[Kernel] Invalid array index, ID = '%s', '%s[%d]', array length is %d.\n", pointer->owner ? pointer->owner->name + 2 : "<No ID>", path, index, length - 1);
		return false;
	}

	result->index = length ? index : -1;
	return true;
}

bool KER_animsys_rna_static_path_resolve(PointerRNA *pointer, StaticPathRNA *path, int index, PathResolvedRNA *result) {
	if (path == NULL) {
		return false;
	}

	/** The path is not the same as the already resolved one, resolve the property again! */
	if (!RNA_static_path_resolve_property(pointer, path, &result->ptr, &result->property)) {
		return false;
	}

	if (pointer->owner == NULL || !RNA_property_animateable(&result->ptr, result->property)) {
		return false;
	}

	int length = RNA_property_array_length(&result->ptr, result->property);
	if (length && index >= length) {
		return false;
	}

	result->index = length ? index : -1;
	return true;
}

bool KER_animsys_rna_curve_resolve(PointerRNA *pointer, FCurve *fcurve, PathResolvedRNA *result) {
	if (fcurve->runtime.static_path != NULL) {
		if (KER_animsys_rna_static_path_resolve(pointer, fcurve->runtime.static_path, fcurve->index, result)) {
			return true;
		}

		/**
		 * Fallthrough for better error management since static path cannot be printed!
		 * \note This is costly but it should never happen in production!
		 */
	}
	else {
		/** 
		 * Since each #ActionSlot is tied to a specific IDType we can resolve path as if the starting pointer StructRNA is known!
		 */
		if ((fcurve->flag & FCURVE_DO_STATIC_COMPILATION) != 0) {
			if (RNA_path_can_do_static_compilation(pointer, fcurve->path)) {
				fcurve->runtime.static_path = RNA_path_new(pointer, fcurve->path, &result->ptr, &result->property);

				if (pointer->owner == NULL || !RNA_property_animateable(&result->ptr, result->property)) {
					return false;
				}

				int length = RNA_property_array_length(&result->ptr, result->property);
				if (length && fcurve->index >= length) {
					fprintf(stderr, "[Kernel] Invalid array index, ID = '%s', '%s[%d]', array length is %d.\n", pointer->owner ? pointer->owner->name + 2 : "<No ID>", fcurve->path, fcurve->index, length - 1);
					return false;
				}

				result->index = length ? fcurve->index : -1;
				return true;
			}

			/**
			 * When the #FCURVE_DO_STATIC_COMPILATION flag is enabled we should always be able to do that!
			 * Strop trying to compile this path!
			 */
			fcurve->flag &= ~FCURVE_DO_STATIC_COMPILATION;
		}
	}

	return KER_animsys_rna_path_resolve(pointer, fcurve->path, fcurve->index, result);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Animation Data Iteration
 * \{ */

/* "User-Data" wrapper used by KER_fcurves_main_cb() */
typedef struct AllFCurvesCbWrapper {
	ID_FCurve_Edit_Callback func; /* Operation to apply on F-Curve */
	void *user_data;			  /* Custom data for that operation */
} AllFCurvesCbWrapper;

/* Helper for adt_apply_all_fcurves_cb() - Apply wrapped operator to list of F-Curves */
static void fcurves_listbase_apply_cb(ID *id, ListBase *fcurves, ID_FCurve_Edit_Callback func, void *user_data) {
	FCurve *fcu;

	for (fcu = (FCurve *)fcurves->first; fcu; fcu = fcu->next) {
		func(id, fcu, user_data);
	}
}

static void fcurves_array_apply_cb(ID *id, FCurve **fcurves, int totcurve, ID_FCurve_Edit_Callback func, void *user_data) {
	FCurve **fcu_p;

	for (fcu_p = &fcurves[0]; fcu_p != &fcurves[totcurve]; fcu_p++) {
		func(id, *fcu_p, user_data);
	}
}

static void nla_strips_apply_all_curves_cb(ID *id, ListBase *strips, ID_FCurve_Edit_Callback func, void *user_data) {
	LISTBASE_FOREACH (NlaStrip *, strip, strips) {
		if (strip->action) {
			ActionChannelBag *bag;
			if ((bag = KER_action_channelbag_for_action_slot_ex(strip->action, strip->handle))) {
				fcurves_array_apply_cb(id, bag->fcurves, bag->totcurve, func, user_data);
			}
		}

		nla_strips_apply_all_curves_cb(id, &strip->strips, func, user_data);
	}
}

/* Helper for KER_fcurves_main_cb() - Dispatch wrapped operator to all F-Curves */
static void adt_apply_all_fcurves_cb(ID *id, AnimData *adt, void *wrapper_data) {
	AllFCurvesCbWrapper *wrapper = (AllFCurvesCbWrapper *)wrapper_data;

	if (adt->action) {
		ActionChannelBag *bag;
		if ((bag = KER_action_channelbag_for_action_slot_ex(adt->action, adt->handle))) {
			fcurves_array_apply_cb(id, bag->fcurves, bag->totcurve, wrapper->func, wrapper->user_data);
		}
	}

	LISTBASE_FOREACH (NlaTrack *, track, &adt->nla_tracks) {
		if (!KER_nla_track_is_enabled(adt, track)) {
			continue;
		}

		nla_strips_apply_all_curves_cb(id, &track->strips, wrapper->func, wrapper->user_data);
	}
}

void KER_fcurves_id_cb(struct ID *id, ID_FCurve_Edit_Callback func, void *user_data) {
	AnimData *adt = KER_animdata_from_id(id);
	if (adt != NULL) {
		AllFCurvesCbWrapper wrapper = {
			.func = func,
			.user_data = user_data,
		};
		adt_apply_all_fcurves_cb(id, adt, &wrapper);
	}
}

/** \} */
