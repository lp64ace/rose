#ifndef RNA_TYPES_H
#define RNA_TYPES_H

#include "LIB_listbase.h"

struct ID;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PropertyRNA PropertyRNA;

/* -------------------------------------------------------------------- */
/** \name Pointer RNA
 * \{ */

typedef struct PointerRNA {
	struct ID *owner;
	struct StructRNA *type;
	void *data;
} PointerRNA;

extern const PointerRNA PointerRNA_NULL;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Public API
 * \{ */

typedef struct PathResolvedRNA {
	struct PointerRNA ptr;
	struct PropertyRNA *property;
	/**
	 * In case the #property is an array property this stores the index 
	 * of the accessed element.
	 * 
	 * \note -1 for not array access.
	 */
	int index;
} PathResolvedRNA;

typedef bool (*IteratorSkipFunc)(struct CollectionPropertyIterator *iter, void *data);

typedef struct ListBaseIterator {
	struct Link *link;
	int flag;
	IteratorSkipFunc skip;
} ListBaseIterator;

typedef struct CollectionPropertyIterator {
	struct PointerRNA parent;
	struct PointerRNA builtin_parent;
	struct PropertyRNA *property;

	union {
		ListBaseIterator listbase;
		void *custom;
	} internal;

	struct PointerRNA ptr;
	
	unsigned int level : 7;
	unsigned int valid : 1;
} CollectionPropertyIterator;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Property Type RNA Enumerators
 * \{ */

typedef enum eRawPropertyType {
	PROP_RAW_UNSET = -1,
	PROP_RAW_INT,
	PROP_RAW_SHORT,
	PROP_RAW_CHAR,
	PROP_RAW_BOOLEAN,
	PROP_RAW_DOUBLE,
	PROP_RAW_FLOAT,
	PROP_RAW_UINT8,
	PROP_RAW_UINT16,
	PROP_RAW_INT64,
	PROP_RAW_UINT64,
	PROP_RAW_INT8,
} eRawPropertyType;

typedef enum ePropertyType {
	PROP_BOOLEAN = 0,
	PROP_INT,
	PROP_FLOAT,
	PROP_STRING,
	PROP_ENUM,
	PROP_POINTER,
	PROP_COLLECTION,
} ePropertyType;

enum {
	PROP_UNIT_NONE = (0 << 16),
	PROP_UNIT_LENGTH = (1 << 16),		 // m
	PROP_UNIT_ROTATION = (2 << 16),		 // radians
	PROP_UNIT_VELOCITY = (3 << 16),		 // m/s
	PROP_UNIT_ACCELERATION = (4 << 16),	 // m/(s^2)
};

#define RNA_SUBTYPE_UNIT(subtype) ((subtype) & 0x00FF0000)
#define RNA_SUBTYPE_VALUE(subtype) ((subtype) & ~0x00FF0000)
#define RNA_SUBTYPE_UNIT_VALUE(subtype) ((subtype) >> 16)

typedef enum ePropertySubType {
	PROP_NONE = 0,

	PROP_UNSIGNED = 1,
	PROP_PIXEL = 2,
	PROP_DISTANCE = 3 | PROP_UNIT_LENGTH,
	PROP_FACTOR = 4,
	PROP_COLOR = 5,
	PROP_TRANSLATION = 6 | PROP_UNIT_LENGTH,
	PROP_EULER = 7 | PROP_UNIT_ROTATION,
	PROP_AXISANGLE = 8,
	PROP_XYZ = 9,
	PROP_XYZ_LENGTH = 10 | PROP_UNIT_LENGTH,
	PROP_COORDS = 11,
} ePropertySubType;

typedef enum ePropertyFlag {
	/**
	 * Animatable means the property can be driven by some
	 * other input, be it animation curves, expressions, ..
	 * properties are animatable by default except for pointers
	 * and collections.
	 */
	PROP_ANIMATABLE = (1 << 0),
	/**
	 * Editable means the property is editable in the user interface, 
	 * properties are editable by default except for pointers and collectins.
	 */
	PROP_EDITABLE = (1 << 1),
	/**
	 * Use for...
	 *  - pointers: in the UI so unsetting for setting to None won't work.
	 *  - strings: so out internal generated get/length/set functions know to do NULL checks before.
	 */
	PROP_NEVER_NULL = (1 << 2),
	/**
	 * Pointers to data that is not owned by the struct.
	 * Typical example: Bone.parent, Bone.child, etc., and rearly all ID pointers.
	 */
	PROP_PTR_NO_OWNERSHIP = (1 << 3),
	/**
	 * This is an IDProperty, not a DNA one.
	 */
	PROP_IDPROPERTY = (1 << 4),
	/**
	 * Mark this property as handling ID user count.
	 *
	 * This is done automatically by the auto-generated setter function. If an RNA property has a
	 * custom setter, it's the setter's responsibility to correctly update the user count.
	 *
	 * \note In most basic cases, makesrna will automatically set this flag, based on the
	 * `STRUCT_ID_REFCOUNT` flag of the defined pointer type. This only works if makesrna can find a
	 * matching DNA property though, 'virtual' RNA properties (using both a getter and setter) will
	 * never get this flag defined automatically.
	 */
	PROP_ID_REFCOUNT = (1 << 5),
} ePropertyFlag;

ENUM_OPERATORS(ePropertyFlag, PROP_PTR_NO_OWNERSHIP);

typedef enum ePropertyInternalFlag {
	PROP_INTERN_RUNTIME = (1 << 0),
	PROP_INTERN_FREE_POINTERS = (1 << 1),
	PROP_INTERN_RAW_ACCESS = (1 << 2),
	PROP_INTERN_PTR_OWNERSHIP_FORCED = (1 << 3),
	PROP_INTERN_BUILTIN = (1 << 4),
} ePropertyInternalFlag;

ENUM_OPERATORS(ePropertyInternalFlag, PROP_INTERN_FREE_POINTERS);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Struct RNA Enumerators
 * \{ */

typedef enum eStructFlag {
	/** Indicates that this struct is an ID struct. */
	STRUCT_ID = (1 << 0),
	/**
	 * Indicates that this ID type's usages should typically be refcounted (i.e. makesrna will
	 * automatically set `PROP_ID_REFCOUNT` to PointerRNA properties that have this RNA type
	 * assigned).
	 */
	STRUCT_ID_REFCOUNT = (1 << 1),
	/** defaults on, indicates when changes in members of a StructRNA should trigger undo steps. */
	STRUCT_UNDO = (1 << 2),

	STRUCT_PUBLIC_NAMESPACE = (1 << 8),
	STRUCT_PUBLIC_NAMESPACE_INHERIT = (1 << 9),
	STRUCT_RUNTIME = (1 << 10),
	STRUCT_FREE_POINTERS = (1 << 11),
} eStructFlag;

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
	struct GHash *srnahash;
	/** Needed because types with an empty identifier aren't included in `srnahash`. */
	unsigned int totsrna;
} RoseRNA;

#define CONTAINER_RNA_ID(container) (*(const char **)(((ContainerRNA *)(container)) + 1))

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RNA_TYPES_H
