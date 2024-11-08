#ifndef RT_OBJECT_H
#define RT_OBJECT_H

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
	OBJ_ENUM,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

struct RCCObject *RT_object_new_variable(struct RCContext *, const struct RCCType *type, const struct RCCToken *token);
struct RCCObject *RT_object_new_typedef(struct RCContext *, const struct RCCType *type, const struct RCCToken *token);
struct RCCObject *RT_object_new_function(struct RCContext *, const struct RCCType *type, const struct RCCToken *token,
										 const struct RCCNode *node);
struct RCCObject *RT_object_new_enum(struct RCContext *, const struct RCCType *type, const struct RCCToken *identifier,
									 const struct RCCNode *value);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool RT_object_is_variable(const struct RCCObject *object);
bool RT_object_is_typedef(const struct RCCObject *object);
bool RT_object_is_function(const struct RCCObject *object);
bool RT_object_is_enum(const struct RCCObject *object);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_OBJECT_H
