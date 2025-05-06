#ifndef LIB_LINKLIST_H
#define LIB_LINKLIST_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct MemPool;
struct MemArena;

typedef void (*LinkNodeFreeFP)(void *link);
typedef void (*LinkNodeApplyFP)(void *link, void *userdata);

typedef struct LinkNode {
	struct LinkNode *next;
	void *link;
} LinkNode;

/**
 * Use for append (single linked list, storing the last element).
 *
 * \note list manipulation functions don't operate on this struct.
 * This is only to be used while appending.
 */
typedef struct LinkNodePair {
	LinkNode *list, *last_node;
} LinkNodePair;

size_t LIB_linklist_count(const LinkNode *list);
size_t LIB_linklist_index(const LinkNode *list, void *ptr);

LinkNode *LIB_linklist_find(LinkNode *list, size_t index);
LinkNode *LIB_linklist_find_last(LinkNode *list);

void LIB_linklist_reverse(LinkNode **listp);

/**
 * Move an item from its current position to a new one inside a single-linked list.
 * \note `*listp` may be modified.
 */
void LIB_linklist_move_item(LinkNode **listp, size_t curr_index, size_t new_index);

/**
 * A version of #LIB_linklist_prepend that takes the allocated link.
 */
void LIB_linklist_prepend_nlink(LinkNode **listp, void *ptr, LinkNode *nlink);
void LIB_linklist_prepend(LinkNode **listp, void *ptr);
void LIB_linklist_prepend_arena(LinkNode **listp, void *ptr, struct MemArena *ma);
void LIB_linklist_prepend_pool(LinkNode **listp, void *ptr, struct MemPool *mempool);

/* Use #LinkNodePair to avoid full search. */

/**
 * A version of append that takes the allocated link.
 */
void LIB_linklist_append_nlink(LinkNodePair *list_pair, void *ptr, LinkNode *nlink);
void LIB_linklist_append(LinkNodePair *list_pair, void *ptr);
void LIB_linklist_append_arena(LinkNodePair *list_pair, void *ptr, struct MemArena *ma);
void LIB_linklist_append_pool(LinkNodePair *list_pair, void *ptr, struct MemPool *mempool);

void *LIB_linklist_pop(LinkNode **listp);
void *LIB_linklist_pop_pool(LinkNode **listp, struct MemPool *mempool);
void LIB_linklist_insert_after(LinkNode **listp, void *ptr);

void LIB_linklist_free(LinkNode *list, LinkNodeFreeFP freefunc);
void LIB_linklist_freeN(LinkNode *list);
void LIB_linklist_free_pool(LinkNode *list, LinkNodeFreeFP freefunc, struct MemPool *mempool);
void LIB_linklist_apply(LinkNode *list, LinkNodeApplyFP applyfunc, void *userdata);

#define LIB_linklist_prepend_alloca(listp, ptr) LIB_linklist_prepend_nlink(listp, ptr, (LinkNode *)alloca(sizeof(LinkNode)))
#define LIB_linklist_append_alloca(list_pair, ptr) LIB_linklist_append_nlink(list_pair, ptr, (LinkNode *)alloca(sizeof(LinkNode)))

#ifdef __cplusplus
}
#endif

#endif	// LIB_LINKLIST_H
