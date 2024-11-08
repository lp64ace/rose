#ifndef KER_IDPROP_H
#define KER_IDPROP_H

#include "LIB_listbase.h"

#include "KER_lib_id.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ID;
struct IDPropertyData;
struct IDProperty;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct IDPropertyData {
	union {
		void *pointer;
		/**
		 * In 32 bit systems a pointer is only 4 bytes, double values though take 8 bytes
		 * for that reason we store them in two 4 byte integers!
		 *
		 * Simply use `*((double *)(&data->value1)) = dbl;`
		 */
		struct {
			int value1;
			int value2;
		};
		/**
		 * This is only used for group although this right now is the 'longest' strcucture
		 * in this union we need it!
		 */
		ListBase group;
	};
} IDPropertyData;

typedef struct IDProperty {
	struct IDProperty *prev, *next;

	int type;
	int subtype;
	int flag;

	char name[64];

	IDPropertyData data;

	/** The length of the array and importantly `LIB_strlen(string) + 1` in case of string. */
	size_t length;
	/** The allocated length of the buffer of the array or string, in elements. */
	size_t alloc;
} IDProperty;

enum eIDPropertyType {
	IDP_STRING = 0,
	IDP_INT = 1,
	IDP_FLOAT = 2,
	/** Array containing integers, floats, doubles. */
	IDP_ARRAY = 3,
	IDP_ID = 4,
	IDP_DOUBLE = 5,
	IDP_IDPARRAY = 6,
	/** True or false value, backed by a `char` underlying type for arrays. */
	IDP_BOOLEAN = 7,
	IDP_GROUP = 8,
};

/** Used by some IDP utils, keep values in sync with type enum above. */
enum {
	IDP_TYPE_FILTER_STRING = 1 << IDP_STRING,
	IDP_TYPE_FILTER_INT = 1 << IDP_INT,
	IDP_TYPE_FILTER_FLOAT = 1 << IDP_FLOAT,
	IDP_TYPE_FILTER_ARRAY = 1 << IDP_ARRAY,
	IDP_TYPE_FILTER_GROUP = 1 << IDP_GROUP,
	IDP_TYPE_FILTER_ID = 1 << IDP_ID,
	IDP_TYPE_FILTER_DOUBLE = 1 << IDP_DOUBLE,
	IDP_TYPE_FILTER_IDPARRAY = 1 << IDP_IDPARRAY,
	IDP_TYPE_FILTER_BOOLEAN = 1 << IDP_BOOLEAN,
};

#define IDP_NUMTYPES 9

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Functions
 * \{ */

IDProperty *IDP_GetProperties(ID *id);
IDProperty *IDP_EnsureProperties(ID *id);

IDProperty *IDP_DuplicateProperty(const IDProperty *property);
IDProperty *IDP_DuplicateProperty_ex(const IDProperty *property, int flag);

void IDP_CopyProperty(IDProperty *destination, const IDProperty *source);

bool IDP_EqualsProperties_ex(const IDProperty *left, const IDProperty *right, bool strict);
bool IDP_EqualsProperties(const IDProperty *left, const IDProperty *right);

/**
 * Allocate and initialize a new IDProperty.
 *
 * This function creates a new property of the specified type, initializes it with the given data,
 * and assigns a name and flags to the property. The IDProperty must then be attached to an ID by
 * the user.
 *
 * \param type The type of the property to create, based on #eIDPropertyType:
 *   - `IDP_STRING`: Expects `data` to be a pointer to a null-terminated string.
 *   - `IDP_BOOLEAN`: Expects `data` to be a pointer to a boolean value (`bool` or `char`).
 *   - `IDP_INT`: Expects `data` to be a pointer to an integer value (`int`).
 *   - `IDP_FLOAT`: Expects `data` to be a pointer to a floating-point value (`float`).
 *   - `IDP_DOUBLE`: Expects `data` to be a pointer to a double-precision floating-point value (`double`).
 *   - `IDP_ARRAY`: Expects `data` to be the type of the elements in the array specified via #eIDPropertyType.
 *   - `IDP_ID`: Expects `data` to be a pointer to the `ID` structure.
 *   - `IDP_IDPARRAY`: This should always be NULL.
 *   - `IDP_GROUP`: This should always be NULL.
 * \param data A pointer to the data to initialize the property with, dependent on the property type.
 * \param length The size or length of the data, valid for #IDP_STRING, #IDP_ARRAY and #IDP_IDARRAY.
 * \param name A null-terminated string specifying the name of the property. The name will be copied internally.
 * \param flag Additional property-specific flags.
 *
 * \return A pointer to the newly allocated `IDProperty` structure, or NULL if allocation fails.
 */
IDProperty *IDP_New(int type, const void *data, size_t length, const char *name, int flag);

void IDP_FreePropertyContent_ex(IDProperty *property, bool do_user);
void IDP_FreePropertyContent(IDProperty *property);

void IDP_FreeProperty_ex(IDProperty *property, bool do_user);
void IDP_FreeProperty(IDProperty *property);

void IDP_ClearProperty(IDProperty *property);
void IDP_Reset(IDProperty *property, const IDProperty *reference);

#define IDP_Int(property) ((property)->data.value1)
#define IDP_Bool(property) ((property)->data.value1)
#define IDP_Array(property) ((property)->data.pointer)
/* C11 const correctness for casts. */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#	define IDP_Float(property)                                 \
		_Generic((property),                                    \
			IDProperty *: (*(float *)&(property)->data.value1), \
			const IDProperty *: (*(const float *)&(property)->data.value1))
#	define IDP_Double(property)                                 \
		_Generic((property),                                     \
			IDProperty *: (*(double *)&(property)->data.value1), \
			const IDProperty *: (*(const double *)&(property)->data.value1))
#	define IDP_String(property)                              \
		_Generic((property),                                  \
			IDProperty *: ((char *)(property)->data.pointer), \
			const IDProperty *: ((const char *)(property)->data.pointer))
#	define IDP_IDPArray(property)                                  \
		_Generic((property),                                        \
			IDProperty *: ((IDProperty *)(property)->data.pointer), \
			const IDProperty *: ((const IDProperty *)(property)->data.pointer))
#	define IDP_Id(property)                                \
		_Generic((property),                                \
			IDProperty *: ((ID *)(property)->data.pointer), \
			const IDProperty *: ((const ID *)(property)->data.pointer))
#else
#	define IDP_Float(property) (*(float *)&(property)->data.value1)
#	define IDP_Double(property) (*(double *)&(property)->data.value1)
#	define IDP_String(property) ((char *)(property)->data.pointer)
#	define IDP_IDPArray(property) ((IDProperty *)(property)->data.pointer)
#	define IDP_Id(property) ((ID *)(property)->data.pointer)
#endif

typedef void (*LibraryIDPropertyCallback)(struct IDProperty *, void *user_data);

void IDP_foreach_property(struct IDProperty *property, int type_filter, LibraryIDPropertyCallback callback, void *user_data);

/** \} */

/* -------------------------------------------------------------------- */
/** \name IDP_STRING
 * \{ */

IDProperty *IDP_NewStringMaxSize(const char *string, size_t maxncpy, const char *name, int flag);
IDProperty *IDP_NewString(const char *string, const char *name, int flag);

void IDP_AssignStringMaxSize(IDProperty *property, const char *string, size_t maxncpy);
void IDP_AssignString(IDProperty *property, const char *string);

#define IDP_ResizeString IDP_ResizeArray
#define IDP_FreeString IDP_FreeArray

/** \} */

/* -------------------------------------------------------------------- */
/** \name IDP_INT
 * \{ */

/** \} */

/* -------------------------------------------------------------------- */
/** \name IDP_FLOAT
 * \{ */

/** \} */

/* -------------------------------------------------------------------- */
/** \name IDP_ARRAY
 * \{ */

void IDP_ResizeArray(IDProperty *property, size_t length);
void IDP_FreeArray(IDProperty *property);

/** \} */

/* -------------------------------------------------------------------- */
/** \name IDP_ID
 * \{ */

void IDP_AssignID(IDProperty *property, ID *id, int flag);

/** \} */

/* -------------------------------------------------------------------- */
/** \name IDP_DOUBLE
 * \{ */

/** \} */

/* -------------------------------------------------------------------- */
/** \name IDP_IDPARRAY
 * \{ */

IDProperty *IDP_NewIDPArray(const char *name);
IDProperty *IDP_DuplicateIDPArray(const IDProperty *array, int flag);

/** We only shallow copy the item, the \p item will have to be freed by the user! */
void IDP_SetIndexArray(IDProperty *property, size_t index, IDProperty *item);

IDProperty *IDP_GetIndexArray(IDProperty *property, size_t index);

/** We only shallow copy the item, the \p item will have to be freed by the user! */
void IDP_AppendArray(IDProperty *property, IDProperty *item);

void IDP_ResizeIDPArray(IDProperty *property, size_t length);

/** \} */

/* -------------------------------------------------------------------- */
/** \name IDP_BOOLEAN
 * \{ */

/** \} */

/* -------------------------------------------------------------------- */
/** \name IDP_GROUP
 * \{ */

/**
 * Sync values from one group to another when values name and types match,
 * copy the values, else ignore.
 *
 * \note Was used for syncing proxies.
 */
void IDP_SyncGroupValues(IDProperty *destination, const IDProperty *source);
void IDP_SyncGroupTypes(IDProperty *destination, const IDProperty *source, bool do_arraylen);
/**
 * Replaces all properties with the same name in a destination group from a source group.
 */
void IDP_ReplaceGroupInGroup(IDProperty *destination, const IDProperty *source);
void IDP_ReplaceInGroup(IDProperty *group, IDProperty *property);
/**
 * Checks if a property with the same name as \p exist exists in the group, and if so replaces it.
 * Use this to preserve order!
 */
void IDP_ReplaceInGroup_ex(IDProperty *group, IDProperty *property, IDProperty *exist);
/**
 * If a property is missing in \a dest, add it.
 * Do it recursively.
 */
void IDP_MergeGroup(IDProperty *destination, const IDProperty *source, bool overwrite);
/**
 * If a property is missing in \a dest, add it.
 * Do it recursively.
 */
void IDP_MergeGroup_ex(IDProperty *destination, const IDProperty *source, bool overwrite, int flag);
/**
 * This function has a sanity check to make sure ID properties with the same name don't
 * get added to the group.
 *
 * The sanity check just means the property is not added to the group if another property
 * exists with the same name; the client code using ID properties then needs to detect this
 * (the function that adds new properties to groups, #IDP_AddToGroup,
 * returns false if a property can't be added to the group, and true if it can)
 * and free the property.
 */
bool IDP_AddToGroup(IDProperty *group, IDProperty *property);
/**
 * This is the same as IDP_AddToGroup, but you can specify the position.
 */
bool IDP_InsertToGroup(IDProperty *group, IDProperty *previous, IDProperty *property);
/**
 * \note this does not free the property!
 *
 * To free the property, you have to do: #IDP_FreeProperty(prop);
 */
void IDP_RemoveFromGroup(IDProperty *group, IDProperty *property);
void IDP_FreeFromGroup(IDProperty *group, IDProperty *property);

IDProperty *IDP_GetPropertyFromGroup(const IDProperty *property, const char *name);
IDProperty *IDP_GetPropertyTypeFromGroup(const IDProperty *property, const char *name, int type);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_IDPROP_H
