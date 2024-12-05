#ifndef RT_OBJECT_H
#define RT_OBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

struct RTContext;
struct RTNode;
struct RTToken;
struct RTType;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RTObject {
	struct RTObject *prev, *next;

	int kind;

	const struct RTType *type;

	const struct RTToken *identifier;
	const struct RTNode *body;
} RTObject;

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

struct RTObject *RT_object_new_variable(struct RTContext *, const struct RTType *type, const struct RTToken *token);
struct RTObject *RT_object_new_typedef(struct RTContext *, const struct RTType *type, const struct RTToken *token);
struct RTObject *RT_object_new_function(struct RTContext *, const struct RTType *type, const struct RTToken *token, const struct RTNode *node);
struct RTObject *RT_object_new_enum(struct RTContext *, const struct RTType *type, const struct RTToken *identifier, const struct RTNode *value);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool RT_object_is_variable(const struct RTObject *object);
bool RT_object_is_typedef(const struct RTObject *object);
bool RT_object_is_function(const struct RTObject *object);
bool RT_object_is_enum(const struct RTObject *object);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_OBJECT_H
