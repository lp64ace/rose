#pragma once

#include "DNA_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Link {
	struct Link *prev, *next;
} Link;

DNA_ACTION_DEFINE(Link, DNA_ACTION_STORE);

typedef struct ListBase {
	struct Link *first, *last;
} ListBase;

DNA_ACTION_DEFINE(ListBase, DNA_ACTION_STORE);

typedef struct LinkData {
	struct LinkData *prev, *next;
	
	void *data;
} LinkData;

/** Name doesn't really fit this here, maybe change it to DNA_ACTION_STORE_WEAK? */
DNA_ACTION_DEFINE(LinkData, DNA_ACTION_RUNTIME);

#ifdef __cplusplus
}
#endif
