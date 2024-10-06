#ifndef __RCC_TYPE_H__
#define __RCC_TYPE_H__

#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RCCToken;

/**
 * \brief Clarification on memory ownership and management of the #RCCType structure.
 *
 * The memory management of the #RCCType structure follows specific ownership rules that ensure 
 * proper handling and avoid memory leaks during parsing and compilation:
 *
 * - **#RCCParser and #RCCObject Ownership:**
 *   - The #RCCParser is responsible for generating and managing a list of #RCCObject instances 
 *     during the compilation process.
 *   - Each #RCCObject owns its corresponding #RCCType, meaning that the object's type is stored 
 *     and managed by the object itself.
 *
 * - **Structure Members and Ownership:**
 *   - Structure members are associated with their own #RCCObject instances.
 *   - This means that the #RCCType of each structure member is owned by the individual member's 
 *     #RCCObject and **not** by the #RCCType of the structure itself.
 *   - In other words, the #RCCMember->type field, representing the type of a structure member, 
 *     is not owned by the #RCCType that contains the #RCCMember.
 * 
 * **tl;dr:** "#RCCMember->type is not owned by the #RCCType that owns the #RCCMember structure!"
 *
 * - **Function Parameters and Ownership:**
 *   - Function parameters are also associated with separate #RCCObject instances.
 *   - Similar to structure members, the #RCCType for each parameter of a function is owned by the 
 *     respective parameter's #RCCObject and not by the #RCCType of the function.
 *   - The #RCCType->params list (which holds the function's parameters) only references these 
 *     types and does not own the memory associated with them.
 * 
 * **tl;dr:** "The #RCCType->params list does not own its contents - #RCCType(s)!"
 * 
 * This ensures that memory for types is managed consistently, with each object retaining 
 * ownership of its own type, while the structures or functions only maintain references.
 */


/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCType RCCType;

enum {
	MEMBER_IS_BITFIELD = (1 << 0),
};

/** \} */

enum {
	TP_VOID,
	TP_BOOL,
	TP_CHAR,
	TP_SHORT,
	TP_INT,
	TP_LONG,
	TP_FLOAT,
	TP_DOUBLE,
	TP_LDOUBLE,
	TP_ENUM,
	TP_PTR,
	TP_FUNC,
	TP_ARRAY,
	TP_STRUCT,
	TP_UNION,
};

typedef struct RCCMember {
	struct RCCMember *prev, *next;
	
	/** No owned by us for the same reason the base type is not owned by us. */
	const struct RCCType *type;
	const struct RCCToken *token;
	const struct RCCToken *name;
	
	size_t index;
	size_t align;
	size_t offset;
	
	int flag;
	
	/** If this is a bitfield, this is the offset of the bitfield. */
	size_t bit_offset;
	/** If this is a bitfield, this is the width of the bitfield. */
	size_t bit_width;
} RCCMember;

/* -------------------------------------------------------------------- */
/** \name Create Functions
 * \{ */

RCCType *RCC_type_duplicate(const RCCType *tp);
RCCType *RCC_type_pointer(const RCCType *tp);
RCCType *RCC_type_array(const RCCType *tp, size_t length);
RCCType *RCC_type_function(const RCCType *tp);

RCCType *RCC_type_enum();
RCCType *RCC_type_struct();

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Functions
 * \{ */

void RCC_type_free(RCCType *type);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Query Functions
 * \{ */

bool RCC_type_is_scalar(const RCCType *type);
/** Check if this type can store real (floating-point) numbers. */
bool RCC_type_is_real(const RCCType *type);
/** Check if this type can store integer or natural numbers. */
bool RCC_type_is_integer(const RCCType *type);

bool RCC_type_unsigned(const RCCType *type);
bool RCC_type_atomic(const RCCType *type);
bool RCC_type_flexible(const RCCType *type);
bool RCC_type_packed(const RCCType *type);
bool RCC_type_variadic(const RCCType *type);

size_t RCC_type_align(const RCCType *type);
size_t RCC_type_size(const RCCType *type);
size_t RCC_type_length(const RCCType *type);

int RCC_type_kind(const RCCType *type);

/** Returns the base type of a pointer or an array. */
const RCCType *RCC_type_base(const RCCType *type);
/** Returns the return type of a function. */
const RCCType *RCC_type_ret(const RCCType *type);

const RCCMember *RCC_member_first(const RCCType *type);
const RCCMember *RCC_member_last(const RCCType *type);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Update Functions
 * \{ */

void RCC_type_make_atomic(RCCType *type, bool value);
void RCC_type_make_variadic(RCCType *type, bool value);

void RCC_member_new(RCCType *type, RCCMember *member);
void RCC_param_new(RCCType *type, const RCCType *param);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Utility Functions
 * \{ */

bool RCC_type_compatible(const RCCType *a, const RCCType *b);
/** The common type we are going to use for conversions! */
RCCType *RCC_type_common(const RCCType *a, const RCCType *b);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Builtin Types
 * \{ */

extern RCCType *Tp_Void;
extern RCCType *Tp_Bool;

extern RCCType *Tp_Char;
extern RCCType *Tp_Short;
extern RCCType *Tp_Int;
extern RCCType *Tp_Long;

extern RCCType *Tp_UChar;
extern RCCType *Tp_UShort;
extern RCCType *Tp_UInt;
extern RCCType *Tp_ULong;

extern RCCType *Tp_Float;
extern RCCType *Tp_Double;
extern RCCType *Tp_LDouble;

/** \} */

#ifdef __cplusplus
}
#endif

#endif // __RCC_TYPE_H__
