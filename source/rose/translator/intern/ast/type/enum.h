#ifndef RT_AST_TYPE_ENUM_H
#define RT_AST_TYPE_ENUM_H

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RTType;
struct RTNode;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 *
 * These data structures are critical to the functionality and integrity of the DNA module.
 * Any modifications to them—whether it’s altering fields, changing types, or adjusting structure
 * layouts—can have a significant impact on the module's behavior and performance.
 *
 * It's essential to carefully understand how these structures are serialized (written to files)
 * and deserialized (read from files) because incorrect changes may cause issues with data
 * compatibility, corruption, or versioning. Be mindful of any dependencies in the file I/O logic
 * and how different components rely on this data.
 *
 * If updates are necessary, ensure proper testing, version control, and backward compatibility
 * strategies are followed.
 * \{ */

typedef struct RTEnumItem {
	struct RTEnumItem *prev, *next;

	/** The identifier that describes the name of this enumerator item. */
	const struct RTToken *identifier;
	/**
	 * The expression that describes the value of this enumerator item,
	 * this has to be a constant expression and should always evaluate to an integer.
	 */
	const struct RTNode *value;
} RTEnumItem;

typedef struct RTTypeEnum {
	bool is_complete;

	/** The identifier that describes the typename of this enumerator type. */
	struct RTToken *identifier;
	/**
	 * The type that is used when accessing a value of this enumerator type,
	 * by default we use the language specified but the user can override this.
	 *
	 * e.g. The underlying type for `enum : unsigned char { ... };` would be `Tp_UChar`.
	 */
	const struct RTType *underlying_type;

	/** A double linked list of all the items, `EnumItem`, that are defined within this enum. */
	ListBase items;
} RTTypeEnum;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

struct RTType *RT_type_new_enum(struct RTContext *, struct RTToken *identifier, const struct RTType *underlying_type);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void RT_type_enum_add_constant_expr(struct RTContext *, struct RTType *e, const struct RTToken *identifier, const struct RTNode *expr);
void RT_type_enum_add_constant_auto(struct RTContext *, struct RTType *e, const struct RTToken *identifier);

/** Returns if the specified enumerator type has the specified item or not. */
bool RT_type_enum_has(const struct RTType *e, const struct RTToken *identifier);
bool RT_type_enum_has_value(const struct RTType *e, long long value);
/** Returns the constant expression that describes the value of the specified item. */
const struct RTNode *RT_type_enum_value(const struct RTType *e, const struct RTToken *identifier);

void RT_type_enum_finalize(struct RTContext *, struct RTType *e);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_AST_TYPE_ENUM_H
