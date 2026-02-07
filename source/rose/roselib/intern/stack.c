#include "MEM_guardedalloc.h"

#include "LIB_stack.h"
#include "LIB_utildefines.h"

#define USE_TOTELEM

#define CHUNK_EMPTY ((size_t)-1)
/* target chunks size: 64kb */
#define CHUNK_SIZE_DEFAULT (1 << 16)
/* ensure we get at least this many elems per chunk */
#define CHUNK_ELEM_MIN 32

typedef struct StackChunk {
	struct StackChunk *next;
	char data[0];
} StackChunk;

typedef struct RoseStack {
	struct StackChunk *chunk_curr; /* currently active chunk */
	struct StackChunk *chunk_free; /* free chunks */
	size_t chunk_index;			   /* index into 'chunk_curr' */
	size_t chunk_elem_max;		   /* number of elements per chunk */
	size_t elem_size;
#ifdef USE_TOTELEM
	size_t elem_num;
#endif
} RoseStack;


/**
 * \return number of elements per chunk, optimized for slop-space.
 */
static size_t stack_chunk_elem_max_calc(const size_t elem_size, size_t chunk_size) {
	/* get at least this number of elems per chunk */
	const size_t elem_size_min = elem_size * CHUNK_ELEM_MIN;
	ROSE_assert((elem_size != 0) && (chunk_size != 0));
	while (chunk_size <= elem_size_min) {
		chunk_size <<= 1;
	}
	/* account for slop-space */
	chunk_size -= (sizeof(struct StackChunk) + MEM_SIZE_OVERHEAD);
	return chunk_size / elem_size;
}

RoseStack *LIB_stack_new_ex(size_t elem, const char *description, size_t chunksize) {
	RoseStack *stack = MEM_callocN(sizeof(*stack), description);

	stack->chunk_elem_max = stack_chunk_elem_max_calc(elem, chunksize);
	stack->elem_size = elem;
	/* force init */
	stack->chunk_index = stack->chunk_elem_max - 1;

	return stack;
}

RoseStack *LIB_stack_new(size_t elem, const char *description) {
	return LIB_stack_new_ex(elem, description, CHUNK_SIZE_DEFAULT);
}

static void stack_free_chunks(struct StackChunk *data) {
	while (data) {
		struct StackChunk *data_next = data->next;
		MEM_freeN(data);
		data = data_next;
	}
}

void LIB_stack_free(RoseStack *stack) {
	stack_free_chunks(stack->chunk_curr);
	stack_free_chunks(stack->chunk_free);
	MEM_freeN(stack);
}

void LIB_stack_discard(RoseStack *stack) {
	ROSE_assert(LIB_stack_is_empty(stack) == false);

#ifdef USE_TOTELEM
	stack->elem_num--;
#endif
	if (--stack->chunk_index == CHUNK_EMPTY) {
		struct StackChunk *chunk_free;

		chunk_free = stack->chunk_curr;
		stack->chunk_curr = stack->chunk_curr->next;

		chunk_free->next = stack->chunk_free;
		stack->chunk_free = chunk_free;

		stack->chunk_index = stack->chunk_elem_max - 1;
	}
}

static void *stack_get_last_elem(RoseStack *stack) {
	return ((char *)(stack)->chunk_curr->data) + ((stack)->elem_size * (stack)->chunk_index);
}

void *LIB_stack_push_r(RoseStack *stack) {
	stack->chunk_index++;

	if (stack->chunk_index == stack->chunk_elem_max) {
		struct StackChunk *chunk;
		if (stack->chunk_free) {
			chunk = stack->chunk_free;
			stack->chunk_free = chunk->next;
		}
		else {
			chunk = MEM_mallocN(sizeof(*chunk) + (stack->elem_size * stack->chunk_elem_max), __func__);
		}
		chunk->next = stack->chunk_curr;
		stack->chunk_curr = chunk;
		stack->chunk_index = 0;
	}

	ROSE_assert(stack->chunk_index < stack->chunk_elem_max);

#ifdef USE_TOTELEM
	stack->elem_num++;
#endif

	/* Return end of stack */
	return stack_get_last_elem(stack);
}

void LIB_stack_push(RoseStack *stack, const void *src) {
	void *dst = LIB_stack_push_r(stack);
	memcpy(dst, src, stack->elem_size);
}

void LIB_stack_pop(RoseStack *stack, void *dst) {
	ROSE_assert(LIB_stack_is_empty(stack) == false);
	memcpy(dst, stack_get_last_elem(stack), stack->elem_size);
	LIB_stack_discard(stack);
}

void *LIB_stack_peek(RoseStack *stack) {
	ROSE_assert(LIB_stack_is_empty(stack) == false);
	return stack_get_last_elem(stack);
}

bool LIB_stack_is_empty(const RoseStack *stack) {
#ifdef USE_TOTELEM
	ROSE_assert((stack->chunk_curr == NULL) == (stack->elem_num == 0));
#endif
	return (stack->chunk_curr == NULL);
}
