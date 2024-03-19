#pragma once

#include "LIB_sys_types.h"

#if __cplusplus
extern "C" {
#endif

typedef void (*LinkNodeFreeFP)(void *link);
typedef void (*LinkNodeApplyFP)(void *link, void *userdata);

typedef struct LinkNode {
	struct LinkNode *next;
	
	void *link;
} LinkNode;

size_t LIB_linklist_count(const LinkNode *list);
size_t LIB_linklist_index(const LinkNode *list, void *ptr);

/** Find the LinkNode at the specified index inside the linklist. */
LinkNode *LIB_linklist_find(LinkNode *list, size_t index);

/** 
 * Prepend a link inside the linklist, and since linklist consinsts solely of LinkNode(s), 
 * the first LinkNode will change.
 *
 * \note This will replace the LinkNode pointing to the start of the list.
 */
void LIB_linklist_prepend(LinkNode **list, void *ptr);

/** Remove the first link from the linklist. */
void *LIB_linklist_pop(LinkNode **list);

void LIB_linklist_free(LinkNode *list, LinkNodeFreeFP freefunc);
void LIB_linklist_apply(LinkNode *list, LinkNodeApplyFP applyfunc, void *userdata);

#if __cplusplus
}
#endif
