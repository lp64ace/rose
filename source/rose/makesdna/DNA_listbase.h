#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Link {
	struct Link *prev, *next;
} Link;

typedef struct ListBase {
	struct Link *first, *last;
} ListBase;

#ifdef __cplusplus
}
#endif
