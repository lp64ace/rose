#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "MEM_alloc.h"

void LIB_addhead(struct ListBase *listbase, void *vlink) {
	Link *link = (Link *)vlink;

	link->prev = listbase->last;
	link->next = NULL;

	if (listbase->last) {
		listbase->last->next = link;
	}
	if (listbase->first == NULL) {
		listbase->first = link;
	}
	listbase->last = link;
}

void LIB_addtail(struct ListBase *listbase, void *vlink) {
	Link *link = (Link *)vlink;

	link->next = listbase->first;
	link->prev = NULL;

	if (listbase->first) {
		listbase->first->prev = link;
	}
	if (listbase->last == NULL) {
		listbase->last = link;
	}
	listbase->first = link;
}

void *LIB_pophead(struct ListBase *listbase) {
	Link *link = (Link *)listbase->first;
	if (link) {
		LIB_remlink(listbase, link);
	}
	return link;
}

void *LIB_poptail(struct ListBase *listbase) {
	Link *link = (Link *)listbase->last;
	if (link) {
		LIB_remlink(listbase, link);
	}
	return link;
}

void LIB_remlink(struct ListBase *listbase, void *vlink) {
	Link *link = (Link *)vlink;

	if (link->next) {
		link->next->prev = link->prev;
	}
	if (link->prev) {
		link->prev->next = link->next;
	}

	if (listbase->last == link) {
		listbase->last = link->prev;
	}
	if (listbase->first == link) {
		listbase->first = link->next;
	}
}

void *LIB_findstring(const struct ListBase *listbase, const char *str, const size_t offset) {
	Link *iter;

	for (iter = listbase->first; iter; iter = iter->next) {
		if (STREQ(POINTER_OFFSET(iter, offset), str)) {
			return iter;
		}
	}

	return NULL;
}

void *LIB_rfindstring(const struct ListBase *listbase, const char *str, const size_t offset) {
	Link *iter;

	for (iter = listbase->last; iter; iter = iter->prev) {
		if (STREQ(POINTER_OFFSET(iter, offset), str)) {
			return iter;
		}
	}

	return NULL;
}

void *LIB_findbytes(const struct ListBase *listbase, const void *bytes, const size_t length, const size_t offset) {
	Link *iter;

	for (iter = listbase->first; iter; iter = iter->next) {
		if (memcmp(POINTER_OFFSET(iter, offset), bytes, length) == 0) {
			return iter;
		}
	}

	return NULL;
}

void *LIB_rfindbytes(const struct ListBase *listbase, const void *bytes, const size_t length, const size_t offset) {
	Link *iter;

	for (iter = listbase->last; iter; iter = iter->prev) {
		if (memcmp(POINTER_OFFSET(iter, offset), bytes, length) == 0) {
			return iter;
		}
	}

	return NULL;
}

void *LIB_findptr(const struct ListBase *listbase, void *ptr, const size_t offset) {
	Link *iter;

	for (iter = listbase->first; iter; iter = iter->next) {
		const void *ptr_iter = *(const void **)POINTER_OFFSET(iter, offset);
		if (ptr_iter == ptr) {
			return iter;
		}
	}

	return NULL;
}

void *LIB_rfindptr(const struct ListBase *listbase, void *ptr, const size_t offset) {
	Link *iter;

	for (iter = listbase->last; iter; iter = iter->prev) {
		const void *ptr_iter = *(const void **)POINTER_OFFSET(iter, offset);
		if (ptr_iter == ptr) {
			return iter;
		}
	}

	return NULL;
}

void LIB_freelistN(struct ListBase *listbase) {
	Link *link, *next;
	
	for(link = listbase->first; link; link = next) {
		next = link->next;
		
		MEM_freeN(link);
	}
}

struct LinkData *LIB_genericNodeN(void *data) {
	LinkData *link;
	
	if(data == NULL) {
		return NULL;
	}
	
	link = MEM_callocN(sizeof(LinkData), "LinkData");
	link->data = data;
	
	return link;
}
