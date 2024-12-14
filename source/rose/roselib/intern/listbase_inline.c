#ifndef LIB_LISTBASE_INLINE_C
#define LIB_LISTBASE_INLINE_C

#include "LIB_listbase.h"

#ifdef __cplusplus
extern "C" {
#endif

ROSE_INLINE bool LIB_listbase_is_empty(const ListBase *listbase) {
	return (listbase->first == NULL && listbase->first == listbase->last);
}

ROSE_INLINE bool LIB_listbase_is_single(const ListBase *listbase) {
	return (listbase->first && listbase->first == listbase->last);
}

ROSE_INLINE void LIB_listbase_clear(ListBase *listbase) {
	listbase->first = listbase->last = NULL;
}

ROSE_INLINE void LIB_listbase_swap(ListBase *lb1, ListBase *lb2) {
	SWAP(void *, lb1->first, lb2->first);
	SWAP(void *, lb1->last, lb2->last);
}

#ifdef __cplusplus
}
#endif

#endif	// LIB_LISTBASE_INLINE_C
