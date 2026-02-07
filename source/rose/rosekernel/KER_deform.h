#ifndef KER_DEFORM_H
#define KER_DEFORM_H

#include "DNA_meshdata_types.h"

struct Object;
struct MDeformVert;
struct MDeformWeight;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Deform Vertex
 * \{ */

struct MDeformWeight *KER_defvert_find_index(const struct MDeformVert *dv, const int defgroup);

/** Ensures that `dv` has a deform weight entry for the specified defweight group. */
struct MDeformWeight *KER_defvert_ensure_index(struct MDeformVert *dv, int defgroup);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Deform Group List
 * \{ */

void KER_defgroup_copy_list(struct ListBase *outbase, const struct ListBase *inbase);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Deform Group
 * \{ */

struct DeformGroup *KER_object_defgroup_new(struct Object *object, const char *name);
struct DeformGroup *KER_defgroup_duplicate(const struct DeformGroup *defgroup);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_DEFORM_H
