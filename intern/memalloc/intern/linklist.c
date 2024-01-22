#include "linklist.h"

#include <stdint.h>

/* -------------------------------------------------------------------- */
/** \name Functions
 * \{ */

void addlink(volatile struct ListBase *lb, struct Link *vlink) {
	vlink->prev = lb->last;
	vlink->next = NULL;

	if (lb->last) {
		lb->last->next = vlink;
	}
	if (!lb->first) {
		lb->first = vlink;
	}
	lb->last = vlink;
}

void remlink(volatile struct ListBase *lb, struct Link *vlink) {
	if (vlink->prev) {
		vlink->prev->next = vlink->next;
	}
	if (vlink->next) {
		vlink->next->prev = vlink->prev;
	}

	if (lb->first == vlink) {
		lb->first = vlink->next;
	}
	if (lb->last == vlink) {
		lb->last = vlink->prev;
	}
}

/* \} */
