#include "MEM_guardedalloc.h"

#include "LIB_linklist.h"
#include "LIB_memarena.h"
#include "LIB_mempool.h"
#include "LIB_utildefines.h"

#include <stdlib.h>

size_t LIB_linklist_count(const LinkNode *list) {
	size_t len;

	for (len = 0; list; list = list->next) {
		len++;
	}

	return len;
}

size_t LIB_linklist_index(const LinkNode *list, void *ptr) {
	size_t index;

	for (index = 0; list; list = list->next, index++) {
		if (list->link == ptr) {
			return index;
		}
	}

	return -1;
}

LinkNode *LIB_linklist_find(LinkNode *list, size_t index) {
	size_t i;

	for (i = 0; list; list = list->next, i++) {
		if (i == index) {
			return list;
		}
	}

	return NULL;
}

LinkNode *LIB_linklist_find_last(LinkNode *list) {
	if (list) {
		while (list->next) {
			list = list->next;
		}
	}
	return list;
}

void LIB_linklist_reverse(LinkNode **listp) {
	LinkNode *rhead = NULL, *cur = *listp;

	while (cur) {
		LinkNode *next = cur->next;

		cur->next = rhead;
		rhead = cur;

		cur = next;
	}

	*listp = rhead;
}

void LIB_linklist_move_item(LinkNode **listp, size_t curr_index, size_t new_index) {
	LinkNode *lnk, *lnk_psrc = NULL, *lnk_pdst = NULL;
	size_t i;

	if (new_index == curr_index) {
		return;
	}

	if (new_index < curr_index) {
		for (lnk = *listp, i = 0; lnk; lnk = lnk->next, i++) {
			if (i == new_index - 1) {
				lnk_pdst = lnk;
			}
			else if (i == curr_index - 1) {
				lnk_psrc = lnk;
				break;
			}
		}

		if (!(lnk_psrc && lnk_psrc->next && (!lnk_pdst || lnk_pdst->next))) {
			/* Invalid indices, abort. */
			return;
		}

		lnk = lnk_psrc->next;
		lnk_psrc->next = lnk->next;
		if (lnk_pdst) {
			lnk->next = lnk_pdst->next;
			lnk_pdst->next = lnk;
		}
		else {
			/* destination is first element of the list... */
			lnk->next = *listp;
			*listp = lnk;
		}
	}
	else {
		for (lnk = *listp, i = 0; lnk; lnk = lnk->next, i++) {
			if (i == new_index) {
				lnk_pdst = lnk;
				break;
			}
			if (i == curr_index - 1) {
				lnk_psrc = lnk;
			}
		}

		if (!(lnk_pdst && (!lnk_psrc || lnk_psrc->next))) {
			/* Invalid indices, abort. */
			return;
		}

		if (lnk_psrc) {
			lnk = lnk_psrc->next;
			lnk_psrc->next = lnk->next;
		}
		else {
			/* source is first element of the list... */
			lnk = *listp;
			*listp = lnk->next;
		}
		lnk->next = lnk_pdst->next;
		lnk_pdst->next = lnk;
	}
}

void LIB_linklist_prepend_nlink(LinkNode **listp, void *ptr, LinkNode *nlink) {
	nlink->link = ptr;
	nlink->next = *listp;
	*listp = nlink;
}

void LIB_linklist_prepend(LinkNode **listp, void *ptr) {
	LinkNode *nlink = MEM_mallocN(sizeof(*nlink), __func__);
	LIB_linklist_prepend_nlink(listp, ptr, nlink);
}

void LIB_linklist_prepend_arena(LinkNode **listp, void *ptr, MemArena *ma) {
	LinkNode *nlink = LIB_memory_arena_malloc(ma, sizeof(*nlink));
	LIB_linklist_prepend_nlink(listp, ptr, nlink);
}

void LIB_linklist_prepend_pool(LinkNode **listp, void *ptr, MemPool *mempool) {
	LinkNode *nlink = LIB_memory_pool_malloc(mempool);
	LIB_linklist_prepend_nlink(listp, ptr, nlink);
}

void LIB_linklist_append_nlink(LinkNodePair *list_pair, void *ptr, LinkNode *nlink) {
	nlink->link = ptr;
	nlink->next = NULL;

	if (list_pair->list) {
		ROSE_assert((list_pair->last_node != NULL) && (list_pair->last_node->next == NULL));
		list_pair->last_node->next = nlink;
	}
	else {
		ROSE_assert(list_pair->last_node == NULL);
		list_pair->list = nlink;
	}

	list_pair->last_node = nlink;
}

void LIB_linklist_append(LinkNodePair *list_pair, void *ptr) {
	LinkNode *nlink = MEM_mallocN(sizeof(*nlink), __func__);
	LIB_linklist_append_nlink(list_pair, ptr, nlink);
}

void LIB_linklist_append_arena(LinkNodePair *list_pair, void *ptr, MemArena *ma) {
	LinkNode *nlink = LIB_memory_arena_malloc(ma, sizeof(*nlink));
	LIB_linklist_append_nlink(list_pair, ptr, nlink);
}

void LIB_linklist_append_pool(LinkNodePair *list_pair, void *ptr, MemPool *mempool) {
	LinkNode *nlink = LIB_memory_pool_malloc(mempool);
	LIB_linklist_append_nlink(list_pair, ptr, nlink);
}

void *LIB_linklist_pop(struct LinkNode **listp) {
	/* intentionally no NULL check */
	void *link = (*listp)->link;
	void *next = (*listp)->next;

	MEM_freeN(*listp);

	*listp = next;
	return link;
}

void *LIB_linklist_pop_pool(struct LinkNode **listp, struct MemPool *mempool) {
	/* intentionally no NULL check */
	void *link = (*listp)->link;
	void *next = (*listp)->next;

	LIB_memory_pool_free(mempool, (*listp));

	*listp = next;
	return link;
}

void LIB_linklist_insert_after(LinkNode **listp, void *ptr) {
	LinkNode *nlink = MEM_mallocN(sizeof(*nlink), __func__);
	LinkNode *node = *listp;

	nlink->link = ptr;

	if (node) {
		nlink->next = node->next;
		node->next = nlink;
	}
	else {
		nlink->next = NULL;
		*listp = nlink;
	}
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

void LIB_linklist_free_pool(LinkNode *list, LinkNodeFreeFP freefunc, struct MemPool *mempool) {
	while (list) {
		LinkNode *next = list->next;

		if (freefunc) {
			freefunc(list->link);
		}
		LIB_memory_pool_free(mempool, list);

		list = next;
	}
}

void LIB_linklist_freeN(LinkNode *list) {
	while (list) {
		LinkNode *next = list->next;

		MEM_freeN(list->link);
		MEM_freeN(list);

		list = next;
	}
}

void LIB_linklist_apply(LinkNode *list, LinkNodeApplyFP applyfunc, void *userdata) {
	for (; list; list = list->next) {
		applyfunc(list->link, userdata);
	}
}
