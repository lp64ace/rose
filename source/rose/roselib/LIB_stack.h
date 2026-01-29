#ifndef LIB_STACK_H
#define LIB_STACK_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RoseStack RoseStack;

RoseStack *LIB_stack_new_ex(size_t elem, const char *description, size_t chunksize);
RoseStack *LIB_stack_new(size_t elem, const char *description);

void LIB_stack_discard(RoseStack *stack);
void LIB_stack_free(RoseStack *stack);

void LIB_stack_push(RoseStack *stack, const void *src);
void LIB_stack_pop(RoseStack *stack, void *dst);
void *LIB_stack_peek(RoseStack *stack);

bool LIB_stack_is_empty(const RoseStack *stack);

#ifdef __cplusplus
}
#endif

#endif	// !LIB_STACK_H
