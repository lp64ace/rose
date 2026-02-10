#ifndef DNA_ANIM_TYPES_H
#define DNA_ANIM_TYPES_H

#include "DNA_ID.h"

#include "DNA_curve_types.h"

struct ActionGroup;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name FCurves
 * \{ */

typedef struct FPoint {
	/** Time + Value */
	float vec[2];
} FPoint;

typedef struct FCurve {
	struct FCurve *prev, *next;

	struct ActionGroup *group;

	BezTriple *bezt;
	/** Baked or imported motion samples (array). */
	FPoint *fpt;
	/** Total number of points which define the curve (i.e. size of arrays in FPoints). */
	int totvert;

	/**
	 * RNA-path to resolve data-access, see: #RNA_path_resolve_property.
	 *
	 * \note String look-ups for collection and custom-properties are escaped using #LIB_str_escape.
	 */
	char *path;
	int index;
} FCurve;

/** \} */

/* -------------------------------------------------------------------- */
/** \name AnimData
 * \{ */

typedef struct AnimData {
	/**
	 * Active action, acts as the tweaking track for the NLA.
	 */
	struct Action *action;

	int handle;

	/**
	 * The animation time is computed using the following formula,
	 * #fmod(scene->r.ctime - adt->stime, action->frame_end - action->frame_start).
	 * 
	 * This allows animations looping independently and also starting in different frames.
	 * \example Defining `stime = scene->r.ctime` at the start would start the animation from the beginning.
	 */
	float stime;
} AnimData;

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
