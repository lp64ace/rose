#pragma once

#if !defined(__APPLE__) && !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__)
#	include <malloc.h>
#else
#	include <stdlib.h>
#endif

/* visual studio 2012 does not define inline for C */
#ifdef _MSC_VER
#	define MEM_INLINE static __inline
#else
#	define MEM_INLINE static inline
#endif

#include "mallocn_inline.h"

#define SIZET_FORMAT "%zu"
#define SIZET_ARG(a) ((size_t)(a))

#define IS_POW2(a) (((a) & ((a)-1)) == 0)

#define SIZET_ALIGN_4(len) ((len + 3) & ~(size_t)3)

#ifdef __cplusplus
extern "C" {
#endif

extern bool leak_detector_has_run;
extern char free_after_leak_detection_message[];

void memory_usage_init(void);
void memory_usage_block_alloc(size_t size);
void memory_usage_block_free(size_t size);
size_t memory_usage_block_num(void);
size_t memory_usage_current(void);
size_t memory_usage_peak(void);
void memory_usage_peak_reset(void);

/* -------------------------------------------------------------------- */
/** \name Lockfree Methods Declaration
 * \{ */

void *MEM_lockfree_mallocN(size_t length, const char *str);
void *MEM_lockfree_malloc_arrayN(size_t length, size_t size, const char *str);
void *MEM_lockfree_mallocN_aligned(size_t length, size_t align, const char *str);
void *MEM_lockfree_callocN(size_t length, const char *str);
void *MEM_lockfree_calloc_arrayN(size_t length, size_t size, const char *str);
void *MEM_lockfree_dupallocN(const void *vmemh);
void *MEM_lockfree_reallocN_id(void *vmemh, size_t length, const char *str);
void *MEM_lockfree_recallocN_id(void *vmemh, size_t length, const char *str);
void MEM_lockfree_freeN(void *vmemh);
size_t MEM_lockfree_allocN_len(const void *vmemh);
const char *MEM_lockfree_name_ptr(void *vmemh);
void MEM_lockfree_name_ptr_set(void *vmemh, const char *str);
void MEM_lockfree_printmemlist(void);
size_t MEM_lockfree_get_memory_in_use(void);
size_t MEM_lockfree_get_memory_blocks_in_use(void);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Guarded Methods Declaration
 * \{ */

void *MEM_guarded_mallocN(size_t length, const char *str);
void *MEM_guarded_malloc_arrayN(size_t length, size_t size, const char *str);
void *MEM_guarded_mallocN_aligned(size_t length, size_t align, const char *str);
void *MEM_guarded_callocN(size_t length, const char *str);
void *MEM_guarded_calloc_arrayN(size_t length, size_t size, const char *str);
void *MEM_guarded_dupallocN(const void *vmemh);
void *MEM_guarded_reallocN_id(void *vmemh, size_t length, const char *str);
void *MEM_guarded_recallocN_id(void *vmemh, size_t length, const char *str);
void MEM_guarded_freeN(void *vmemh);
size_t MEM_guarded_allocN_len(const void *vmemh);
const char *MEM_guarded_name_ptr(void *vmemh);
void MEM_guarded_name_ptr_set(void *vmemh, const char *str);
void MEM_guarded_printmemlist(void);
size_t MEM_guarded_get_memory_in_use(void);
size_t MEM_guarded_get_memory_blocks_in_use(void);

/* \} */

#define ALIGNED_MALLOC_MINIMUM_ALIGNMENT sizeof(void *)

void *aligned_malloc(size_t size, size_t alignment);
void aligned_free(void *ptr);

#ifdef __cplusplus
}
#endif
