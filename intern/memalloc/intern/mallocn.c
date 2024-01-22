#include "MEM_alloc.h"
#include "mallocn_intern.h"

#include <assert.h>

/* -------------------------------------------------------------------- */
/** \name Methods Declaration
 * \{ */

#ifdef NDEBUG

/** Lockfree Allocator (for Perfomance) */

size_t (*MEM_allocN_len)(const void *vmemh) = MEM_lockfree_allocN_len;
void *(*MEM_mallocN)(size_t length, const char *str) = MEM_lockfree_mallocN;
void *(*MEM_malloc_arrayN)(size_t length, size_t size, const char *str) = MEM_lockfree_malloc_arrayN;
void *(*MEM_mallocN_aligned)(size_t length, size_t align, const char *str) = MEM_lockfree_mallocN_aligned;
void *(*MEM_callocN)(size_t length, const char *str) = MEM_lockfree_callocN;
void *(*MEM_calloc_arrayN)(size_t length, size_t size, const char *str) = MEM_lockfree_calloc_arrayN;
void *(*MEM_dupallocN)(const void *vmemh) = MEM_lockfree_dupallocN;
void *(*MEM_reallocN_id)(void *vmemh, size_t length, const char *str) = MEM_lockfree_reallocN_id;
void *(*MEM_recallocN_id)(void *vmemh, size_t length, const char *str) = MEM_lockfree_recallocN_id;
void (*MEM_freeN)(void *vmemh) = MEM_lockfree_freeN;
void (*MEM_printmemlist)(void) = MEM_lockfree_printmemlist;
size_t (*MEM_get_memory_in_use)(void) = MEM_lockfree_get_memory_in_use;
size_t (*MEM_get_memory_blocks_in_use)(void) = MEM_lockfree_get_memory_blocks_in_use;

#else

/** Guarded Allocator (for Debugging) */

size_t (*MEM_allocN_len)(const void *vmemh) = MEM_guarded_allocN_len;
void *(*MEM_mallocN)(size_t length, const char *str) = MEM_guarded_mallocN;
void *(*MEM_malloc_arrayN)(size_t length, size_t size, const char *str) = MEM_guarded_malloc_arrayN;
void *(*MEM_mallocN_aligned)(size_t length, size_t align, const char *str) = MEM_guarded_mallocN_aligned;
void *(*MEM_callocN)(size_t length, const char *str) = MEM_guarded_callocN;
void *(*MEM_calloc_arrayN)(size_t length, size_t size, const char *str) = MEM_guarded_calloc_arrayN;
void *(*MEM_dupallocN)(const void *vmemh) = MEM_guarded_dupallocN;
void *(*MEM_reallocN_id)(void *vmemh, size_t length, const char *str) = MEM_guarded_reallocN_id;
void *(*MEM_recallocN_id)(void *vmemh, size_t length, const char *str) = MEM_guarded_recallocN_id;
void (*MEM_freeN)(void *vmemh) = MEM_guarded_freeN;
const char *(*MEM_name_ptr)(void *vmemh) = MEM_guarded_name_ptr;
void (*MEM_name_ptr_set)(void *vmemh, const char *str) = MEM_guarded_name_ptr_set;
void (*MEM_printmemlist)(void) = MEM_guarded_printmemlist;
size_t (*MEM_get_memory_in_use)(void) = MEM_guarded_get_memory_in_use;
size_t (*MEM_get_memory_blocks_in_use)(void) = MEM_guarded_get_memory_blocks_in_use;

#endif

/* \} */

/* -------------------------------------------------------------------- */
/** \name Public API Methods
 * \{ */

void MEM_use_lockfree_allocator(void) {
	MEM_allocN_len = MEM_lockfree_allocN_len;
	MEM_mallocN = MEM_lockfree_mallocN;
	MEM_malloc_arrayN = MEM_lockfree_malloc_arrayN;
	MEM_mallocN_aligned = MEM_lockfree_mallocN_aligned;
	MEM_callocN = MEM_lockfree_callocN;
	MEM_calloc_arrayN = MEM_lockfree_calloc_arrayN;
	MEM_dupallocN = MEM_lockfree_dupallocN;
	MEM_reallocN_id = MEM_lockfree_reallocN_id;
	MEM_recallocN_id = MEM_lockfree_recallocN_id;
	MEM_freeN = MEM_lockfree_freeN;
#ifndef NDEBUG
	MEM_name_ptr = MEM_lockfree_name_ptr;
	MEM_name_ptr_set = MEM_lockfree_name_ptr_set;
#endif
	MEM_printmemlist = MEM_lockfree_printmemlist;
	MEM_get_memory_in_use = MEM_lockfree_get_memory_in_use;
	MEM_get_memory_blocks_in_use = MEM_lockfree_get_memory_blocks_in_use;
}

void MEM_use_guarded_allocator(void) {
	MEM_allocN_len = MEM_guarded_allocN_len;
	MEM_mallocN = MEM_guarded_mallocN;
	MEM_malloc_arrayN = MEM_guarded_malloc_arrayN;
	MEM_mallocN_aligned = MEM_guarded_mallocN_aligned;
	MEM_callocN = MEM_guarded_callocN;
	MEM_calloc_arrayN = MEM_guarded_calloc_arrayN;
	MEM_dupallocN = MEM_guarded_dupallocN;
	MEM_reallocN_id = MEM_guarded_reallocN_id;
	MEM_recallocN_id = MEM_guarded_recallocN_id;
	MEM_freeN = MEM_guarded_freeN;
#ifndef NDEBUG
	MEM_name_ptr = MEM_guarded_name_ptr;
	MEM_name_ptr_set = MEM_guarded_name_ptr_set;
#endif
	MEM_printmemlist = MEM_guarded_printmemlist;
	MEM_get_memory_in_use = MEM_guarded_get_memory_in_use;
	MEM_get_memory_blocks_in_use = MEM_guarded_get_memory_blocks_in_use;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Aligned Alloc Methods
 * \{ */

void *aligned_malloc(size_t size, size_t alignment) {
	/** #posix_memalign requires alignment to be a multiple of `sizeof(void *)`. */
	assert(alignment >= ALIGNED_MALLOC_MINIMUM_ALIGNMENT);

#ifdef _WIN32
	return _aligned_malloc(size, alignment);
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	void *result;

	if (posix_memalign(&result, alignment, size)) {
		/**
		 * non-zero means allocation error
		 * either no allocation or bad alignment value
		 */
		return NULL;
	}
	return result;
#else
	return memalign(alignment, size);
#endif
}

void aligned_free(void *ptr) {
#ifdef _WIN32
	_aligned_free(ptr);
#else
	free(ptr);
#endif
}

/* \} */
