#include "MEM_alloc.h"

#include "LIB_linklist.h"
#include "LIB_string.h"

size_t LIB_linklist_count(const LinkNode *list) {
	size_t cnt = 0;

	for (; list; list = list->next) {
		cnt++;
	}

	return cnt;
}

size_t LIB_linklist_index(const LinkNode *list, void *ptr) {
	size_t index = 0;

	for (; list; index++, list = list->next) {
		if (list->link == ptr) {
			return index;
		}
	}

	return LIB_NPOS;
}

LinkNode *LIB_linklist_find(LinkNode *list, size_t index) {
	while (list && index--) {
		list = list->next;
	}

	return list;
}

static void lib_linklist_prepend_nlink(LinkNode **listp, void *ptr, LinkNode *nlink) {
	nlink->link = ptr;
	nlink->next = *listp;
	*listp = nlink;
}

void LIB_linklist_prepend(LinkNode **listp, void *ptr) {
	LinkNode *nlink = MEM_mallocN(sizeof(LinkNode), "rose::LinkNode");

	lib_linklist_prepend_nlink(listp, ptr, nlink);
}

void *LIB_linklist_pop(LinkNode **listp) {
	void *link = (*listp)->link;
	void *next = (*listp)->next;

	MEM_freeN(*listp);

	*listp = next;
	return link;
}

void LIB_linklist_free(LinkNode *list, LinkNodeFreeFP freefunc) {
	while (list) {
		LinkNode *next = list->next;

		if (freefunc) {
			freefunc(list->link);
		}
		MEM_freeN(list);

		list = next;
	}
}

void LIB_linklist_apply(LinkNode *list, LinkNodeApplyFP applyfunc, void *userdata) {
	for (; list; list = list->next) {
		applyfunc(list->link, userdata);
	}
}
