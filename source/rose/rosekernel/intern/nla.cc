#include "MEM_guardedalloc.h"

#include "KER_action.h"
#include "KER_fcurve.h"
#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_nla.h"

#include "LIB_string_utils.h"

#include "nla_private.hh"

#include "RLO_read_write.h"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Data Management
 * \{ */

NlaTrack *KER_nla_track_add(AnimData *adt, NlaTrack *prev) {
	if (adt == NULL) {
		return NULL;
	}

	NlaTrack *ntrack = static_cast<NlaTrack *>(MEM_callocN(sizeof(NlaTrack), "NlaTrack"));
	ntrack->flag = NLATRACK_SELECTED;
	ntrack->index = LIB_listbase_count(&adt->nla_tracks);

	if (prev != NULL) {
		LIB_insertlinkafter(&adt->nla_tracks, prev, ntrack);
	}
	else {
		LIB_addtail(&adt->nla_tracks, ntrack);
	}

	KER_nla_track_set_active(&adt->nla_tracks, ntrack);

	LIB_strcpy(ntrack->name, sizeof(ntrack->name), "NlaTrack");
	LIB_uniquename(&adt->nla_tracks, ntrack, "NlaTrack", '.', offsetof(NlaTrack, name), sizeof(ntrack->name));
	return ntrack;
}

ROSE_STATIC NlaStrip *nla_strip_new(Action *action, ID *animated_id) {
	if (action == NULL) {
		return NULL;
	}

	NlaStrip *strip = static_cast<NlaStrip *>(MEM_callocN(sizeof(NlaStrip), "NlaStrip"));

	strip->flag = NLASTRIP_FLAG_SELECT | NLASTRIP_FLAG_SYNC_LENGTH;

	if ((action->flag & ACT_FRAME_RANGE) != 0) {
		strip->flag &= ~NLASTRIP_FLAG_SYNC_LENGTH;
	}

	if (KER_action_is_cyclic(action)) {
		strip->flag |= NLASTRIP_FLAG_USR_TIME_CYCLIC;
	}

	strip->repeat = 1.0f;

	KER_action_assign_nla(strip, action, animated_id);

	return strip;
}

ROSE_STATIC void nla_strip_set_initial_length(NlaStrip *strip) {
	KER_action_slot_get_frame_range(strip->action, strip->handle, &strip->action_start, &strip->action_end);
	KER_nla_clip_length_ensure_nonzero(&strip->action_start, &strip->action_end);
	strip->strip_start = strip->action_start;
	strip->strip_end = strip->action_end;
}

NlaStrip *KER_nla_strip_new(Action *action, ID *animated_id) {
	NlaStrip *strip = nla_strip_new(action, animated_id);
	if (!strip) {
		return NULL;
	}

	nla_strip_set_initial_length(strip);

	return strip;
}

NlaStrip *KER_nla_strip_new_for_slot(Action *action, int handle, ID *animated_id) {
	NlaStrip *strip = nla_strip_new(action, animated_id);
	if (!strip) {
		return NULL;
	}

	if (!KER_action_assign_nla_ex(strip, action, handle, animated_id)) {
		KER_nla_strip_free(NULL, strip, true);
	}

	nla_strip_set_initial_length(strip);

	return strip;
}

NlaStrip *KER_nla_stack_add_strip(AnimData *adt, ID *animated_id) {
	if (ELEM(NULL, adt, adt->action)) {
		return NULL;
	}

	NlaStrip *strip = KER_nla_strip_new_for_slot(adt->action, adt->handle, animated_id);
	if (strip == NULL) {
		return NULL;
	}

	/* firstly try adding strip to last track, but if that fails, add to a new track */
	if (KER_nla_track_add_strip(static_cast<NlaTrack *>(adt->nla_tracks.last), strip)) {
		NlaTrack *track = KER_nla_track_add(adt, NULL);
		KER_nla_track_set_active(&adt->nla_tracks, track);
		KER_nla_track_add_strip(track, strip);
		LIB_strcpy(track->name, sizeof(track->name), KER_id_name(&adt->action->id));
	}

	KER_nla_strip_validate_name(adt, strip);

	return strip;
}

NlaStrip *KER_nla_strip_copy(Main *main, const NlaStrip *strip, const bool use_same_actions, const int flag) {
	const bool do_id_user = (flag & LIB_ID_CREATE_NO_USER_REFCOUNT) == 0;

	if (strip == NULL) {
		return NULL;
	}

	NlaStrip *new_strip = static_cast<NlaStrip *>(MEM_dupallocN(strip));
	new_strip->prev = new_strip->next = NULL;

	if (new_strip->action) {
		if (use_same_actions) {
			if (do_id_user) {
				id_us_add(&new_strip->action->id);
			}
		}
		else {
			KER_id_copy_ex(main, &new_strip->action->id, reinterpret_cast<ID **>(&new_strip->action), flag);
		}
	}

	KER_fcurves_copy(&new_strip->fcurves, &strip->fcurves);
	
	ROSE_assert(LIB_listbase_is_empty(&new_strip->modifiers));
	ROSE_assert(LIB_listbase_is_empty(&strip->modifiers));

	LIB_listbase_clear(&new_strip->strips);

	LISTBASE_FOREACH (const NlaStrip *, substrip, &strip->strips) {
		NlaStrip *new_substrip = KER_nla_strip_copy(main, substrip, use_same_actions, flag);
		LIB_addtail(&new_strip->strips, new_substrip);
	}

	return new_strip;
}

NlaTrack *KER_nla_track_copy(Main *main, const NlaTrack *track, const bool use_same_actions, const int flag) {
	if (track == NULL) {
		return NULL;
	}

	NlaTrack *new_track = static_cast<NlaTrack *>(MEM_dupallocN(track));
	new_track->prev = new_track->next = NULL;

	LIB_listbase_clear(&new_track->strips);

	LISTBASE_FOREACH (NlaStrip *, strip, &track->strips) {
		NlaStrip *new_strip = KER_nla_strip_copy(main, strip, use_same_actions, flag);
		LIB_addtail(&new_track->strips, new_strip);
	}

	return new_track;
}

void KER_nla_tracks_copy(Main *main, ListBase *dst, const ListBase *src, int flag) {
	if (ELEM(NULL, dst, src)) {
		return;
	}

	LIB_listbase_clear(dst);

	LISTBASE_FOREACH (const NlaTrack *, track, src) {
		NlaTrack *new_track = KER_nla_track_copy(main, track, true, flag);
		LIB_addtail(dst, new_track);
	}
}

void KER_nla_tracks_copy_from_adt(Main *main, AnimData *dst, const AnimData *src, int flag) {
	KER_nla_tracks_copy(main, &dst->nla_tracks, &src->nla_tracks, flag);
}

void KER_nla_strip_foreach_id(NlaStrip *strip, struct LibraryForeachIDData *data) {
	KER_LIB_FOREACHID_PROCESS_IDSUPER(data, strip->action, IDWALK_CB_USER);

	LISTBASE_FOREACH (NlaStrip *, substrip, &strip->strips) {
		KER_LIB_FOREACHID_PROCESS_FUNCTION_CALL(data, KER_nla_strip_foreach_id(substrip, data));
	}
}

void KER_nla_strip_free(ListBase *strips, NlaStrip *strip, bool do_id_user) {
	if (strip == NULL) {
		return;
	}

	LISTBASE_FOREACH_MUTABLE (NlaStrip *, child_strip, &strip->strips) {
		KER_nla_strip_free(&strip->strips, child_strip, do_id_user);
	}

	if (strip->action != NULL && do_id_user) {
		id_us_rem(&strip->action->id);
	}

	KER_fcurves_free(&strip->fcurves);
	KER_fcurve_modifiers_free(&strip->modifiers);
	
	if (strips) {
		LIB_freelinkN(strips, strip);
	}
	else {
		MEM_freeN(strip);
	}
}

void KER_nla_track_free(ListBase *tracks, NlaTrack *track, bool do_id_user) {
	if (track == NULL) {
		return;
	}

	LISTBASE_FOREACH_MUTABLE(NlaStrip *, strip, &track->strips) {
		KER_nla_strip_free(&track->strips, strip, do_id_user);
	}

	if (tracks) {
		LIB_freelinkN(tracks, track);
	}
	else {
		MEM_freeN(track);
	}
}

void KER_nla_tracks_free(ListBase *tracks, bool do_id_user) {
	if (ELEM(NULL, tracks, tracks->first)) {
		return;
	}

	LISTBASE_FOREACH_MUTABLE(NlaTrack *, track, tracks) {
		KER_nla_track_free(tracks, track, do_id_user);
	}

	LIB_listbase_clear(tracks);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Rose Read/Write
 * \{ */

ROSE_STATIC void rose_write_nla_strips(RoseWriter *writer, ListBase *strips) {
	LISTBASE_FOREACH (NlaStrip *, strip, strips) {
		RLO_write_struct(writer, NlaStrip, strip);

		KER_fcurves_rose_write_data(writer, &strip->fcurves);
		// KER_fmodifiers_rose_write_data(writer, &strip->modifiers);

		rose_write_nla_strips(writer, &strip->strips);
	}
}

void KER_nla_rose_write(RoseWriter *writer, ListBase *tracks) {
	LISTBASE_FOREACH (NlaTrack *, track, tracks) {
		RLO_write_struct(writer, NlaTrack, track);

		rose_write_nla_strips(writer, &track->strips);
	}
}

ROSE_STATIC void rose_data_read_nla_strips(RoseDataReader *reader, ListBase *strips) {
	LISTBASE_FOREACH (NlaStrip *, strip, strips) {
		RLO_read_struct_list(reader, NlaStrip, &strip->strips);
		rose_data_read_nla_strips(reader, &strip->strips);

		RLO_read_struct_list(reader, FCurve, &strip->fcurves);
		KER_fcurve_rose_read_data_listbase(reader, &strip->fcurves);

		RLO_read_struct_list(reader, FModifier, &strip->modifiers);
		// KER_fmodifier_rose_read_data_listbase(reader, &strip->fcurves);
	}
}

void KER_nla_rose_read_data(RoseDataReader *reader, ID *id, ListBase *tracks) {
	LISTBASE_FOREACH (NlaTrack *, track, tracks) {
		RLO_read_struct_list(reader, NlaStrip, &track->strips);

		rose_data_read_nla_strips(reader, &track->strips);
	}
}

void KER_nla_rose_read_lib(RoseLibReader *reader, ID *id, ListBase *strips) {
	LISTBASE_FOREACH (NlaStrip *, strip, strips) {
		RLO_read_id_address(reader, id->lib, &strip->action);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name NlaTrack
 * \{ */

bool KER_nla_track_is_enabled(const AnimData *adt, const NlaTrack *track) {
	if (adt->flag & ADT_NLA_SOLO_TRACK) {
		return track->flag & NLATRACK_SOLO;
	}

	return (track->flag & NLATRACK_MUTED) == 0;
}

bool KER_nla_track_add_strip(NlaTrack *track, NlaStrip *strip) {
	if (ELEM(NULL, track, strip)) {
		return false;
	}

	/* try to add the strip to the track using a more generic function */
	return KER_nla_strips_add_strip(&track->strips, strip);
}

void KER_nla_track_set_active(ListBase *tracks, NlaTrack *track) {
	if (ELEM(NULL, tracks, tracks->first)) {
		return;
	}

	LISTBASE_FOREACH(NlaTrack *, itr, tracks) {
		itr->flag &= ~NLATRACK_ACTIVE;
	}

	if (track) {
		track->flag |= NLATRACK_ACTIVE;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name NlaStrip
 * \{ */

ROSE_INLINE float nlastrip_get_frame_transition(NlaStrip *strip, float cframe, short mode) {
	float length = strip->strip_end - strip->strip_start;

	if (mode == NLATIME_CONVERT_MAP) {
		return (length * cframe) / length;
	}

	return (cframe - strip->strip_start) / length;
}

ROSE_INLINE float nla_strip_get_frame_actionclip(NlaStrip *strip, float cframe, short mode) {
	if (IS_EQF(strip->repeat, 0.0f)) {
		strip->repeat = 1.0f;
	}

	const float actlength = KER_nla_clip_length_get_nonzero(strip);

	if (mode == NLATIME_CONVERT_MAP) {
		return strip->strip_start + (cframe - strip->action_start);
	}
	if (mode == NLATIME_CONVERT_UNMAP) {
		return strip->action_start + (cframe - strip->strip_start);
	}

	if (IS_EQF(cframe, strip->strip_end) && IS_EQF(strip->repeat, floorf(strip->repeat))) {
		/**
		 * This case prevents the motion snapping back to the first frame at the end of the strip
		 * by catching the case where repeats is a whole number, which means that the end of the
		 * strip could also be interpreted as the end of the start of a repeat.
		 */
    	return strip->action_end;
	}
	
    return strip->action_start + fmodf(cframe - strip->strip_start, actlength);
}

float nla_strip_get_frame(NlaStrip *strip, float cframe, short mode) {
	switch (strip->type) {
		case NLASTRIP_TYPE_TRANSITION:
			return nlastrip_get_frame_transition(strip, cframe, mode);
		case NLASTRIP_TYPE_CLIP:
			return nla_strip_get_frame_actionclip(strip, cframe, mode);
	}

	ROSE_assert_unreachable();

	return 0.0f;
}

bool KER_nla_strips_has_space(ListBase *strips, float start, float end) {
	if (strips == NULL || IS_EQF(start, end)) {
		return false;
	}
	if (start > end) {
		fprintf(stderr, "[Kernel] Error #KER_nla_strips_has_space, start and end arguments swapped.\n");
		SWAP(float, start, end);
	}

	LISTBASE_FOREACH (NlaStrip *, strip, strips) {
		/**
		 * If start frame of strip is past the target end-frame, 
		 * that means that we've gone past the window we need to check for.
		 */
		if (strip->strip_start >= end) {
			return true;
		}

		/**
		 * If the end of the strip is greater than either of the boundaries, the range must fall within the extends of the strip
		 */
		if ((strip->strip_end > start) || (strip->strip_end > end)) {
			return false;
		}
	}

	/* if we are still here, we haven't encountered any overlapping strips */
	return true;
}

bool KER_nla_strips_add_strip(ListBase *strips, NlaStrip *strip) {
	if (ELEM(NULL, strips, strip)) {
		return false;
	}

	if (!KER_nla_strips_has_space(strips, strip->strip_start, strip->strip_end)) {
		return false;
	}

	KER_nla_strips_add_strip_unsafe(strips, strip);

	return true;
}

void KER_nla_strips_add_strip_unsafe(ListBase *strips, NlaStrip *strip) {
	bool not_added = true;

	ROSE_assert(!ELEM(NULL, strips, strip));

	/* find the right place to add the strip to the nominated track */
	LISTBASE_FOREACH (NlaStrip *, itr, strips) {
		if (itr->strip_start >= strip->strip_start) {
			LIB_insertlinkbefore(strips, itr, strip);
			not_added = false;
			break;
		}
	}

	if (not_added) {
		LIB_addtail(strips, strip);
	}
}

ROSE_INLINE int strip_cmp(const void *vleft, const void *vright, void *userdata) {
	NlaStrip *left = (NlaStrip *)vleft, *right = (NlaStrip *)vright;

	// if (IS_EQF(left->strip_start, right->strip_end)) {
	// 	return 0;
	// }

	return (int)ceilf(left->strip_start - right->strip_start);
}

void KER_nla_strips_sort_strips(ListBase *strips) {
	if (ELEM(NULL, strips, strips->first)) {
		return;
	}

#if 0
	/** Even 16 are too many strips, these should be fairly optimized...! */
	ROSE_assert(LIB_listbase_count(strips) <= 16);

	ListBase tmp;
	LIB_listbase_clear(&tmp);

	LISTBASE_FOREACH_MUTABLE(NlaStrip *, strip, strips) {
		bool not_added = true;

		LIB_remlink(strips, strip);

		LISTBASE_FOREACH_BACKWARD (NlaStrip *, sstrip, &tmp) {
			if (sstrip->strip_start <= strip->strip_start) {
				LIB_insertlinkafter(&tmp, sstrip, strip);
				not_added = false;
				break;
			}
		}

		if (not_added) {
			LIB_addhead(&tmp, strip);
		}
	}

	LIB_listbase_swap(strips, &tmp);
#else
	/** Even 255 are too many strips, these should be fairly optimized...! */
	ROSE_assert(LIB_listbase_count(strips) <= 255);

	LIB_listbase_sort(strips, strip_cmp, NULL);
#endif
}


ROSE_STATIC bool nla_strip_validate_name_cb(void *arg, const char *name) {
	GHash *gh = (GHash *)arg;

	return LIB_ghash_haskey(gh, name);
}

void KER_nla_strip_validate_name(AnimData *adt, NlaStrip *strip) {
	if (ELEM(NULL, adt, strip)) {
		return;
	}

	if (strip->name[0] == 0) {
		switch (strip->type) {
			case NLASTRIP_TYPE_CLIP: {
				LIB_strcpy(strip->name, sizeof(strip->name), strip->action ? KER_id_name(&strip->action->id) : "<No Action>");
			} break;
			case NLASTRIP_TYPE_TRANSITION: {
				LIB_strcpy(strip->name, sizeof(strip->name), "Transition");
			} break;
			default: {
				LIB_strcpy(strip->name, sizeof(strip->name), "NLA Strip");
			} break;
		}
	}

	GHash *gh = LIB_ghash_str_new(__func__);

	LISTBASE_FOREACH (NlaTrack *, track, &adt->nla_tracks) {
		LISTBASE_FOREACH (NlaStrip *, itr, &track->strips) {
			if (strip == itr) {
				continue;
			}

			LIB_ghash_insert(gh, itr->name, itr);
		}
	}

	LIB_uniquename_cb(nla_strip_validate_name_cb, gh, "NlaStrip", '.', strip->name, sizeof(strip->name));

	LIB_ghash_free(gh, NULL, NULL);
}

ROSE_STATIC void nla_strip_fix_resize_overlaps(NlaStrip *strip) {
	/* next strips - do this first, since we're often just getting longer */
	if (strip->next) {
		NlaStrip *nls = strip->next;
		float offset = 0.0f;

		if (nls->type == NLASTRIP_TYPE_TRANSITION) {
			/**
			 * Transition strips should grow/shrink to accommodate the resized strip,
			 * but if the strip's bounds now exceed the transition, we're forced to
			 * offset everything to maintain the balance
			 */
			if (strip->strip_end <= nls->strip_start) {
				/* grow the transition to fill the void */
				nls->strip_start = strip->strip_end;
			}
			else if (strip->strip_end < nls->strip_end) {
				/* shrink the transition to give the strip room */
				nls->strip_start = strip->strip_end;
			}
			else {
				/**
				 * Shrink transition down to 1 frame long (so that it can still be found),
				 * then offset everything else by the remaining deficit to give the strip room.
				 */
				nls->strip_start = nls->strip_end - 1.0f;

				/* XXX: review whether preventing fractional values is good here... */
				offset = ceilf(strip->strip_end - nls->strip_start);

				/* apply necessary offset to ensure that the strip has enough space */
				for (; nls; nls = nls->next) {
					nls->strip_start += offset;
					nls->strip_end += offset;
				}
			}
		}
		else if (strip->strip_end > nls->strip_start) {
			/* NOTE: need to ensure we don't have a fractional frame offset, even if that leaves a gap,
			* otherwise it will be very hard to get rid of later
			*/
			offset = ceilf(strip->strip_end - nls->strip_start);

			/* apply to times of all strips in this direction */
			for (; nls; nls = nls->next) {
				nls->strip_start += offset;
				nls->strip_end += offset;
			}
		}
	}

	/* previous strips - same routine as before */
	/* NOTE: when strip bounds are recalculated, this is not considered! */
	if (strip->prev) {
		NlaStrip *nls = strip->prev;
		float offset = 0.0f;

		if (nls->type == NLASTRIP_TYPE_TRANSITION) {
			/**
			 * Transition strips should grow/shrink to accommodate the resized strip,
			 * but if the strip's bounds now exceed the transition, we're forced to
			 * offset everything to maintain the balance
			 */
			if (strip->strip_start >= nls->strip_end) {
				/* grow the transition to fill the void */
				nls->strip_end = strip->strip_start;
			}
			else if (strip->strip_start > nls->strip_start) {
				/* shrink the transition to give the strip room */
				nls->strip_end = strip->strip_start;
			}
			else {
				/* Shrink transition down to 1 frame long (so that it can still be found),
				* then offset everything else by the remaining deficit to give the strip room. */
				nls->strip_end = nls->strip_start + 1.0f;

				/* XXX: review whether preventing fractional values is good here... */
				offset = ceilf(nls->strip_end - strip->strip_start);

				/* apply necessary offset to ensure that the strip has enough space */
				for (; nls; nls = nls->prev) {
					nls->strip_start -= offset;
					nls->strip_end -= offset;
				}
			}
		}
		else if (strip->strip_start < nls->strip_end) {
			/* NOTE: need to ensure we don't have a fractional frame offset, even if that leaves a gap,
			* otherwise it will be very hard to get rid of later
			*/
			offset = ceilf(nls->strip_end - strip->strip_start);

			/* apply to times of all strips in this direction */
			for (; nls; nls = nls->prev) {
				nls->strip_start -= offset;
				nls->strip_end -= offset;
			}
		}
	}
}

void KER_nla_strip_recalculate_bounds(struct NlaStrip *strip) {
	float mapping;

	if ((strip == NULL) || (strip->type != NLASTRIP_TYPE_CLIP)) {
		return;
	}

	/* calculate new length factors */
	const float actlen = KER_nla_clip_length_get_nonzero(strip);

	mapping = strip->repeat;

	/* adjust endpoint of strip in response to this */
	if (IS_EQF(mapping, 0.0f) == 0) {
		strip->strip_end = (actlen * mapping) + strip->strip_start;
	}

	/* make sure we don't overlap our neighbors */
	nla_strip_fix_resize_overlaps(strip);
}

void KER_nla_strip_recalculate_bounds_sync_action(struct NlaStrip *strip) {
	float prev_actstart;

	if (strip == nullptr || strip->type != NLASTRIP_TYPE_CLIP) {
		return;
	}

	prev_actstart = strip->action_start;

	KER_action_slot_get_frame_range(strip->action, strip->handle, &strip->action_start, &strip->action_end);
	KER_nla_clip_length_ensure_nonzero(&strip->action_start, &strip->action_end);

	/* Set start such that key's do not visually move, to preserve the overall animation result. */
	strip->strip_start += (strip->action_start - prev_actstart);

	KER_nla_strip_recalculate_bounds(strip);
}

float KER_nla_clip_length_get_nonzero(const NlaStrip *strip) {
	if (strip->action_end <= strip->action_start) {
		return 1.0f;
	}
	return strip->action_end - strip->action_start;
}

void KER_nla_clip_length_ensure_nonzero(const float *r_start, float *r_end) {
	if (*r_end <= *r_start) {
		*r_end = *r_start + 1.0f;
	}
}

/** \} */
