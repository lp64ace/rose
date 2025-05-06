#ifndef RM_STRUCTURE_H
#define RM_STRUCTURE_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

ROSE_INLINE struct RMDiskLink *rmesh_disk_edge_link_from_vert(const struct RMEdge *e, const struct RMVert *v);

void rmesh_disk_edge_append(struct RMEdge *e, struct RMVert *v);
void rmesh_disk_edge_remove(struct RMEdge *e, struct RMVert *v);

ROSE_INLINE struct RMEdge *rmesh_disk_edge_next(const struct RMEdge *e, const struct RMVert *v);
ROSE_INLINE struct RMEdge *rmesh_disk_edge_prev(const struct RMEdge *e, const struct RMVert *v);

struct RMEdge *rmesh_disk_faceedge_find_next(const struct RMEdge *e, const struct RMVert *v);

void rmesh_radial_loop_append(struct RMEdge *e, struct RMLoop *l);
void rmesh_radial_loop_remove(struct RMEdge *e, struct RMLoop *l);

/**
 * Counts the number of loop users for this vertex. Note that this is equivalent to counting the
 * number of faces incident upon this vertex.
 */
int rmesh_disk_facevert_count(const struct RMVert *v);
int rmesh_disk_facevert_count_at_most(const struct RMVert *v, int count_max);

/**
 * \brief FIND FIRST FACE EDGE
 *
 * Finds the first edge in a vertices disk cycle that has one of this vert's loops attached to it.
 */
struct RMEdge *rmesh_disk_faceedge_find_first(const struct RMEdge *e, const struct RMVert *v);

/**
 * Special case for RM_LOOPS_OF_VERT & RM_FACES_OF_VERT, avoids 2x calls.
 *
 * The returned #RMLoop.e matches the result of #rmesh_disk_faceedge_find_first
 */
struct RMLoop *rmesh_disk_faceloop_find_first(const struct RMEdge *e, const struct RMVert *v);

/**
 * \brief RADIAL COUNT FACE VERT
 *
 * Returns the number of times a vertex appears in a radial cycle.
 */
int rmesh_radial_facevert_count(const struct RMLoop *l, const struct RMVert *v);
int rmesh_radial_facevert_count_at_most(const struct RMLoop *l, const struct RMVert *v, int count_max);

bool rmesh_radial_facevert_check(const RMLoop *l, const RMVert *v);

struct RMLoop *rmesh_radial_faceloop_find_first(const struct RMLoop *l, const struct RMVert *v);
struct RMLoop *rmesh_radial_faceloop_find_next(const struct RMLoop *l, const struct RMVert *v);

#ifdef __cplusplus
}
#endif

#include "intern/rm_structure_inline.h"

#endif // RM_STRUCTURE_H
