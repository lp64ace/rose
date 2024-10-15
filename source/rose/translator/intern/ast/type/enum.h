#ifndef RT_AST_TYPE_ENUM_H
#define RT_AST_TYPE_ENUM_H

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RCCType;
struct RCCNode;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct EnumItem {
	struct EnumItem *prev, *next;

	/** The identifier that describes the name of this enumerator item. */
	const struct RCCToken *identifier;
	/**
	 * The expression that describes the value of this enumerator item,
	 * this has to be a constant expression and should always evaluate to an integer.
	 */
	const struct RCCNode *value;
} EnumItem;

typedef struct RCCTypeEnum {
	unsigned int is_complete : 1;

	/** The identifier that describes the typename of this enumerator type. */
	struct RCCToken *identifier;
	/**
	 * The type that is used when accessing a value of this enumerator type,
	 * by default we use the language specified but the user can override this.
	 *
	 * e.g. The underlying type for `enum : unsigned char { ... };` would be `Tp_UChar`.
	 */
	const struct RCCType *underlying_type;

	/** A double linked list of all the items, `EnumItem`, that are defined within this enum. */
	ListBase items;
} RCCTypeEnum;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

struct RCCType *RT_type_new_enum(struct RCContext *, struct RCCToken *identifier, const struct RCCType *underlying_type);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void RT_type_enum_add_constant_expr(struct RCContext *, struct RCCType *e, const struct RCCToken *identifier, const struct RCCNode *expr);
void RT_type_enum_add_constant_auto(struct RCContext *, struct RCCType *e, const struct RCCToken *identifier);

/** Returns if the specified enumerator type has the specified item or not. */
bool RT_type_enum_has(const struct RCCType *e, const struct RCCToken *identifier);
bool RT_type_enum_has_value(const struct RCCType *e, long long value);
/** Returns the constant expression that describes the value of the specified item. */
const struct RCCNode *RT_type_enum_value(const struct RCCType *e, const struct RCCToken *identifier);

void RT_type_enum_finalize(struct RCContext *, struct RCCType *e);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_AST_TYPE_ENUM_H
