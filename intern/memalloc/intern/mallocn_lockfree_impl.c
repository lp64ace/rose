#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MEM_alloc.h"
#include "mallocn_intern.h"

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

/**
 * A structure with data about a memory block; this precedes the memory the user can write into.
 * The pointer returned to the user is the pointer after this data block.
 */
typedef struct MemHead {
	size_t length;
} MemHead;

typedef struct MemHeadAligned {
	short alignment;
	size_t length;
} MemHeadAligned;

/**
 * We always pad memory up to multiples of 4bytes, so we are safe to assume that the last two bytes of the #MemHead->length are
 * always zero, since they are not used we can use them to stro flags.
 */
enum {
	MEMHEAD_ALIGN_FLAG = (1 << 0),
};

#define MEMHEAD_ALIGNED_FROM_PTR(ptr) (((struct MemHeadAligned *)ptr) - 1)
#define MEMHEAD_REAL_PTR(ptr) ((char *)ptr - MEMHEAD_ALIGN_PADDING(ptr->alignment))
#define MEMHEAD_ALIGN_PADDING(align) ((size_t)align - (sizeof(MemHeadAligned) % (size_t)align))

#define MEMHEAD_FROM_PTR(ptr) (((struct MemHead *)ptr) - 1)
#define PTR_FROM_MEMHEAD(ptr) (void *)(ptr + 1)

#define MEMHEAD_IS_ALIGNED(memhead) ((memhead)->length & (size_t)MEMHEAD_ALIGN_FLAG)
#define MEMHEAD_LENGTH(memhead) ((memhead)->length & ~((size_t)(MEMHEAD_ALIGN_FLAG)))

/* \} */

/* -------------------------------------------------------------------- */
/** \name Function Definitions
 * \{ */

void *MEM_lockfree_mallocN(size_t length, const char *str) {
	MemHead *memh;

	length = SIZET_ALIGN_4(length);

	memh = (MemHead *)malloc(length + sizeof(MemHead));

	if (memh) {
#ifndef NDEBUG
		if (length) {
			memset(memh + 1, 0xff, length);
		}
#endif

		memh->length = length;
		memory_usage_block_alloc(length);

		return PTR_FROM_MEMHEAD(memh);
	}
	fprintf(stderr, "Malloc returns NULL for length = " SIZET_FORMAT " in %s, total " SIZET_FORMAT "\n", SIZET_ARG(length), str, SIZET_ARG(memory_usage_current()));
	return NULL;
}

void *MEM_lockfree_malloc_arrayN(size_t length, size_t size, const char *str) {
	size_t total_size;
	if (!MEM_size_safe_multiply(length, size, &total_size)) {
		fprintf(stderr,
				"Malloc array aborted due to integer overflow for "
				"length = " SIZET_FORMAT "x" SIZET_FORMAT " in %s, total " SIZET_FORMAT "\n",
				SIZET_ARG(length),
				SIZET_ARG(size),
				str,
				SIZET_ARG(memory_usage_current()));
		abort();
		return NULL;
	}

	return MEM_lockfree_mallocN(total_size, str);
}

void *MEM_lockfree_mallocN_aligned(size_t length, size_t align, const char *str) {
	/** Huge alignment values doesn't make sense and they wouldn't fit into 'short' used in the MemHead. */
	assert(align < 1024);
	/** We only support alignments that are a power of two. */
	assert(IS_POW2(align));

	if (align < ALIGNED_MALLOC_MINIMUM_ALIGNMENT) {
		align = ALIGNED_MALLOC_MINIMUM_ALIGNMENT;
	}
	/**
	 * It's possible that MemHead's size is not properly aligned, do extra padding to deal with this.
	 * We only support small alignments which fits into short in order to save some bits in MemHead structure.
	 */
	size_t extra_padding = MEMHEAD_ALIGN_PADDING(align);

	length = SIZET_ALIGN_4(length);

	MemHeadAligned *memh = (MemHeadAligned *)aligned_malloc(length + extra_padding + sizeof(MemHeadAligned), align);

	if (memh) {
		memh = (MemHeadAligned *)((char *)memh + extra_padding);

#ifndef NDEBUG
		if (length) {
			memset(memh + 1, 0xff, length);
		}
#endif

		memh->length = length | (size_t)MEMHEAD_ALIGN_FLAG;
		memh->alignment = (short)align;
		memory_usage_block_alloc(length);

		return PTR_FROM_MEMHEAD(memh);
	}
	fprintf(stderr, "Malloc returns NULL for length = " SIZET_FORMAT " in %s, total " SIZET_FORMAT "\n", SIZET_ARG(length), str, SIZET_ARG(memory_usage_current()));
	return NULL;
}

void *MEM_lockfree_callocN(size_t length, const char *str) {
	MemHead *memh;

	length = SIZET_ALIGN_4(length);

	memh = (MemHead *)calloc(1, length + sizeof(MemHead));

	if (memh) {
		memh->length = length;
		memory_usage_block_alloc(length);

		return PTR_FROM_MEMHEAD(memh);
	}
	fprintf(stderr, "Malloc returns NULL for length = " SIZET_FORMAT " in %s, total " SIZET_FORMAT "\n", SIZET_ARG(length), str, SIZET_ARG(memory_usage_current()));
	return NULL;
}

void *MEM_lockfree_calloc_arrayN(size_t length, size_t size, const char *str) {
	size_t total_size;
	if (!MEM_size_safe_multiply(length, size, &total_size)) {
		fprintf(stderr,
				"Malloc array aborted due to integer overflow for "
				"length = " SIZET_FORMAT "x" SIZET_FORMAT " in %s, total " SIZET_FORMAT "\n",
				SIZET_ARG(length),
				SIZET_ARG(size),
				str,
				SIZET_ARG(memory_usage_current()));
		abort();
		return NULL;
	}

	return MEM_lockfree_mallocN(total_size, str);
}

void *MEM_lockfree_dupallocN(const void *vmemh) {
	void *newp = NULL;
	if (vmemh) {
		MemHead *memh = MEMHEAD_FROM_PTR(vmemh);
		const size_t prev_size = MEM_lockfree_allocN_len(vmemh);
		if (MEMHEAD_IS_ALIGNED(memh)) {
			MemHeadAligned *memh_aligned = MEMHEAD_ALIGNED_FROM_PTR(vmemh);
			newp = MEM_lockfree_mallocN_aligned(prev_size, (size_t)memh_aligned->alignment, "DupAlloc");
		}
		else {
			newp = MEM_lockfree_mallocN(prev_size, "DupAlloc");
		}
		memcpy(newp, vmemh, prev_size);
	}
	return newp;
}

void *MEM_lockfree_reallocN_id(void *vmemh, size_t length, const char *str) {
	void *newp = NULL;

	if (vmemh) {
		MemHead *memh = MEMHEAD_FROM_PTR(vmemh);
		size_t old_len = MEM_lockfree_allocN_len(vmemh);

		if (!MEMHEAD_IS_ALIGNED(memh)) {
			newp = MEM_lockfree_mallocN(length, "ReAlloc");
		}
		else {
			MemHeadAligned *memh_aligned = MEMHEAD_ALIGNED_FROM_PTR(vmemh);
			newp = MEM_lockfree_mallocN_aligned(length, (size_t)memh_aligned->alignment, "ReAlloc");
		}

		if (newp) {
			if (length < old_len) {
				memcpy(newp, vmemh, length);
			}
			else {
				memcpy(newp, vmemh, old_len);
			}
		}

		MEM_lockfree_freeN(vmemh);
	}
	else {
		newp = MEM_lockfree_mallocN(length, str);
	}

	return newp;
}

void *MEM_lockfree_recallocN_id(void *vmemh, size_t length, const char *str) {
	void *newp = NULL;

	if (vmemh) {
		MemHead *memh = MEMHEAD_FROM_PTR(vmemh);
		size_t old_len = MEM_lockfree_allocN_len(vmemh);

		if (!MEMHEAD_IS_ALIGNED(memh)) {
			newp = MEM_lockfree_mallocN(length, "ReClearAlloc");
		}
		else {
			MemHeadAligned *memh_aligned = MEMHEAD_ALIGNED_FROM_PTR(vmemh);
			newp = MEM_lockfree_mallocN_aligned(length, (size_t)memh_aligned->alignment, "ReClearAlloc");
		}

		if (newp) {
			if (length < old_len) {
				memcpy(newp, vmemh, length);
			}
			else {
				memcpy(newp, vmemh, old_len);

				if (length > old_len) {
					/** The memory grew, zero the new bytes. */
					memset(((char *)newp) + old_len, 0, length - old_len);
				}
			}
		}

		MEM_lockfree_freeN(vmemh);
	}
	else {
		newp = MEM_lockfree_mallocN(length, str);
	}

	return newp;
}

void MEM_lockfree_freeN(void *vmemh) {
	if (leak_detector_has_run) {
		fprintf(stderr, "%s\n", free_after_leak_detection_message);
	}
	if (!vmemh) {
		fprintf(stderr, "Attempt to free NULL pointer.\n");
#ifndef NDEBUG
		abort();
#endif
		return;
	}

	MemHead *memh = MEMHEAD_FROM_PTR(vmemh);
	size_t length = MEMHEAD_LENGTH(memh);

	memory_usage_block_free(length);

	if (MEMHEAD_IS_ALIGNED(memh)) {
		MemHeadAligned *memh_aligned = MEMHEAD_ALIGNED_FROM_PTR(vmemh);
		aligned_free(MEMHEAD_REAL_PTR(memh_aligned));
	}
	else {
		free(memh);
	}
}

size_t MEM_lockfree_allocN_len(const void *vmemh) {
	if (vmemh) {
		return MEMHEAD_LENGTH(MEMHEAD_FROM_PTR(vmemh));
	}
	return 0;
}

const char *MEM_lockfree_name_ptr(void *vmemh) {
	if (vmemh) {
		return "Unkown Block Name";
	}
	return "NULL";
}

void MEM_lockfree_name_ptr_set(void *vmemh, const char *str) {
}

void MEM_lockfree_printmemlist(void) {
}

size_t MEM_lockfree_get_memory_in_use(void) {
	return memory_usage_current();
}

size_t MEM_lockfree_get_memory_blocks_in_use(void) {
	return memory_usage_block_num();
}

/* \} **/
