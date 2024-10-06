#ifndef __RCC_TYPE_ENUM_H__
#define __RCC_TYPE_ENUM_H__

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
	struct RCCType *underlying_type;

	/** A double linked list of all the items, `EnumItem`, that are defined within this enum. */
	ListBase items;
} RCCTypeEnum;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

struct RCCType *RCC_type_new_enum(struct RCContext *, struct RCCToken *identifier, struct RCCType *underlying_type);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void RCC_type_enum_add_constant_expr(struct RCContext *, struct RCCType *e, struct RCCToken *identifier, struct RCCNodeConstexpr *expr);
void RCC_type_enum_add_constant_auto(struct RCContext *, struct RCCType *e, struct RCCToken *identifier);

/** Returns if the specified enumerator type has the specified item or not. */
bool RCC_type_enum_has(const struct RCCType *e, const struct RCCToken *identifier);
/** Returns the constant expression that describes the value of the specified item. */
struct RCCNodeConstexpr *RCC_type_enum_value(const struct RCCType *e, const struct RCCToken *identifier);

void RCC_type_enum_finalize(struct RCCType *e);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// __RCC_TYPE_ENUM_H__
