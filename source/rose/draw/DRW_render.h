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

typedef unsigned char DRWViewportEmptyList;

#define DRW_VIEWPORT_LIST_SIZE(list) (sizeof(list) == sizeof(DRWViewportEmptyList) ? 0 : (sizeof(list) / sizeof(void *)))

typedef struct DrawEngineDataSize {
	int fbl_len;
	int txl_len;
	int psl_len;
	int stl_len;
} DrawEngineDataSize;

#define DRW_VIEWPORT_DATA_SIZE(type)                    \
	{                                                   \
		DRW_VIEWPORT_LIST_SIZE(*(((type *)NULL)->fbl)), \
		DRW_VIEWPORT_LIST_SIZE(*(((type *)NULL)->txl)), \
		DRW_VIEWPORT_LIST_SIZE(*(((type *)NULL)->psl)), \
		DRW_VIEWPORT_LIST_SIZE(*(((type *)NULL)->stl)), \
	}

typedef struct DrawEngineType {
	struct DrawEngineType *prev, *next;

	char name[64];

	/**
	 * For the general/base format of the Engine's draw data refer to the
	 * #ViewportEngineData struct defined in `intern/draw_engine.h`.
	 */
	const DrawEngineDataSize *vdata_size;

	void (*engine_init)(void *vdata);

	void (*cache_init)(void *vdata);
	void (*cache_populate)(void *vdata, struct Object *object);
	void (*cache_finish)(void *vdata);

	void (*draw)(void *vdata);

	void (*engine_free)(void);
} DrawEngineType;

#ifdef __cplusplus
}
#endif

#endif	// DRW_RENDER_H
