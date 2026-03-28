#ifndef KER_DEFORM_H
#define KER_DEFORM_H

#include "DNA_meshdata_types.h"

struct Object;
struct MDeformVert;
struct MDeformWeight;
struct RoseDataReader;
struct RoseWriter;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Deform Vertex
 * \{ */

struct MDeformWeight *KER_defvert_find_index(const struct MDeformVert *dv, const int defgroup);

/** Ensures that `dv` has a deform weight entry for the specified defweight group. */
struct MDeformWeight *KER_defvert_ensure_index(struct MDeformVert *dv, int defgroup);

void KER_defvert_rose_write(struct RoseWriter *writer, int count, struct MDeformVert *dvlist);
void KER_defvert_rose_read(struct RoseDataReader *reader, int count, struct MDeformVert *dvlist);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Deform Group List
 * \{ */

void KER_defgroup_copy_list(struct ListBase *outbase, const struct ListBase *inbase);
void KER_defgroup_rose_write(struct RoseWriter *writer, const struct ListBase *defgroup);

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
