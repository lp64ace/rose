#pragma once

#include "LIB_listbase.h"

#ifdef __cplusplus
extern "C" {
#endif

ROSE_INLINE bool LIB_listbase_is_empty(const struct ListBase *listbase) {
	return (listbase->first == NULL && listbase->first == listbase->last);
}

ROSE_INLINE bool LIB_listbase_is_single(const struct ListBase *listbase) {
	return (listbase->first && listbase->first == listbase->last);
}

ROSE_INLINE void LIB_listbase_clear(struct ListBase *listbase) {
	listbase->first = listbase->last = NULL;
}

#ifdef __cplusplus
}
#endif
