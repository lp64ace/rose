#pragma once

#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct rcti;

/* gpu_select_pick */

void gpu_select_pick_begin(struct GPUSelectResult *buffer, unsigned int buffer_len, const struct rcti *input, enum SelectMode mode);
bool gpu_select_pick_load_id(unsigned int id, bool end);
unsigned int gpu_select_pick_end(void);

void gpu_select_pick_cache_begin(void);
void gpu_select_pick_cache_end(void);
/**
 * \return true if drawing is not needed.
 */
bool gpu_select_pick_is_cached(void);
void gpu_select_pick_cache_load_id(void);

/* gpu_select_sample_query */

void gpu_select_query_begin(struct GPUSelectResult *buffer, unsigned int buffer_len, const struct rcti *input, enum SelectMode mode, int oldhits);
bool gpu_select_query_load_id(unsigned int id);
unsigned int gpu_select_query_end(void);

#define SELECT_ID_NONE ((unsigned int)0xffffffff)

#if defined(__cplusplus)
}
#endif
