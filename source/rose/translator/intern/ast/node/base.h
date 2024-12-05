#ifndef RT_AST_NODE_BASE_H
#define RT_AST_NODE_BASE_H

#include "ast/node.h"
#include "ast/type.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

void *RT_node_new(struct RTContext *, const struct RTToken *token, const struct RTType *cast, int kind, int type, size_t length);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_AST_NODE_BASE_H
