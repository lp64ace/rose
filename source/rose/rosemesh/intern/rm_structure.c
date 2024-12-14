#include "RM_type.h"
#include "intern/rm_private.h"

void rmesh_disk_edge_append(RMEdge *e, RMVert *v) {
	if (!v->e) {
		RMDiskLink *dl1 = rmesh_disk_edge_link_from_vert(e, v);

		v->e = e;
		dl1->next = dl1->prev = e;
	}
	else {
		RMDiskLink *dl1, *dl2, *dl3;

		dl1 = rmesh_disk_edge_link_from_vert(e, v);
		dl2 = rmesh_disk_edge_link_from_vert(v->e, v);
		dl3 = dl2->prev ? rmesh_disk_edge_link_from_vert(dl2->prev, v) : NULL;

		dl1->next = v->e;
		dl1->prev = dl2->prev;

		dl2->prev = e;
		if (dl3) {
			dl3->next = e;
		}
	}
}

void rmesh_disk_edge_remove(RMEdge *e, RMVert *v) {
	RMDiskLink *dl1, *dl2;

	dl1 = rmesh_disk_edge_link_from_vert(e, v);
	if (dl1->prev) {
		dl2 = rmesh_disk_edge_link_from_vert(dl1->prev, v);
		dl2->next = dl1->next;
	}

	if (dl1->next) {
		dl2 = rmesh_disk_edge_link_from_vert(dl1->next, v);
		dl2->prev = dl1->prev;
	}

	if (v->e == e) {
		v->e = (e != dl1->next) ? dl1->next : NULL;
	}

	dl1->next = dl1->prev = NULL;
}

RMEdge *rmesh_disk_faceedge_find_next(const RMEdge *e, const RMVert *v) {
	RMEdge *e_find;
	e_find = rmesh_disk_edge_next(e, v);
	do {
		if (e_find->l && rmesh_radial_facevert_check(e_find->l, v)) {
			return e_find;
		}
	} while ((e_find = rmesh_disk_edge_next(e_find, v)) != e);
	return (RMEdge *)e;
}

int rmesh_disk_facevert_count(const RMVert *v) {
	/* is there an edge on this vert at all */
	int count = 0;
	if (v->e) {
		RMEdge *e_first, *e_iter;

		/* first, loop around edge */
		e_first = e_iter = v->e;
		do {
			if (e_iter->l) {
				count += rmesh_radial_facevert_count(e_iter->l, v);
			}
		} while ((e_iter = rmesh_disk_edge_next(e_iter, v)) != e_first);
	}

	return count;
}

int rmesh_disk_facevert_count_at_most(const RMVert *v, const int count_max) {
	/* is there an edge on this vert at all */
	int count = 0;
	if (v->e) {
		RMEdge *e_first, *e_iter;

		/* first, loop around edge */
		e_first = e_iter = v->e;
		do {
			if (e_iter->l) {
				count += rmesh_radial_facevert_count_at_most(e_iter->l, v, count_max - count);
				if (count == count_max) {
					break;
				}
			}
		} while ((e_iter = rmesh_disk_edge_next(e_iter, v)) != e_first);
	}

	return count;
}

RMEdge *rmesh_disk_faceedge_find_first(const RMEdge *e, const RMVert *v) {
	const RMEdge *e_iter = e;
	do {
		if (e_iter->l != NULL) {
			return (RMEdge *)((e_iter->l->v == v) ? e_iter : e_iter->l->next->e);
		}
	} while ((e_iter = rmesh_disk_edge_next(e_iter, v)) != e);

	return NULL;
}

RMLoop *rmesh_disk_faceloop_find_first(const RMEdge *e, const RMVert *v) {
	const RMEdge *e_iter = e;
	do {
		if (e_iter->l != NULL) {
			return (e_iter->l->v == v) ? e_iter->l : e_iter->l->next;
		}
	} while ((e_iter = rmesh_disk_edge_next(e_iter, v)) != e);

	return NULL;
}

void rmesh_radial_loop_append(RMEdge *e, RMLoop *l) {
	if (e->l == NULL) {
		e->l = l;
		l->radial_next = l->radial_prev = l;
	}
	else {
		l->radial_prev = e->l;
		l->radial_next = e->l->radial_next;

		e->l->radial_next->radial_prev = l;
		e->l->radial_next = l;

		e->l = l;
	}

	if (l->e && l->e != e) {
		/* l is already in a radial cycle for a different edge */
		ROSE_assert_unreachable();
	}

	l->e = e;
}

void rmesh_radial_loop_remove(RMEdge *e, RMLoop *l) {
	/* if e is non-NULL, l must be in the radial cycle of e */
	if (e != l->e) {
		ROSE_assert_unreachable();
	}

	if (l->radial_next != l) {
		if (l == e->l) {
			e->l = l->radial_next;
		}

		l->radial_next->radial_prev = l->radial_prev;
		l->radial_prev->radial_next = l->radial_next;
	}
	else {
		if (l == e->l) {
			e->l = NULL;
		}
		else {
			ROSE_assert_unreachable();
		}
	}

	/* l is no longer in a radial cycle; empty the links
	 * to the cycle and the link back to an edge */
	l->radial_next = l->radial_prev = NULL;
	l->e = NULL;
}

int rmesh_radial_facevert_count(const RMLoop *l, const RMVert *v) {
	const RMLoop *l_iter;
	int count = 0;
	l_iter = l;
	do {
		if (l_iter->v == v) {
			count++;
		}
	} while ((l_iter = l_iter->radial_next) != l);

	return count;
}

int rmesh_radial_facevert_count_at_most(const RMLoop *l, const RMVert *v, const int count_max) {
	const RMLoop *l_iter;
	int count = 0;
	l_iter = l;
	do {
		if (l_iter->v == v) {
			count++;
			if (count == count_max) {
				break;
			}
		}
	} while ((l_iter = l_iter->radial_next) != l);

	return count;
}

RMLoop *rmesh_radial_faceloop_find_first(const RMLoop *l, const RMVert *v) {
	const RMLoop *l_iter;
	l_iter = l;
	do {
		if (l_iter->v == v) {
			return (RMLoop *)l_iter;
		}
	} while ((l_iter = l_iter->radial_next) != l);

	return NULL;
}

RMLoop *rmesh_radial_faceloop_find_next(const RMLoop *l, const RMVert *v) {
	RMLoop *l_iter;
	l_iter = l->radial_next;
	do {
		if (l_iter->v == v) {
			return l_iter;
		}
	} while ((l_iter = l_iter->radial_next) != l);

	return (RMLoop *)l;
}

bool rmesh_radial_facevert_check(const RMLoop *l, const RMVert *v) {
	const RMLoop *l_iter;
	l_iter = l;
	do {
		if (l_iter->v == v) {
			return true;
		}
	} while ((l_iter = l_iter->radial_next) != l);

	return false;
}
