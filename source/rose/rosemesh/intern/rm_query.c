#include "MEM_guardedalloc.h"

#include "RM_type.h"
#include "intern/rm_private.h"

RMEdge *RM_edge_exists(RMVert *v_a, RMVert *v_b) {
	/* speedup by looping over both edges verts
	 * where one vert may connect to many edges but not the other. */

	RMEdge *e_a, *e_b;

	ROSE_assert(v_a != v_b);
	ROSE_assert(v_a->head.htype == RM_VERT && v_b->head.htype == RM_VERT);

	if ((e_a = v_a->e) && (e_b = v_b->e)) {
		RMEdge *e_a_iter = e_a, *e_b_iter = e_b;

		do {
			if (RM_vert_in_edge(e_a_iter, v_b)) {
				return e_a_iter;
			}
			if (RM_vert_in_edge(e_b_iter, v_a)) {
				return e_b_iter;
			}
		} while (((e_a_iter = rmesh_disk_edge_next(e_a_iter, v_a)) != e_a) && ((e_b_iter = rmesh_disk_edge_next(e_b_iter, v_b)) != e_b));
	}

	return NULL;
}

RMEdge *RM_edge_find_double(RMEdge *e) {
	RMVert *v = e->v1;
	RMVert *v_other = e->v2;

	RMEdge *e_iter;

	e_iter = e;
	while ((e_iter = rmesh_disk_edge_next(e_iter, v)) != e) {
		if (RM_vert_in_edge(e_iter, v_other)) {
			return e_iter;
		}
	}

	return NULL;
}

RMFace *RM_face_exists(RMVert **varr, int len) {
	if (varr[0]->e) {
		RMEdge *e_iter, *e_first;
		e_iter = e_first = varr[0]->e;

		/* would normally use RM_LOOPS_OF_VERT, but this runs so often,
		 * its faster to iterate on the data directly */
		do {
			if (e_iter->l) {
				RMLoop *l_iter_radial, *l_first_radial;
				l_iter_radial = l_first_radial = e_iter->l;

				do {
					if ((l_iter_radial->v == varr[0]) && (l_iter_radial->f->len == len)) {
						/* the fist 2 verts match, now check the remaining (len - 2) faces do too
						 * winding isn't known, so check in both directions */
						int i_walk = 2;

						if (l_iter_radial->next->v == varr[1]) {
							RMLoop *l_walk = l_iter_radial->next->next;
							do {
								if (l_walk->v != varr[i_walk]) {
									break;
								}
							} while ((void)(l_walk = l_walk->next), ++i_walk != len);
						}
						else if (l_iter_radial->prev->v == varr[1]) {
							RMLoop *l_walk = l_iter_radial->prev->prev;
							do {
								if (l_walk->v != varr[i_walk]) {
									break;
								}
							} while ((void)(l_walk = l_walk->prev), ++i_walk != len);
						}

						if (i_walk == len) {
							return l_iter_radial->f;
						}
					}
				} while ((l_iter_radial = l_iter_radial->radial_next) != l_first_radial);
			}
		} while ((e_iter = RM_DISK_EDGE_NEXT(e_iter, varr[0])) != e_first);
	}

	return NULL;
}

RMFace *RM_face_find_double(RMFace *f) {
	RMLoop *l_first = RM_FACE_FIRST_LOOP(f);
	for (RMLoop *l_iter = l_first->radial_next; l_first != l_iter; l_iter = l_iter->radial_next) {
		if (l_iter->f->len == l_first->f->len) {
			if (l_iter->v == l_first->v) {
				RMLoop *l_a = l_first, *l_b = l_iter, *l_b_init = l_iter;
				do {
					if (l_a->e != l_b->e) {
						break;
					}
				} while (((void)(l_a = l_a->next), (l_b = l_b->next)) != l_b_init);
				if (l_b == l_b_init) {
					return l_iter->f;
				}
			}
			else {
				RMLoop *l_a = l_first, *l_b = l_iter, *l_b_init = l_iter;
				do {
					if (l_a->e != l_b->e) {
						break;
					}
				} while (((void)(l_a = l_a->prev), (l_b = l_b->next)) != l_b_init);
				if (l_b == l_b_init) {
					return l_iter->f;
				}
			}
		}
	}
	return NULL;
}

bool RM_edge_share_vert_check(RMEdge *e1, RMEdge *e2) {
	return (e1->v1 == e2->v1 || e1->v1 == e2->v2 || e1->v2 == e2->v1 || e1->v2 == e2->v2);
}

RMVert *RM_edge_share_vert(RMEdge *e1, RMEdge *e2) {
	ROSE_assert(e1 != e2);
	if (RM_vert_in_edge(e2, e1->v1)) {
		return e1->v1;
	}
	if (RM_vert_in_edge(e2, e1->v2)) {
		return e1->v2;
	}
	return NULL;
}
