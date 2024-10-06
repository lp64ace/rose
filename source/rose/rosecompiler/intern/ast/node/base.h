#ifndef __RCC_AST_NODE_BASE_H__
#define __RCC_AST_NODE_BASE_H__

#include "ast/node.h"
#include "ast/type.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

void *RCC_node_new(struct RCContext *, const struct RCCToken *token, int kind, int type, size_t length);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // __RCC_AST_NODE_BASE_H__
