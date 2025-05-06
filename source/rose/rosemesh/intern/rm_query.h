#ifndef RM_QUERY_H
#define RM_QUERY_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

ROSE_INLINE bool RM_edge_in_loop(const struct RMEdge *e, const struct RMLoop *l);
ROSE_INLINE bool RM_vert_in_edge(const struct RMEdge *e, const struct RMVert *v);
ROSE_INLINE bool RM_verts_in_edge(const struct RMVert *v1, const struct RMVert *v2, const struct RMEdge *e);

ROSE_INLINE struct RMVert *RM_edge_other_vert(struct RMEdge *e, const struct RMVert *v);

ROSE_INLINE bool RM_loop_is_adjacent(const struct RMLoop *l_a, const struct RMLoop *l_b);

/**
 * Returns the edge existing between \a v_a and \a v_b, or NULL if there isn't one.
 *
 * \note multiple edges may exist between any two vertices, and therefore
 * this function only returns the first one found.
 */
struct RMEdge *RM_edge_exists(struct RMVert *v_a, struct RMVert *v_b);
/**
 * Returns an edge sharing the same vertices as this one.
 * This isn't an invalid state but tools should clean up these cases before
 * returning the mesh to the user.
 */
struct RMEdge *RM_edge_find_double(struct RMEdge *e);

/**
 * Given a set of vertices (varr), find out if
 * there is a face with exactly those vertices
 * (and only those vertices).
 *
 * \note there used to be a RM_face_exists_overlap function that checks for partial overlap.
 */
struct RMFace *RM_face_exists(struct RMVert **varr, int len);
/**
 * Check if the face has an exact duplicate (both winding directions).
 * This isn't an invalid state but tools should clean up these cases before
 * returning the mesh to the user.
 */
struct RMFace *RM_face_find_double(struct RMFace *f);

/**
 * Tests to see if e1 shares a vertex with e2.
 */
bool RM_edge_share_vert_check(struct RMEdge *e1, struct RMEdge *e2);

/**
 * Return the shared vertex between the two edges or NULL.
 */
struct RMVert *RM_edge_share_vert(struct RMEdge *e1, struct RMEdge *e2);

#ifdef __cplusplus
}
#endif

#include "intern/rm_query_inline.h"

#endif // RM_QUERY_H
