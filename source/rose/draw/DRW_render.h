#ifndef DRW_RENDER_H
#define DRW_RENDER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct DrawEngineType;
struct Object;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DrawEngineType {
	struct DrawEngineType *prev, *next;

	char name[64];

	void (*cache_init)(void *vdata);
	void (*cache_populate)(void *vdata, struct Object *object);
	void (*cache_finish)(void *vdata);
	void (*draw)(void *vdata);
} DrawEngineType;

#ifdef __cplusplus
}
#endif

#endif // DRW_RENDER_H
