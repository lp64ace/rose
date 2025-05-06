#ifndef LIB_GSQUEUE_H
#define LIB_GSQUEUE_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _GSQueue GSQueue;

GSQueue *LIB_gsqueue_new(size_t elem_size);

/**
 * Returns true if the queue is empty, false otherwise.
 */
bool LIB_gsqueue_is_empty(const GSQueue *queue);

size_t LIB_gsqueue_len(const GSQueue *queue);

/**
 * Retrieves and removes the first element from the queue.
 * The value is copies to \a r_item, which must be at least \a elem_size bytes.
 *
 * Does not reduce amount of allocated memory.
 */
void LIB_gsqueue_pop(GSQueue *queue, void *r_item);

/**
 * Copies the source value onto the end of the queue
 *
 * \note This copies #GSQueue.elem_size bytes from \a item,
 * (the pointer itself is not stored).
 *
 * \param item: source data to be copied to the queue.
 */
void LIB_gsqueue_push(GSQueue *queue, const void *item);

/**
 * Free the queue's data and the queue itself.
 */
void LIB_gsqueue_free(GSQueue *queue);

#ifdef __cplusplus
}
#endif

#endif	// LIB_GSQUEUE_H
