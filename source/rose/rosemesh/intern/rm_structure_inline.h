#ifndef RM_STRUCTURE_INLINE_H
#define RM_STRUCTURE_INLINE_H

#include "LIB_utildefines.h"

#include "rm_query.h"

ROSE_INLINE RMDiskLink *rmesh_disk_edge_link_from_vert(const RMEdge *e, const RMVert *v) {
	ROSE_assert(RM_vert_in_edge(e, v));
	return (RMDiskLink *)&(&e->v1_disk_link)[v == e->v2];
}

ROSE_INLINE struct RMEdge *rmesh_disk_edge_next(const struct RMEdge *e, const struct RMVert *v) {
	return RM_DISK_EDGE_NEXT(e, v);
}

ROSE_INLINE struct RMEdge *rmesh_disk_edge_prev(const struct RMEdge *e, const struct RMVert *v) {
	return RM_DISK_EDGE_PREV(e, v);
}

#endif // RM_STRUCTURE_INLINE_H
