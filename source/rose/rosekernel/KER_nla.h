#ifndef KER_NLA_H
#define KER_NLA_H

#include "LIB_listbase.h"

struct Action;
struct AnimData;
struct LibraryForeachIDData;
struct NlaStrip;
struct NlaTrack;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Management
 * \{ */

/**
 * Add a NLA Track to the given AnimData.
 * \param prev: NLA-Track to add the new one after.
 */
struct NlaTrack *KER_nla_track_add(struct AnimData *adt, struct NlaTrack *prev);
/**
 * Create a NLA Strip referencing the given Action.
 * 
 * If this is a layered Action, a suitable slot is automatically chosen.
 * If there is non available, no slot will be assigned.
 */
struct NlaStrip *KER_nla_strip_new(struct Action *action, struct ID *animated_id);
/**
 * Create a NLA Strip referencing the given Action & Slot.
 *
 * This can return NULL only when act == NULL or when the slot ID type
 * does not match the given animated ID.
 */
struct NlaStrip *KER_nla_strip_new_for_slot(struct Action *action, int handle, struct ID *animated_id);
/**
 * Add new NLA-strip to the top of the NLA stack - i.e.
 * into the last track if space, or a new one otherwise.
 */
struct NlaStrip *KER_nla_stack_add_strip(struct AnimData *adt, struct ID *animated_id);

/**
 * Copy all NLA data.
 * \param flag: Control ID pointers management, see LIB_ID_CREATE_.../LIB_ID_COPY_...
 */
void KER_nla_tracks_copy(Main *main, ListBase *dst, const ListBase *src, int flag);
/**
 * Copy NLA tracks from #adt_source to #adt_dest, and update the active track/strip pointers to
 * point at those copies.
 */
void KER_nla_tracks_copy_from_adt(struct Main *main, struct AnimData *dst, const struct AnimData *src, int flag);

/**
 * Callback used by lib_query to walk over all ID usages
 * (mimics `foreach_id` callback of #IDTypeInfo structure).
 */
void KER_nla_strip_foreach_id(struct NlaStrip *strip, struct LibraryForeachIDData *data);

/**
 * Remove the given NLA strip from the NLA track it occupies, free the strip's data,
 * and the strip itself.
 */
void KER_nla_strip_free(struct ListBase *strips, struct NlaStrip *strip, bool do_id_user);
/**
 * Remove the given NLA track from the set of NLA tracks, free the track's data,
 * and the track itself.
 */
void KER_nla_track_free(struct ListBase *tracks, struct NlaTrack *track, bool do_id_user);
/**
 * Free the elements of type NLA Tracks provided in the given list, but do not free
 * the list itself since that is not free-standing
 */
void KER_nla_tracks_free(struct ListBase *tracks, bool do_id_user);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Rose Read/Write
 * \{ */

void KER_nla_rose_write(struct RoseWriter *writer, struct ListBase *tracks);
void KER_nla_rose_read_data(struct RoseDataReader *reader, struct ID *id, struct ListBase *tracks);
void KER_nla_rose_read_lib(struct RoseLibReader *reader, struct ID *id, struct ListBase *strips);

/** \} */

/* -------------------------------------------------------------------- */
/** \name NlaTrack
 * \{ */

enum {
	/* convert from global time to strip time - for evaluation */
	NLATIME_CONVERT_EVAL = 0,
	/* convert from global time to strip time - for editing corrections */
	NLATIME_CONVERT_UNMAP,
	/* convert from strip time to global time */
	NLATIME_CONVERT_MAP,
};

/**
 * Return whether this NLA track is enabled.
 *
 * If any track is solo'ed: returns true when this is the solo'ed one.
 * If no track is solo'ed: returns true when this track is not muted.
 */
bool KER_nla_track_is_enabled(const struct AnimData *adt, const struct NlaTrack *track);

/**
 * Add the given NLA-Strip to the given NLA-Track.
 * Calls #KER_nla_strips_add_strip to check if strip can be added.
 */
bool KER_nla_track_add_strip(struct NlaTrack *track, struct NlaStrip *strip);

/**
 * Make the given NLA-track the active one for the given stack. If no track is provided,
 * this function can be used to simply deactivate all the NLA tracks in the given stack too.
 */
void KER_nla_track_set_active(struct ListBase *tracks, struct NlaTrack *track);

/** \} */

/* -------------------------------------------------------------------- */
/** \name NlaStrip
 * \{ */

/**
 * NULL checks incoming strip and verifies no overlap / invalid
 * configuration against other strips in NLA Track before calling
 * #KER_nla_strips_add_strip_unsafe.
 */
bool KER_nla_strips_add_strip(struct ListBase *strips, struct NlaStrip *strip);
/**
 * Add the given NLA-Strip to the given list of strips, assuming that it
 * isn't currently a member of another list, NULL, or conflicting with existing
 * strips position.
 */
void KER_nla_strips_add_strip_unsafe(struct ListBase *strips, struct NlaStrip *strip);

/**
 * Find (and set) a unique name for a strip from the whole AnimData block
 * Uses a similar method to the BLI method, but is implemented differently
 * as we need to ensure that the name is unique over several lists of tracks,
 * not just a single track.
 */
void KER_nla_strip_validate_name(struct AnimData *adt, struct NlaStrip *strip);

/**
 * Recalculate the start and end frames for the current strip, after changing
 * the extents of the action or the mapping (repeats or scale factor) info.
 */
void KER_nla_strip_recalculate_bounds(struct NlaStrip *strip);
/**
 * Recalculate the start and end frames for the strip to match the bounds of its action such that
 * the overall NLA animation result is unchanged.
 */
void KER_nla_strip_recalculate_bounds_sync_action(struct NlaStrip *strip);

/**
 * Compute the length of the passed strip's clip, unless the clip length
 * is zero in which case a non-zero value is returned.
 *
 * WARNING: this function is *very narrow* and special-cased in its
 * application.  It was introduced as part of the fix for issue #107030,
 * as a way to collect a bunch of whack-a-mole inline applications of this
 * logic in one place.  The logic itself isn't principled in any way,
 * and should almost certainly not be used anywhere that it isn't already,
 * short of one of those whack-a-mole inline places being overlooked.
 *
 * The underlying purpose of this function is to ensure that the computed
 * clip length for an NLA strip is (in certain places) never zero, in order to
 * avoid the strip's scale having to be infinity.  In other words, it's a
 * hack.  But at least now it's a hack collected in one place.
 *
 */
float KER_nla_clip_length_get_nonzero(const struct NlaStrip *strip);

/**
 * Ensure the passed range has non-zero length, using the same logic as
 * `KER_nla_clip_length_get_nonzero` to determine the new non-zero length.
 *
 * See the documentation for `KER_nla_clip_length_get_nonzero` for the
 * reason this function exists and the issues around its use.
 *
 * Usage: both `r_start` and `r_end` should already be set to the
 * start/end values of a strip's clip. `r_end` will be modified
 * if necessary to ensure the range is non-zero in length.
 */
void KER_nla_clip_length_ensure_nonzero(const float *r_start, float *r_end);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // !KER_NLA_H
