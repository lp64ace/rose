#ifndef RNA_INTERNAL_TYPES_H
#define RNA_INTERNAL_TYPES_H

#include "LIB_ghash.h"
#include "LIB_listbase.h"

#include "RNA_define.h"
#include "RNA_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct StructRNA;

/* -------------------------------------------------------------------- */
/** \name Property RNA Data Structures
 * \{ */

typedef struct PropertyRNA {
	struct PropertyRNA *prev, *next;

	int magic;

	/** Unique identifier for the property, not exposed to the user. */
	const char *identifier;

	ePropertyFlag flag;
	ePropertyType type;
	ePropertySubType subtype;

	/** User readable property name for usage in the UI. */
	const char *name;
	/** User readable property description for usage in the UI. */
	const char *description;

	/** Dimension of array. */
	unsigned int arraydimension;
	/** Array lengths for all dimensions (when `arraydimension > 0`). */
	unsigned int arraylength[RNA_MAX_ARRAY_DIMENSION];
	unsigned int totarraylength;

	ePropertyInternalFlag flagex;

	/* Raw access. */

	int rawoffset;
	eRawPropertyType rawtype;

	/**
	 * Attributes attached directly to this collection.
	 */
	struct StructRNA *srna;
} PropertyRNA;

/** \} */

struct PointerRNA;

/* -------------------------------------------------------------------- */
/** \name Boolean Property RNA Data Structures
 * \{ */

typedef bool (*PropBooleanGetFunc)(struct PointerRNA *ptr);
typedef void (*PropBooleanSetFunc)(struct PointerRNA *ptr, bool value);
typedef void (*PropBooleanArrayGetFunc)(struct PointerRNA *ptr, bool *values);
typedef void (*PropBooleanArraySetFunc)(struct PointerRNA *ptr, const bool *values);

typedef bool (*PropBooleanGetFuncEx)(struct PointerRNA *ptr, struct PropertyRNA *prop);
typedef void (*PropBooleanSetFuncEx)(struct PointerRNA *ptr, struct PropertyRNA *prop, bool value);
typedef void (*PropBooleanArrayGetFuncEx)(struct PointerRNA *ptr, struct PropertyRNA *prop, bool *values);
typedef void (*PropBooleanArraySetFuncEx)(struct PointerRNA *ptr, struct PropertyRNA *prop, const bool *values);

typedef struct BoolPropertyRNA {
	PropertyRNA property;

	PropBooleanGetFunc get;
	PropBooleanSetFunc set;
	PropBooleanArrayGetFunc getarray;
	PropBooleanArraySetFunc setarray;

	PropBooleanGetFuncEx get_ex;
	PropBooleanSetFuncEx set_ex;
	PropBooleanArrayGetFuncEx getarray_ex;
	PropBooleanArraySetFuncEx setarray_ex;

	PropBooleanGetFuncEx get_default;
	PropBooleanArrayGetFuncEx get_default_array;
	bool defaultvalue;
	const bool *defaultarray;
} BoolPropertyRNA;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Int Property RNA Data Structures
 * \{ */

typedef int (*PropIntGetFunc)(struct PointerRNA *ptr);
typedef void (*PropIntSetFunc)(struct PointerRNA *ptr, int value);
typedef void (*PropIntArrayGetFunc)(struct PointerRNA *ptr, int *values);
typedef void (*PropIntArraySetFunc)(struct PointerRNA *ptr, const int *values);
typedef void (*PropIntRangeFunc)(struct PointerRNA *ptr, int *min, int *max, int *softmin, int *softmax);

typedef int (*PropIntGetFuncEx)(struct PointerRNA *ptr, struct PropertyRNA *prop);
typedef void (*PropIntSetFuncEx)(struct PointerRNA *ptr, struct PropertyRNA *prop, int value);
typedef void (*PropIntArrayGetFuncEx)(struct PointerRNA *ptr, struct PropertyRNA *prop, int *values);
typedef void (*PropIntArraySetFuncEx)(struct PointerRNA *ptr, struct PropertyRNA *prop, const int *values);
typedef void (*PropIntRangeFuncEx)(struct PointerRNA *ptr, struct PropertyRNA *prop, int *min, int *max, int *softmin, int *softmax);

typedef struct IntPropertyRNA {
	PropertyRNA property;

	PropIntGetFunc get;
	PropIntSetFunc set;
	PropIntArrayGetFunc getarray;
	PropIntArraySetFunc setarray;
	PropIntRangeFunc range;

	PropIntGetFuncEx get_ex;
	PropIntSetFuncEx set_ex;
	PropIntArrayGetFunc getarray_ex;
	PropIntArraySetFunc setarray_ex;
	PropIntRangeFuncEx range_ex;

	int softmin, softmax;
	int hardmin, hardmax;
	int step;

	PropIntGetFuncEx get_default;
	PropIntArrayGetFunc get_default_array;
	int defaultvalue;
	const int *defaultarray;
} IntPropertyRNA;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Float Property RNA Data Structures
 * \{ */

typedef float (*PropFloatGetFunc)(struct PointerRNA *ptr);
typedef void (*PropFloatSetFunc)(struct PointerRNA *ptr, float value);
typedef void (*PropFloatArrayGetFunc)(struct PointerRNA *ptr, float *values);
typedef void (*PropFloatArraySetFunc)(struct PointerRNA *ptr, const float *values);
typedef void (*PropFloatRangeFunc)(struct PointerRNA *ptr, float *min, float *max, float *softmin, float *softmax);

typedef float (*PropFloatGetFuncEx)(struct PointerRNA *ptr, struct PropertyRNA *prop);
typedef void (*PropFloatSetFuncEx)(struct PointerRNA *ptr, struct PropertyRNA *prop, float value);
typedef void (*PropFloatArrayGetFuncEx)(struct PointerRNA *ptr, struct PropertyRNA *prop, float *values);
typedef void (*PropFloatArraySetFuncEx)(struct PointerRNA *ptr, struct PropertyRNA *prop, const float *values);
typedef void (*PropFloatRangeFuncEx)(struct PointerRNA *ptr, struct PropertyRNA *prop, float *min, float *max, float *softmin, float *softmax);

typedef struct FloatPropertyRNA {
	PropertyRNA property;

	PropFloatGetFunc get;
	PropFloatGetFunc set;
	PropFloatArrayGetFunc getarray;
	PropFloatArraySetFunc setarray;
	PropFloatRangeFunc range;

	PropFloatGetFuncEx get_ex;
	PropFloatSetFuncEx set_ex;
	PropFloatArrayGetFuncEx getarray_ex;
	PropFloatArraySetFuncEx setarray_ex;
	PropFloatRangeFuncEx range_ex;

	float softmin, softmax;
	float hardmin, hardmax;
	float step;
	int precision;

	PropFloatGetFuncEx get_default;
	PropFloatArrayGetFuncEx get_default_array;
	float defaultvalue;
	const float *defaultarray;
} FloatPropertyRNA;

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Property RNA Data Structures
 * \{ */

typedef void (*PropStringGetFunc)(struct PointerRNA *ptr, char *value);
typedef int (*PropStringLengthFunc)(struct PointerRNA *ptr);
typedef void (*PropStringSetFunc)(struct PointerRNA *ptr, const char *value);

typedef void (*PropStringGetFuncEx)(struct PointerRNA *ptr, struct PropertyRNA *prop, char *value);
typedef int (*PropStringLengthFuncEx)(struct PointerRNA *ptr, struct PropertyRNA *prop);
typedef void (*PropStringSetFuncEx)(struct PointerRNA *ptr, struct PropertyRNA *prop, const char *value);

typedef struct StringPropertyRNA {
	PropertyRNA property;

	PropStringGetFunc get;
	PropStringLengthFunc length;
	PropStringSetFunc set;

	PropStringGetFuncEx get_ex;
	PropStringLengthFuncEx length_ex;
	PropStringSetFuncEx set_ex;

	/** Maximum length including the string terminator! */
	int maxlength;

	const char *defaultvalue;
} StringPropertyRNA;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Pointer Property RNA Data Structures
 * \{ */

typedef struct PointerRNA (*PropPointerGetFunc)(struct PointerRNA *ptr);
typedef struct StructRNA *(*PropPointerTypeFunc)(struct PointerRNA *ptr);
typedef void (*PropPointerSetFunc)(struct PointerRNA *ptr, const struct PointerRNA value);
typedef bool (*PropPointerPollFunc)(struct PointerRNA *ptr, const struct PointerRNA value);

typedef struct PointerPropertyRNA {
	PropertyRNA property;

	PropPointerGetFunc get;
	PropPointerSetFunc set;
	PropPointerTypeFunc type;
	PropPointerPollFunc poll;

	struct StructRNA *srna;
} PointerPropertyRNA;

/** \} */

typedef struct ContainerRNA {
	void *prev, *next;

	ListBase properties;
} ContainerRNA;

/* -------------------------------------------------------------------- */
/** \name Struct RNA Data Structures
 * \{ */

typedef struct StructRNA {
	ContainerRNA container;

	/** Unique identifier for the property, not exposed to the user. */
	const char *identifier;

	eStructFlag flag;

	/** User readable property name for usage in the UI. */
	const char *name;
	/** User readable property description for usage in the UI. */
	const char *description;

	struct PropertyRNA *nameproperty;
	struct PropertyRNA *iteratorproperty;

	/** Struct this is derived from. */
	struct StructRNA *base;

	/**
	 * Only use for nested structs, where both the parent and child access
	 * the same C Struct but nesting is used for grouping properties.
	 * The parent property is used so we know NULL checks are not needed,
	 * and that this struct will never exist without its parent.
	 */
	struct StructRNA *nested;
} StructRNA;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Rose RNA Data Structures
 * \{ */

typedef struct RoseRNA {
	ListBase srnabase;
	/**
	 * A map of structs: `{StructRNA.identifier -> StructRNA}`
	 * These are ensured to have unique names (with #STRUCT_PUBLIC_NAMESPACE enabled).
	 */
	GHash *srnahash;
	/** Needed because types with an empty identifier aren't included in `srnahash`. */
	unsigned int totsrna;
} RoseRNA;

#define CONTAINER_RNA_ID(container) (*(const char **)(((ContainerRNA *)(container)) + 1))

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RNA_INTERNAL_TYPES_H
