#ifndef RM_QUERY_INLINE_H
#define RM_QUERY_INLINE_H

#include "RM_type.h"

ROSE_INLINE bool RM_vert_in_edge(const RMEdge *e, const RMVert *v) {
	return (ELEM(v, e->v1, e->v2));
}

ROSE_INLINE bool RM_edge_in_loop(const RMEdge *e, const RMLoop *l) {
	return (l->e == e || l->prev->e == e);
}

ROSE_INLINE bool RM_verts_in_edge(const RMVert *v1, const RMVert *v2, const RMEdge *e) {
	return ((e->v1 == v1 && e->v2 == v2) || (e->v1 == v2 && e->v2 == v1));
}

ROSE_INLINE RMVert *RM_edge_other_vert(RMEdge *e, const RMVert *v) {
	if (e->v1 == v) {
		return e->v2;
	}
	else if (e->v2 == v) {
		return e->v1;
	}
	return NULL;
}

ROSE_INLINE bool RM_loop_is_adjacent(const RMLoop *l_a, const RMLoop *l_b) {
	ROSE_assert(l_a->f == l_b->f);
	ROSE_assert(l_a != l_b);
	return (ELEM(l_b, l_a->next, l_a->prev));
}

#endif // RM_QUERY_INLINE_H
