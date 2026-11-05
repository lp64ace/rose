#ifndef DNA_ANIM_TYPES_H
#define DNA_ANIM_TYPES_H

#include "DNA_ID.h"

#include "DNA_curve_types.h"

struct Action;
struct ActionGroup;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Modifiers
 * \{ */

typedef struct FModifier {
	struct FModifier *prev, *next;

	
} FModifier;

/** \} */

/* -------------------------------------------------------------------- */
/** \name FCurves
 * \{ */

typedef struct FPoint {
	/** Time + Value */
	float vec[2];
} FPoint;


typedef struct FCurve_Runtime {
 	struct StaticPathRNA *static_path;
} FCurve_Runtime;

typedef struct FCurve {
	struct FCurve *prev, *next;

	struct ActionGroup *group;

	BezTriple *bezt;
	/** Baked or imported motion samples (array). */
	FPoint *fpt;
	/** Total number of points which define the curve (i.e. size of arrays in FPoints). */
	int totvert;
	int flag;

	/**
	 * RNA-path to resolve data-access, see: #RNA_path_resolve_property.
	 *
	 * \note String look-ups for collection and custom-properties are escaped using #LIB_str_escape.
	 */
	char *path;
	int index;

	FCurve_Runtime runtime;
} FCurve;

enum {
	FCURVE_DO_STATIC_COMPILATION = 1 << 0,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name NLA Strips
 * \{ */

typedef struct NlaStrip {
	struct NlaStrip *prev, *next;
	struct NlaStrip *original;

	/** Child strips (used for meta strips). */
	ListBase strips;
	/** Action that is referenced by this strip (strip is 'user' of the action). */
	struct Action *action;

	ListBase fcurves;
	ListBase modifiers;

	char name[64];

	float influence;
	float strip_start;
	float strip_end;
	float action_start;
	float action_end;
	float blend_in;
	float blend_out;

	/** Runtime value, current 'time' whithin action being used (automatically evaluated, but can be overriden). */
	float time;
	/** The number of times to repeat the action range (only when no F-Curves). */
	float repeat;

	/**
	 * Slot handle to determine which animation data to look at in #action.
	 * 
	 * An NLA strip is limited to using a single slot in the Action.
	 */
	int handle;
	int flag;
	int type;
	int blend_mode;
	int extend_mode;
} NlaStrip;

/** #NlaStrip->flag */
enum {
	NLASTRIP_FLAG_ACTIVE = 1 << 0,
	NLASTRIP_FLAG_SELECT = 1 << 1,
	NLASTRIP_FLAG_MUTED = 1 << 2,

	/** NLA strip length is synced to the length of the referenced action */
	NLASTRIP_FLAG_SYNC_LENGTH = 1 << 8,
	NLASTRIP_FLAG_USR_INFLUENCE = 1 << 9,
	NLASTRIP_FLAG_USR_TIME = 1 << 10,
	NLASTRIP_FLAG_USR_TIME_CYCLIC = 1 << 11,
	NLASTRIP_FLAG_NO_TIME_MAP = 1 << 12,

	NLASTRIP_FLAG_EDIT_TOUCHED = 1 << 30,
};

/** #NlaStrip->type */
enum {
	/* References an Action. */
	NLASTRIP_TYPE_CLIP = 0,
	/* Blends between the adjacent strips. */
	NLASTRIP_TYPE_TRANSITION,
};

/** #NlaStrip->blend_mode */
enum {
	NLASTRIP_MODE_REPLACE = 0,
	NLASTRIP_MODE_ADD,
	NLASTRIP_MODE_SUBTRACT,
	NLASTRIP_MODE_MULTIPLY,
	NLASTRIP_MODE_COMBINE,
};

/** #NlaStrip->extend_mode */
enum {
	/** Extend before first frame if no previous strips in track, and always hold & extend last frame. */
	NLASTRIP_EXTEND_HOLD = 0,
	/** Only hold & extend last frame. */
	NLASTRIP_EXTEND_HOLD_FORWARD,
	/** Don't contribute at all. */
	NLASTRIP_EXTEND_NOTHING,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name NLA Tracks
 * \{ */

 /**
 * NLA Track (Non-Linear-Animation Track)
 *
 * A track groups a bunch of 'strips', which should form a continuous set of
 * motion, on top of which other such groups can be layered. This should allow
 * for animators to work in a non-destructive manner, layering tweaks, etc. over
 * 'rough' blocks of their work.
 */
typedef struct NlaTrack {
	struct NlaTrack *prev, *next;

	char name[64];

	int flag;
	/** Index of the track in the stack, not really useful but we need a pad anyways. */
	int index;

	ListBase strips;
} NlaTrack;

/** #NlaTrack->flag */
enum {
	NLATRACK_ACTIVE = 1 << 0,
	NLATRACK_SELECTED = 1 << 1,
	NLATRACK_MUTED = 1 << 2,
	NLATRACK_SOLO = 1 << 3,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name AnimData
 * \{ */

typedef struct AnimData {
	/**
	 * Active action, acts as the tweaking track for the NLA.
	 */
	struct Action *action;

	ListBase nla_tracks;

	int flag;
	int handle;

	/**
	 * The animation time is computed using the following formula,
	 * #fmod(scene->r.ctime - adt->stime, action->frame_end - action->frame_start).
	 * 
	 * This allows animations looping independently and also starting in different frames.
	 * \example Defining `stime = scene->r.ctime` at the start would start the animation from the beginning.
	 */
	float stime;

	/* Settings for active action evaluation (based on NLA strip settings) */

	/** Accumulation mode for active action. */
	int action_blend_mode;
	/** Extrapolation mode for active action. */
	int action_extend_mode;
	/** Influence for active action. */
	float action_influence;
} AnimData;

/** #AnimData->flag */
enum {
	/** Only evaluate a single track in the NLA. */
	ADT_NLA_SOLO_TRACK = 1 << 0,
	/** Don't use NLA */
	ADT_NLA_EVAL_OFF = 1 << 1,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Base Struct for Anim
 * \{ */

typedef struct IdAdtTemplate {
	ID id;
	AnimData *adt;
} IdAdtTemplate;

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// DNA_ANIM_TYPES_H
