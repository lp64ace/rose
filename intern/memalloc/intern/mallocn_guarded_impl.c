#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <pthread.h>

#include "MEM_alloc.h"
#include "mallocn_intern.h"

#include "linklist.h"

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

/**
 * A structure with data about a memory block; this precedes the memory the user can write into.
 * The pointer returned to the user is the pointer after this data block.
 */
typedef struct MemHead {
	int tag1;
	size_t length;
	struct MemHead *prev, *next;
	const char *name;
	int tag2;
	short pad;
	short alignment;
} MemHead;

typedef MemHead MemHeadAligned;

typedef struct MemTail {
	int tag3;
	int pad;
} MemTail;

#define MAKE_ID(a, b, c, d) ((int)(d) << 24 | (int)(c) << 16 | (b) << 8 | (a))

#define MEMTAG1 MAKE_ID('M', 'E', 'M', 'O')
#define MEMTAG2 MAKE_ID('R', 'Y', 'B', 'L')
#define MEMTAG3 MAKE_ID('O', 'C', 'K', '!')
#define MEMFREE MAKE_ID('F', 'R', 'E', 'E')

/** Returns the #MemHead from a #Link. */
#define MEMGET(x) ((MemHead *)(((char *)x) - offsetof(MemHead, prev)))

#define MEMHEAD_ALIGNED_FROM_PTR(ptr) (((struct MemHeadAligned *)ptr) - 1)
#define MEMHEAD_REAL_PTR(ptr) ((char *)ptr - MEMHEAD_ALIGN_PADDING(ptr->alignment))
#define MEMHEAD_ALIGN_PADDING(align) ((size_t)align - (sizeof(MemHeadAligned) % (size_t)align))

#define MEMHEAD_FROM_PTR(ptr) (((struct MemHead *)ptr) - 1)
#define PTR_FROM_MEMHEAD(ptr) (void *)(ptr + 1)

#define MEMHEAD_IS_ALIGNED(memhead) ((memhead)->alignment != 0)
#define MEMHEAD_LENGTH(memhead) ((memhead)->length)

/* \} */

/* -------------------------------------------------------------------- */
/** \name Variables
 * \{ */

static size_t totblock = 0;
static size_t mem_in_use = 0;
static size_t peak_mem = 0;

static volatile ListBase _membase;
static volatile ListBase *membase = &_membase;

/* \} */

/* -------------------------------------------------------------------- */
/** \name Internal Functions Declarations
 * \{ */

static void mem_lock_thread(void);
static void mem_unlock_thread(void);

static void make_memhead_header(MemHead *memh, size_t len, const char *str);
static void rem_memblock(MemHead *memh);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Function Definitions
 * \{ */

void *MEM_guarded_mallocN(size_t length, const char *str) {
	MemHead *memh;

	length = SIZET_ALIGN_4(length);

	memh = (MemHead *)malloc(length + sizeof(MemHead) + sizeof(MemTail));

	if (memh) {
		make_memhead_header(memh, length, str);

		if (length) {
			memset(memh + 1, 0xff, memh->length);
		}
		return (++memh);
	}
	fprintf(stderr,
			"Malloc returns NULL for length = " SIZET_FORMAT " in %s, total " SIZET_FORMAT "\n",
			SIZET_ARG(length),
			str,
			SIZET_ARG(mem_in_use));
	return NULL;
}

void *MEM_guarded_malloc_arrayN(size_t length, size_t size, const char *str) {
	size_t total_size;
	if (!MEM_size_safe_multiply(length, size, &total_size)) {
		fprintf(stderr,
				"Malloc array aborted due to integer overflow for "
				"length = " SIZET_FORMAT "x" SIZET_FORMAT " in %s, total " SIZET_FORMAT "\n",
				SIZET_ARG(length),
				SIZET_ARG(size),
				str,
				SIZET_ARG(mem_in_use));
		abort();
		return NULL;
	}

	return MEM_guarded_mallocN(total_size, str);
}

void *MEM_guarded_mallocN_aligned(size_t length, size_t align, const char *str) {
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

	MemHead *memh = (MemHead *)aligned_malloc(length + extra_padding + sizeof(MemHead) + sizeof(MemTail), align);

	if (memh) {
		memh = (MemHead *)((char *)memh + extra_padding);

		make_memhead_header(memh, length, str);
		memh->alignment = (short)align;

#ifndef NDEBUG
		if (length) {
			memset(memh + 1, 0xff, length);
		}
#endif
		return PTR_FROM_MEMHEAD(memh);
	}
	fprintf(stderr,
			"Malloc returns NULL for length = " SIZET_FORMAT " in %s, total " SIZET_FORMAT "\n",
			SIZET_ARG(length),
			str,
			SIZET_ARG(memory_usage_current()));
	return NULL;
}

void *MEM_guarded_callocN(size_t length, const char *str) {
	MemHead *memh;

	length = SIZET_ALIGN_4(length);

	memh = (MemHead *)calloc(1, length + sizeof(MemHead) + sizeof(MemTail));

	if (memh) {
		make_memhead_header(memh, length, str);

		return (++memh);
	}
	fprintf(stderr,
			"Malloc returns NULL for length = " SIZET_FORMAT " in %s, total " SIZET_FORMAT "\n",
			SIZET_ARG(length),
			str,
			SIZET_ARG(mem_in_use));
	return NULL;
}

void *MEM_guarded_calloc_arrayN(size_t length, size_t size, const char *str) {
	size_t total_size;
	if (!MEM_size_safe_multiply(length, size, &total_size)) {
		fprintf(stderr,
				"Malloc array aborted due to integer overflow for "
				"length = " SIZET_FORMAT "x" SIZET_FORMAT " in %s, total " SIZET_FORMAT "\n",
				SIZET_ARG(length),
				SIZET_ARG(size),
				str,
				SIZET_ARG(mem_in_use));
		abort();
		return NULL;
	}

	return MEM_guarded_callocN(total_size, str);
}

void *MEM_guarded_dupallocN(const void *vmemh) {
	void *newp = NULL;
	if (vmemh) {
		MemHead *memh = MEMHEAD_FROM_PTR(vmemh);
		const size_t prev_size = MEM_guarded_allocN_len(vmemh);
		if (MEMHEAD_IS_ALIGNED(memh)) {
			newp = MEM_guarded_mallocN_aligned(prev_size, (size_t)memh->alignment, "DupAlloc");
		}
		else {
			newp = MEM_guarded_mallocN(prev_size, "DupAlloc");
		}
		memcpy(newp, vmemh, prev_size);
	}
	return newp;
}

void *MEM_guarded_reallocN_id(void *vmemh, size_t length, const char *str) {
	void *newp = NULL;

	if (vmemh) {
		MemHead *memh = MEMHEAD_FROM_PTR(vmemh);
		size_t old_len = MEM_guarded_allocN_len(vmemh);

		if (!MEMHEAD_IS_ALIGNED(memh)) {
			newp = MEM_guarded_mallocN(length, "ReAlloc");
		}
		else {
			newp = MEM_guarded_mallocN_aligned(length, (size_t)memh->alignment, "ReAlloc");
		}

		if (newp) {
			if (length < old_len) {
				memcpy(newp, vmemh, length);
			}
			else {
				memcpy(newp, vmemh, old_len);
			}
		}

		MEM_guarded_freeN(vmemh);
	}
	else {
		newp = MEM_guarded_mallocN(length, str);
	}

	return newp;
}

void *MEM_guarded_recallocN_id(void *vmemh, size_t length, const char *str) {
	void *newp = NULL;

	if (vmemh) {
		MemHead *memh = MEMHEAD_FROM_PTR(vmemh);
		size_t old_len = MEM_guarded_allocN_len(vmemh);

		if (!MEMHEAD_IS_ALIGNED(memh)) {
			newp = MEM_guarded_mallocN(length, "ReClearAlloc");
		}
		else {
			newp = MEM_guarded_mallocN_aligned(length, (size_t)memh->alignment, "ReClearAlloc");
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

		MEM_guarded_freeN(vmemh);
	}
	else {
		newp = MEM_guarded_mallocN(length, str);
	}

	return newp;
}

void MEM_guarded_freeN(void *vmemh) {
	MemTail *memt;
	MemHead *memh = vmemh;

	if (!vmemh) {
		fprintf(stderr, "Attempt to free NULL pointer.\n");
#ifndef NDEBUG
		abort();
#endif
		return;
	}

	if (sizeof(intptr_t) == 8) {
		if (((intptr_t)memh) & 0x7) {
			fprintf(stderr, "Attempt to free illegal pointer.\n");
			return;
		}
	}
	else {
		if (((intptr_t)memh) & 0x3) {
			fprintf(stderr, "Attempt to free illegal pointer.\n");
			return;
		}
	}

	memh--;
	if (memh->tag1 == MEMFREE && memh->tag2 == MEMFREE) {
		fprintf(stderr, "Attempt to double free.\n");
		return;
	}

	if ((memh->tag1 == MEMTAG1) && (memh->tag2 == MEMTAG2) && ((memh->length & 0x3) == 0)) {
		memt = (MemTail *)(((char *)memh) + sizeof(MemHead) + memh->length);
		if (memt->tag3 == MEMTAG3) {
			if (leak_detector_has_run) {
				fprintf(stderr, "[%s] %s\n", memh->name, free_after_leak_detection_message);
			}

			memh->tag1 = MEMFREE;
			memh->tag2 = MEMFREE;
			memt->tag3 = MEMFREE;
			/** Keep after tags!!! */
			rem_memblock(memh);

			return;
		}
		fprintf(stderr, "[%s] MemTail is Corrupt\n", memh->name);
	}
	else {
		fprintf(stderr, "Error in header\n");
	}

	totblock = totblock - (size_t)-1;
}

size_t MEM_guarded_allocN_len(const void *vmemh) {
	if (vmemh) {
		MemHead *memh = MEMHEAD_FROM_PTR(vmemh);
		return MEMHEAD_LENGTH(memh);
	}
	return 0;
}

const char *MEM_guarded_name_ptr(void *vmemh) {
	if (vmemh) {
		MemHead *memh = MEMHEAD_FROM_PTR(vmemh);
		return memh->name;
	}
	return "NULL";
}

void MEM_guarded_name_ptr_set(void *vmemh, const char *str) {
	if (vmemh) {
		MemHead *memh = MEMHEAD_FROM_PTR(vmemh);
		memh->name = str;
	}
}

void MEM_guarded_printmemlist(void) {
	MemHead *membl;

	mem_lock_thread();

	printf("MemBase = {\n");

	membl = (membase->first) ? MEMGET(membase->first) : NULL;
	while (membl) {
		printf("\t[name = %s, length = " SIZET_FORMAT ", pointer = %p],\n", membl->name, membl->length, membl);

		membl = (membl->next) ? MEMGET(membl->next) : NULL;
	}

	printf("};\n");

	mem_unlock_thread();
}

size_t MEM_guarded_get_memory_in_use(void) {
	mem_lock_thread();
	const size_t mem_in_use_fetched = mem_in_use;
	mem_unlock_thread();
	
	return mem_in_use_fetched;
}

size_t MEM_guarded_get_memory_blocks_in_use(void) {
	mem_lock_thread();
	const size_t totblock_fetched = totblock;
	mem_unlock_thread();
	
	return totblock_fetched;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Internal Functions Definitions
 * \{ */

static pthread_mutex_t thread_lock = PTHREAD_MUTEX_INITIALIZER;

static void mem_lock_thread(void) {
	pthread_mutex_lock(&thread_lock);
}

static void mem_unlock_thread(void) {
	pthread_mutex_unlock(&thread_lock);
}

static void make_memhead_header(MemHead *memh, size_t length, const char *str) {
	MemTail *memt;

	memh->tag1 = MEMTAG1;
	memh->name = str;
	memh->length = length;
	memh->alignment = 0;
	memh->tag2 = MEMTAG2;

	memt = (MemTail *)(((char *)memh) + sizeof(MemHead) + length);
	memt->tag3 = MEMTAG3;

	mem_lock_thread();
	totblock = totblock + (size_t)1;
	mem_in_use = mem_in_use + (size_t)length;
	mem_unlock_thread();

	mem_lock_thread();
	addlink(membase, (Link *)(&memh->prev));
	peak_mem = mem_in_use > peak_mem ? mem_in_use : peak_mem;
	mem_unlock_thread();
}

static void rem_memblock(MemHead *memh) {
	mem_lock_thread();
	remlink(membase, (Link *)(&memh->prev));
	mem_unlock_thread();

	mem_lock_thread();
	totblock = totblock - (size_t)1;
	mem_in_use = mem_in_use - (size_t)memh->length;
	mem_unlock_thread();

	if (memh->length) {
		memset(memh + 1, 0xff, memh->length);
	}
	if (memh->alignment == 0) {
		free(memh);
	}
	else {
		aligned_free(MEMHEAD_REAL_PTR(memh));
	}
}

/* \} */
