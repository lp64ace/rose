#ifndef __RCC_OBJECT_H__
#define __RCC_OBJECT_H__

#ifdef __cplusplus
extern "C" {
#endif

struct RCContext;
struct RCCNode;
struct RCCToken;
struct RCCType;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCObject {
	struct RCCObject *prev, *next;
	
	int kind;
	
	const struct RCCType *type;
	
	const struct RCCToken *identifier;
	const struct RCCNode *body;
} RCCObject;

enum {
	OBJ_VARIABLE,
	OBJ_TYPEDEF,
	OBJ_FUNCTION,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

struct RCCObject *RCC_object_new_variable(struct RCContext *, const struct RCCType *type, const struct RCCToken *token);
struct RCCObject *RCC_object_new_typedef(struct RCContext *, const struct RCCType *type, const struct RCCToken *token);
struct RCCObject *RCC_object_new_function(struct RCContext *, const struct RCCType *type, const struct RCCToken *token, const struct RCCNode *node);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // __RCC_OBJECT_H__
