#include "MEM_guardedalloc.h"

#include "LIB_endian_switch.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "KER_idtype.h"
#include "KER_idprop.h"

#include <math.h>

#ifndef NDEBUG
#	include <stdio.h>
#endif

#include "RLO_read_write.h"

#define DEFAULT_ALLOC_FOR_NULL_STRINGS 64
#define IDP_ARRAY_REALLOC_LIMIT 200

/* -------------------------------------------------------------------- */
/** \name Utilities
 * \{ */

static size_t idp_size_table[] = {
	[IDP_BOOLEAN] = sizeof(char),
	[IDP_STRING] = sizeof(char),
	[IDP_INT] = sizeof(int),
	[IDP_FLOAT] = sizeof(float),
	[IDP_DOUBLE] = sizeof(double),
	[IDP_ID] = sizeof(void *),

	[IDP_GROUP] = sizeof(ListBase),

	[IDP_ARRAY] = 0,
	[IDP_IDPARRAY] = 0,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Functions
 * \{ */

IDProperty *IDP_GetProperties(ID *id) {
	return id->properties;
}

IDProperty *IDP_EnsureProperties(ID *id) {
	if (id->properties == NULL) {
		id->properties = MEM_callocN(sizeof(IDProperty), "IDProperty");
		id->properties->type = IDP_GROUP;
	}
	return id->properties;
}

IDProperty *IDP_DuplicateProperty(const IDProperty *property) {
	return IDP_DuplicateProperty_ex(property, 0);
}

/** Forward declaration, this function is not exported to the user! */
IDProperty *IDP_DuplicateGeneric(const IDProperty *property, const int flag);
/** Forward declaration, this function is not exported to the user! */
IDProperty *IDP_DuplicateArray(const IDProperty *property, const int flag);
/** Forward declaration, this function is not exported to the user! */
IDProperty *IDP_DuplicateGroup(const IDProperty *property, const int flag);
/** Forward declaration, this function is not exported to the user! */
IDProperty *IDP_DuplicateString(const IDProperty *property, const int flag);
/** Forward declaration, this function is not exported to the user! */
IDProperty *IDP_DuplicateID(const IDProperty *property, const int flag);

IDProperty *IDP_DuplicateProperty_ex(const IDProperty *property, int flag) {
	switch (property->type) {
		case IDP_GROUP:
			return IDP_DuplicateGroup(property, flag);
		case IDP_STRING:
			return IDP_DuplicateString(property, flag);
		case IDP_ID:
			return IDP_DuplicateID(property, flag);
		case IDP_ARRAY:
			return IDP_DuplicateArray(property, flag);
		case IDP_IDPARRAY:
			return IDP_DuplicateIDPArray(property, flag);
	}
	return IDP_DuplicateGeneric(property, flag);
}

ROSE_INLINE void IDP_CopyProperty_ex(IDProperty *destination, const IDProperty *source, int flag) {
	IDProperty *newp = IDP_DuplicateProperty_ex(source, flag);
	newp->prev = destination->prev;
	newp->next = destination->next;
	memswap(destination, newp, sizeof(IDProperty));
	/** We now free the duplicate "pointer" with the contents of the old property! */
	IDP_FreeProperty(newp);
}

void IDP_CopyProperty(IDProperty *destination, const IDProperty *source) {
	IDP_CopyProperty_ex(destination, source, 0);
}

bool IDP_EqualsProperties_ex(const IDProperty *left, const IDProperty *right, bool strict) {
	if (left == NULL && right == NULL) {
		return true;
	}
	if (left == NULL || right == NULL) {
		return !strict;
	}
	if (left->type != right->type) {
		return false;
	}

	switch (left->type) {
		case IDP_INT: {
			return (IDP_Int(left) == IDP_Int(right));
		} break;
		case IDP_FLOAT: {
#ifndef NDEBUG
			float p1 = IDP_Float(left);
			float p2 = IDP_Float(right);
			if ((p1 != p2) && (fabsf(p1 - p2) < 1e-7f)) {
				printf("WARNING: Comparing float properties that have nearly the same value!\n");
			}
#endif
			return (IDP_Float(left) == IDP_Float(right));
		} break;
		case IDP_DOUBLE: {
			return (IDP_Double(left) == IDP_Double(right));
		} break;
		case IDP_STRING: {
			return (left->length == right->length) && STREQLEN(IDP_String(left), IDP_String(right), left->length);
		} break;
		case IDP_ARRAY: {
			if (left->length == right->length && left->subtype == right->subtype) {
				size_t element = idp_size_table[left->subtype];
				return memcmp(IDP_Array(left), IDP_Array(right), element * left->length) == 0;
			}
			return false;
		} break;
		case IDP_GROUP: {
			if (strict && left->length != right->length) {
				return false;
			}

			LISTBASE_FOREACH(const IDProperty *, link1, &left->data.group) {
				const IDProperty *link2 = IDP_GetPropertyFromGroup(right, link1->name);

				if (!IDP_EqualsProperties_ex(link1, link2, strict)) {
					return false;
				}
			}

			return true;
		} break;
		case IDP_IDPARRAY: {
			const IDProperty *array1 = IDP_IDPArray(left);
			const IDProperty *array2 = IDP_IDPArray(right);

			if (left->length != right->length) {
				return false;
			}

			for (int index = 0; index < left->length; index++) {
				if (!IDP_EqualsProperties_ex(&array1[index], &array2[index], strict)) {
					return false;
				}
			}

			return true;
		} break;
		case IDP_ID: {
			return IDP_Id(left) == IDP_Id(right);
		} break;
		default: {
			ROSE_assert_unreachable();
		} break;
	}

	return !strict;
}

bool IDP_EqualsProperties(const IDProperty *left, const IDProperty *right) {
	return IDP_EqualsProperties_ex(left, right, true);
}

IDProperty *IDP_New(int type, const void *data, size_t length, const char *name, int flag) {
	IDProperty *property = NULL;

	switch (type) {
		case IDP_INT: {
			/** The length is not used, or can be set to one (but will be ignored)! */
			ROSE_assert(ELEM(length, (size_t)0, (size_t)1));
			property = MEM_callocN(sizeof(IDProperty), "IDProperty");
			IDP_Int(property) = *(const int *)data;
		} break;
		case IDP_FLOAT: {
			/** The length is not used, or can be set to one (but will be ignored)! */
			ROSE_assert(ELEM(length, (size_t)0, (size_t)1));
			property = MEM_callocN(sizeof(IDProperty), "IDProperty");
			IDP_Float(property) = *(const float *)data;
		} break;
		case IDP_DOUBLE: {
			/** The length is not used, or can be set to one (but will be ignored)! */
			ROSE_assert(ELEM(length, (size_t)0, (size_t)1));
			property = MEM_callocN(sizeof(IDProperty), "IDProperty");
			IDP_Double(property) = *(const double *)data;
		} break;
		case IDP_BOOLEAN: {
			/** The length is not used, or can be set to one (but will be ignored)! */
			ROSE_assert(ELEM(length, (size_t)0, (size_t)1));
			property = MEM_callocN(sizeof(IDProperty), "IDProperty");
			IDP_Bool(property) = *(const bool *)data;
		} break;
		case IDP_ARRAY: {
			int subtype = (int)(intptr_t)data;	// This silences the warning that we are casting a pointer to an 'int'.

			if (ELEM(subtype, IDP_FLOAT, IDP_INT, IDP_DOUBLE, IDP_GROUP, IDP_BOOLEAN)) {
				property = MEM_callocN(sizeof(IDProperty), "IDProperty(Array)");
				property->subtype = subtype;
				if (length) {
					size_t element = idp_size_table[property->subtype];
					property->data.pointer = MEM_mallocN(element * length, "IDProperty::ArrayData");
				}
				property->length = property->alloc = length;
			}
			return NULL;
		} break;
		case IDP_STRING: {
			property = MEM_callocN(sizeof(IDProperty), "IDProperty(String)");
			if (data == NULL || length <= 1) {
				property->data.pointer = MEM_mallocN(DEFAULT_ALLOC_FOR_NULL_STRINGS, "IDProperty::StringData(Default)");
				IDP_String(property)[0] = '\0';
				property->alloc = DEFAULT_ALLOC_FOR_NULL_STRINGS;
				property->length = 1;
			}
			else {
				ROSE_assert(length <= LIB_strlen((const char *)data) + 1);
				property->data.pointer = MEM_mallocN(length, "IDProperty::StringData");
				memcpy(property->data.pointer, (const char *)data, length - 1);
				IDP_String(property)[length - 1] = '\0';
				property->length = property->alloc = length;
			}
		} break;
		case IDP_GROUP: {
			ROSE_assert(data == NULL);
			property = MEM_callocN(sizeof(IDProperty), "IDProperty(Group)");
		} break;
		case IDP_ID: {
			property = MEM_callocN(sizeof(IDProperty), "IDProperty(ID)");
			property->data.pointer = (ID *)data;
			id_us_add(IDP_Id(property));
		} break;
		case IDP_IDPARRAY: {
			ROSE_assert(data == NULL);
			property = MEM_callocN(sizeof(IDProperty), "IDProperty(IDPArray)");
		} break;
	}

	if (property) {
		property->type = type;
		LIB_strcpy(property->name, ARRAY_SIZE(property->name), name);
		property->flag = flag;
	}

	return property;
}

/** Forward declaration, this function is not exported to the user! */
void IDP_FreeIDPArray(IDProperty *property, const bool do_user);
/** Forward declaration, this function is not exported to the user! */
void IDP_FreeGroup(IDProperty *property, const bool do_user);

void IDP_FreePropertyContent_ex(IDProperty *property, bool do_user) {
	switch (property->type) {
		case IDP_ARRAY: {
			IDP_FreeArray(property);
		} break;
		case IDP_STRING: {
			IDP_FreeString(property);
		} break;
		case IDP_GROUP: {
			IDP_FreeGroup(property, do_user);
		} break;
		case IDP_IDPARRAY: {
			IDP_FreeIDPArray(property, do_user);
		} break;
		case IDP_ID: {
			if (do_user) {
				/** Will delete the ID if there are no more users! */
				id_us_rem(IDP_Id(property));
			}
		} break;
	}
}

void IDP_FreePropertyContent(IDProperty *property) {
	IDP_FreePropertyContent_ex(property, true);
}

void IDP_FreeProperty_ex(IDProperty *property, bool do_user) {
	IDP_FreePropertyContent_ex(property, do_user);
	MEM_freeN(property);
}

void IDP_FreeProperty(IDProperty *property) {
	IDP_FreePropertyContent(property);
	MEM_freeN(property);
}

void IDP_ClearProperty(IDProperty *property) {
	IDP_FreePropertyContent(property);
	memset(&property->data, 0, sizeof(IDPropertyData));
	property->length = 0;
	property->alloc = 0;
}

void IDP_Reset(IDProperty *property, const IDProperty *reference) {
	if (property) {
		IDP_ClearProperty(property);
		if (reference) {
			IDP_MergeGroup(property, reference, true);
		}
	}
}

void IDP_WriteProperty_OnlyData(const IDProperty *property, RoseWriter *writer);

ROSE_INLINE void IDP_WriteArray(const IDProperty *prop, RoseWriter *writer) {
	/* Remember to set #IDProperty.totallen to len in the linking code! */
	if (prop->data.pointer) {
		RLO_write_raw(writer, MEM_allocN_length(prop->data.pointer), prop->data.pointer);

    	if (prop->subtype == IDP_GROUP) {
			IDProperty **array = prop->data.pointer;

			for (int a = 0; a < prop->length; a++) {
				IDP_RoseWrite(writer, array[a]);
			}
		}
	}
}

static void IDP_WriteIDPArray(const IDProperty *prop, RoseWriter *writer) {
	/* Remember to set #IDProperty.totallen to len in the linking code! */
	if (prop->data.pointer) {
		const IDProperty *array = prop->data.pointer;

		RLO_write_struct_array(writer, IDProperty, prop->length, array);

		for (int a = 0; a < prop->length; a++) {
			IDP_WriteProperty_OnlyData(&array[a], writer);
		}
	}
}

static void IDP_WriteString(const IDProperty *prop, RoseWriter *writer) {
	/* Remember to set #IDProperty.totallen to len in the linking code! */
	RLO_write_raw(writer, (size_t)prop->length, prop->data.pointer);
}

static void IDP_WriteGroup(const IDProperty *prop, RoseWriter *writer) {
	LISTBASE_FOREACH (IDProperty *, loop, &prop->data.group) {
		IDP_RoseWrite(writer, loop);
	}
}

void IDP_WriteProperty_OnlyData(const IDProperty *property, RoseWriter *writer) {
	switch (property->type) {
		case IDP_GROUP:
			IDP_WriteGroup(property, writer);
			break;
		case IDP_STRING:
			IDP_WriteString(property, writer);
			break;
		case IDP_ARRAY:
			IDP_WriteArray(property, writer);
			break;
		case IDP_IDPARRAY:
			IDP_WriteIDPArray(property, writer);
			break;
	}
}

void IDP_RoseWrite(RoseWriter *writer, IDProperty *property) {
	RLO_write_struct(writer, IDProperty, property);
	IDP_WriteProperty_OnlyData(property, writer);
}

void IDP_DirectLinkProperty(IDProperty *prop, RoseDataReader *reader);
void IDP_LibLinkProperty(IDProperty *prop, RoseDataReader *reader);

void IDP_DirectLinkIDPArray(IDProperty *prop, RoseDataReader *reader) {
	IDProperty *array;
	int i;

	/* since we didn't save the extra buffer, set totallen to len */
	prop->alloc = prop->length;
	RLO_read_data_address(reader, &prop->data.pointer);

	array = (IDProperty *)prop->data.pointer;

	/* note!, idp-arrays didn't exist in 2.4x, so the pointer will be cleared
	* theres not really anything we can do to correct this, at least don't crash */
	if (array == NULL) {
		prop->length = 0;
		prop->alloc = 0;
	}

	for (i = 0; i < prop->length; i++) {
		IDP_DirectLinkProperty(&array[i], reader);
	}
}

void IDP_DirectLinkArray(IDProperty *prop, RoseDataReader *reader) {

	/* since we didn't save the extra buffer, set totallen to len */
	prop->alloc = prop->length;

	if (prop->subtype == IDP_GROUP) {
		RLO_read_pointer_array(reader, prop->length, &prop->data.pointer);
		IDProperty **array = (IDProperty **)prop->data.pointer;

		for (int i = 0; i < prop->length; i++) {
			IDP_DirectLinkProperty(array[i], reader);
		}
	}
	else if (prop->subtype == IDP_DOUBLE) {
		RLO_read_double_array(reader, prop->length, (double **)&prop->data.pointer);
	}
	else {
		RLO_read_int32_array(reader, prop->length, (int32_t **)&prop->data.pointer);
	}
}

void IDP_DirectLinkString(IDProperty *prop, RoseDataReader *reader) {
	/*since we didn't save the extra string buffer, set totallen to len.*/
	prop->length = prop->length;
	RLO_read_data_address(reader, &prop->data.pointer);
}

void IDP_DirectLinkGroup(IDProperty *prop, RoseDataReader *reader) {
	ListBase *lb = &prop->data.group;

	RLO_read_list(reader, lb);

	/*Link child id properties now*/
	LISTBASE_FOREACH(IDProperty *, loop, &prop->data.group) {
		IDP_DirectLinkProperty(loop, reader);
	}
}

void IDP_DirectLinkProperty(IDProperty *prop, RoseDataReader *reader) {
	switch (prop->type) {
		case IDP_GROUP:
			IDP_DirectLinkGroup(prop, reader);
			break;
		case IDP_STRING:
			IDP_DirectLinkString(prop, reader);
			break;
		case IDP_ARRAY:
			IDP_DirectLinkArray(prop, reader);
			break;
		case IDP_IDPARRAY:
			IDP_DirectLinkIDPArray(prop, reader);
			break;
		case IDP_DOUBLE:
			/* erg, stupid doubles.  since I'm storing them
			* in the same field as int val; val2 in the
			* IDPropertyData struct, they have to deal with
			* endianness specifically
			*
			* in theory, val and val2 would've already been swapped
			* if switch_endian is true, so we have to first unswap
			* them then reswap them as a single 64-bit entity.
			*/
			if (RLO_read_requires_endian_switch(reader)) {
				LIB_endian_switch_int32(&prop->data.value1);
				LIB_endian_switch_int32(&prop->data.value2);
				LIB_endian_switch_int64((int64_t *)&prop->data.value1);
			}
			break;
		case IDP_INT:
		case IDP_FLOAT:
		case IDP_ID:
			break; /* Nothing special to do here. */
		default:
			/* Unknown IDP type, nuke it (we cannot handle unknown types everywhere in code,
			* IDP are way too polymorphic to do it safely. */
			printf("%s: found unknown IDProperty type %d, reset to Integer one !\n", __func__, prop->type);
			/* Note: we do not attempt to free unknown prop, we have no way to know how to do that! */
			prop->type = IDP_INT;
			prop->subtype = 0;
			IDP_Int(prop) = 0;
	}
}

void IDP_RoseReadData(RoseDataReader *reader, IDProperty **property, const char *allocname) {
	if (*property) {
		if ((*property)->type == IDP_GROUP) {
			IDP_DirectLinkGroup(*property, reader);
		}
		else {
			/* corrupt file! */
			printf("%s: found non group data, freeing type %d!\n", allocname, (*property)->type);
			/* don't risk id, data's likely corrupt. */
			// IDP_FreePropertyContent(*prop);
			*property = NULL;
		}
	}
}

void IDP_RoseReadLib(RoseLibReader *reader, IDProperty *property) {
	if (!property) {
		return;
	}

	switch (property->type) {
		case IDP_ID: {
			void *newaddr = RLO_read_get_new_id_address(reader, NULL, IDP_Id(property));
#ifndef NDEBUG
			if (IDP_Id(property) && !newaddr) {
				fprintf(stderr, "[Kernel] Error while loading \"%s\". Data not found in file!\n", property->name);
			}
#endif
			property->data.pointer = newaddr;
		} break;
		case IDP_IDPARRAY: {
			IDProperty *idp_array = IDP_IDPArray(property);
			for (int i = 0; i < property->length; i++) {
				IDP_RoseReadLib(reader, &(idp_array[i]));
			}
		} break;
		case IDP_GROUP: {
			LISTBASE_FOREACH (IDProperty *, loop, &property->data.group) {
				IDP_RoseReadLib(reader, loop);
			}
		} break;
	}
}

void IDP_foreach_property(IDProperty *property, int type_filter, LibraryIDPropertyCallback callback, void *user_data) {
	if (!property) {
		return;
	}

	if (type_filter == 0 || (1 << property->type) & type_filter) {
		callback(property, user_data);
	}

	/* Recursive call into container types of ID properties. */
	switch (property->type) {
		case IDP_GROUP: {
			LISTBASE_FOREACH(IDProperty *, loop, &property->data.group) {
				IDP_foreach_property(loop, type_filter, callback, user_data);
			}
		} break;
		case IDP_IDPARRAY: {
			IDProperty *loop = (IDProperty *)(IDP_Array(property));
			for (int i = 0; i < property->length; i++) {
				IDP_foreach_property(&loop[i], type_filter, callback, user_data);
			}
		} break;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name IDP_STRING
 * \{ */

IDProperty *IDP_DuplicateString(const IDProperty *property, const int flag) {
	ROSE_assert(property->type == IDP_STRING);

	IDProperty *newp = IDP_DuplicateGeneric(property, flag);
	if (IDP_String(property)) {
		newp->data.pointer = MEM_dupallocN(IDP_String(property));
	}
	newp->length = property->length;
	newp->subtype = property->subtype;
	newp->alloc = property->alloc;

	return newp;
}

IDProperty *IDP_NewStringMaxSize(const char *string, size_t maxncpy, const char *name, int flag) {
	IDProperty *property = MEM_callocN(sizeof(IDProperty), "IDProperty(String)");

	if (string == NULL) {
		property->data.pointer = MEM_mallocN(DEFAULT_ALLOC_FOR_NULL_STRINGS, "IDProperty::StringData(Default)");
		IDP_String(property)[0] = '\0';
		property->alloc = DEFAULT_ALLOC_FOR_NULL_STRINGS;
		property->length = 1;
	}
	else {
		ROSE_assert(maxncpy <= LIB_strlen((const char *)string) + 1);
		property->data.pointer = MEM_mallocN(maxncpy, "IDProperty::StringData");
		memcpy(property->data.pointer, (const char *)string, maxncpy - 1);
		IDP_String(property)[maxncpy - 1] = '\0';
		property->length = property->alloc = maxncpy;
	}

	property->type = IDP_STRING;
	LIB_strcpy(property->name, ARRAY_SIZE(property->name), name);
	property->flag = flag;

	return property;
}

IDProperty *IDP_NewString(const char *string, const char *name, int flag) {
	size_t length = 0;
	if (string) {
		length = LIB_strlen(string) + 1;
	}
	return IDP_NewStringMaxSize(string, length, name, flag);
}

void IDP_AssignStringMaxSize(IDProperty *property, const char *string, size_t maxncpy) {
	ROSE_assert(property->type == IDP_STRING);

	IDP_ResizeString(property, maxncpy);
	if (maxncpy) {
		memcpy(IDP_String(property), string, maxncpy - 1);
		IDP_String(property)[maxncpy - 1] = '\0';
	}
}

void IDP_AssignString(IDProperty *property, const char *string) {
	ROSE_assert(property->type == IDP_STRING);

	size_t length = 0;
	if (string) {
		length = LIB_strlen(string) + 1;
	}
	IDP_AssignStringMaxSize(property, string, length);
}

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

IDProperty *IDP_DuplicateGeneric(const IDProperty *property, const int flag) {
	UNUSED_VARS(flag);

	IDProperty *newp = MEM_mallocN(sizeof(IDProperty), "IDProperty");

	newp->type = property->type;
	newp->flag = property->flag;
	if (ELEM(property->type, IDP_INT, IDP_FLOAT, IDP_DOUBLE, IDP_BOOLEAN)) {
		newp->data.value1 = property->data.value1;
		newp->data.value2 = property->data.value2;
	}
	LIB_strcpy(newp->name, ARRAY_SIZE(newp->name), property->name);

	return newp;
}

IDProperty *IDP_DuplicateArray(const IDProperty *property, const int flag) {
	IDProperty *newp = IDP_DuplicateGeneric(property, flag);

	if (IDP_Array(property)) {
		IDP_Array(newp) = MEM_dupallocN(IDP_Array(property));

		if (property->type == IDP_GROUP) {
			IDProperty **array = (IDProperty **)IDP_Array(newp);

			for (int index = 0; index < property->length; index++) {
				array[index] = IDP_DuplicateProperty_ex(array[index], flag);
			}
		}
	}
	newp->length = property->length;
	newp->subtype = property->subtype;
	newp->alloc = property->alloc;

	return newp;
}

void IDP_ResizeGroupArray(IDProperty *property, int length, void *data);

void IDP_ResizeArray(IDProperty *property, size_t length) {
	const bool grow = length > property->length;

	if (length <= property->alloc && property->alloc - length < IDP_ARRAY_REALLOC_LIMIT) {
		IDP_ResizeGroupArray(property, length, IDP_Array(property));
		property->length = length;
		return;
	}

	size_t newsize = (length >> 3) + (length < 9 ? 3 : 6) + length;

	if (!grow) {
		IDP_ResizeGroupArray(property, length, IDP_Array(property));
	}

	size_t element = idp_size_table[property->subtype];
	IDP_Array(property) = MEM_recallocN(IDP_Array(property), element * newsize);

	if (grow) {
		IDP_ResizeGroupArray(property, length, IDP_Array(property));
	}

	property->length = length;
	property->alloc = newsize;
}

void IDP_FreeArray(IDProperty *property) {
	if (IDP_Array(property)) {
		IDP_ResizeGroupArray(property, 0, NULL);
		MEM_freeN(IDP_Array(property));
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name IDP_ID
 * \{ */

IDProperty *IDP_DuplicateID(const IDProperty *prop, const int flag) {
	ROSE_assert(prop->type == IDP_ID);

	IDProperty *newp = IDP_DuplicateGeneric(prop, flag);
	newp->data.pointer = prop->data.pointer;

	if ((flag & LIB_ID_CREATE_NO_USER_REFCOUNT) == 0) {
		id_us_add(IDP_Id(newp));
	}

	return newp;
}

void IDP_AssignID(IDProperty *property, ID *id, int flag) {
	ROSE_assert(property->type == IDP_ID);

	if ((flag & LIB_ID_CREATE_NO_USER_REFCOUNT) == 0 && IDP_Id(property) != NULL) {
		id_us_rem(IDP_Id(property));
	}

	property->data.pointer = id;

	if ((flag & LIB_ID_CREATE_NO_USER_REFCOUNT) == 0) {
		id_us_add(IDP_Id(property));
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name IDP_DOUBLE
 * \{ */

/** \} */

/* -------------------------------------------------------------------- */
/** \name IDP_IDPARRAY
 * \{ */

#define GETPROP(prop, i) &(IDP_IDPArray(prop)[i])

IDProperty *IDP_NewIDPArray(const char *name) {
	IDProperty *property = MEM_callocN(sizeof(IDProperty), "IDProperty(IDPArray)");
	property->type = IDP_IDPARRAY;
	property->length = 0;
	LIB_strcpy(property->name, ARRAY_SIZE(property->name), name);
	return property;
}

IDProperty *IDP_DuplicateIDPArray(const IDProperty *array, int flag) {
	/* Don't use MEM_dupallocN because this may be part of an array */
	ROSE_assert(array->type == IDP_IDPARRAY);

	IDProperty *narray = MEM_mallocN(sizeof(IDProperty), "IDProperty(IDPArray)");
	memcpy(narray, array, sizeof(IDProperty));

	narray->data.pointer = MEM_dupallocN(IDP_IDPArray(array));
	for (size_t index = 0; index < narray->length; index++) {
		IDP_CopyProperty_ex(GETPROP(narray, index), GETPROP(narray, index), flag);
	}

	return narray;
}

void IDP_SetIndexArray(IDProperty *property, size_t index, IDProperty *item) {
	ROSE_assert(property->type == IDP_IDPARRAY);

	if (index >= property->length) {
		return;
	}

	IDProperty *old = GETPROP(property, index);
	if (item != old) {
		IDP_FreePropertyContent(old);

		memcpy(old, item, sizeof(IDProperty));
	}
}

IDProperty *IDP_GetIndexArray(IDProperty *property, size_t index) {
	ROSE_assert(property->type == IDP_IDPARRAY);

	return GETPROP(property, index);
}

void IDP_AppendArray(IDProperty *property, IDProperty *item) {
	ROSE_assert(property->type == IDP_IDPARRAY);

	IDP_ResizeIDPArray(property, property->length + 1);
	IDP_SetIndexArray(property, property->length - 1, item);
}

void IDP_ResizeIDPArray(IDProperty *property, size_t length) {
	ROSE_assert(property->type == IDP_IDPARRAY);

	/* first check if the array buffer size has room */
	if (length <= property->alloc) {
		if (length < property->length && property->alloc - length < IDP_ARRAY_REALLOC_LIMIT) {
			for (int index = length; index < property->length; index++) {
				IDP_FreePropertyContent(GETPROP(property, index));
			}

			property->length = length;
			return;
		}
		if (length >= property->length) {
			property->length = length;
			return;
		}
	}

	if (length < property->length) {
		for (int index = length; index < property->length; index++) {
			IDP_FreePropertyContent(GETPROP(property, index));
		}
	}

	size_t newsize = (length >> 3) + (length < 9 ? 3 : 6) + length;
	property->data.pointer = MEM_recallocN(IDP_IDPArray(property), sizeof(IDProperty) * newsize);

	property->length = length;
	property->alloc = newsize;
}

void IDP_FreeIDPArray(IDProperty *property, const bool do_user) {
	ROSE_assert(property->type == IDP_IDPARRAY);

	for (int index = 0; index < property->length; index++) {
		IDP_FreePropertyContent_ex(GETPROP(property, index), do_user);
	}

	if (IDP_IDPArray(property)) {
		MEM_freeN(IDP_IDPArray(property));
	}
}

#undef GETPROP

/** \} */

/* -------------------------------------------------------------------- */
/** \name IDP_BOOLEAN
 * \{ */

/** \} */

/* -------------------------------------------------------------------- */
/** \name IDP_GROUP
 * \{ */

IDProperty *IDP_DuplicateGroup(const IDProperty *property, const int flag) {
	ROSE_assert(property->type == IDP_GROUP);

	IDProperty *newp = IDP_DuplicateGeneric(property, flag);
	newp->length = property->length;
	newp->subtype = property->subtype;

	LIB_listbase_clear(&newp->data.group);
	LISTBASE_FOREACH(IDProperty *, link, &property->data.group) {
		LIB_addtail(&newp->data.group, IDP_DuplicateProperty_ex(link, flag));
	}

	return newp;
}

void IDP_ResizeGroupArray(IDProperty *property, int length, void *data) {
	if (property->subtype == IDP_GROUP) {
		if (length >= property->length) {
			IDProperty **array = (IDProperty **)data;
			for (int index = property->length; index < length; index++) {
				array[index] = IDP_New(IDP_GROUP, NULL, 0, "IDP_ResizeArray::Group", 0);
			}
		}
		else {
			IDProperty **array = (IDProperty **)IDP_Array(property);
			for (int index = length; index < property->length; index++) {
				IDP_FreeProperty(array[index]);
			}
		}
	}
}

void IDP_SyncGroupValues(IDProperty *destination, const IDProperty *source) {
	ROSE_assert(destination->type == IDP_GROUP);
	ROSE_assert(source->type == IDP_GROUP);

	LISTBASE_FOREACH(IDProperty *, property, &source->data.group) {
		IDProperty *other = LIB_findstr(&destination->data.group, property->name, offsetof(IDProperty, name));
		if (other && property->type == other->type) {
			switch (property->type) {
				case IDP_INT:
				case IDP_FLOAT:
				case IDP_DOUBLE:
				case IDP_BOOLEAN: {
					memcpy(&other->data, &property->data, sizeof(IDPropertyData));
				} break;
				case IDP_GROUP: {
					IDP_SyncGroupValues(other, property);
				} break;
				default: {
					LIB_insertlinkreplace(&destination->data.group, other, IDP_DuplicateProperty(property));
					IDP_FreeProperty(other);
				} break;
			}
		}
	}
}

void IDP_SyncGroupTypes(IDProperty *destination, const IDProperty *source, bool do_arraylen) {
	ROSE_assert(destination->type == IDP_GROUP);
	ROSE_assert(source->type == IDP_GROUP);

	LISTBASE_FOREACH_MUTABLE(IDProperty *, property, &destination->data.group) {
		const IDProperty *other = IDP_GetPropertyFromGroup((IDProperty *)source, property->name);
		if (other != NULL) {
			const bool type_missmatch = (property->type != other->type || property->subtype != other->subtype);
			const bool length_missmatch = (do_arraylen && ELEM(property->type, IDP_ARRAY, IDP_IDPARRAY) && (other->length != property->length));
			if (type_missmatch || length_missmatch) {
				LIB_insertlinkreplace(&destination->data.group, property, IDP_DuplicateProperty(other));
				IDP_FreeProperty(property);
			}
			else if (property->type == IDP_GROUP) {
				IDP_SyncGroupTypes(property, other, do_arraylen);
			}
		}
		else {
			IDP_FreeFromGroup(destination, property);
		}
	}
}

void IDP_ReplaceGroupInGroup(IDProperty *destination, const IDProperty *source) {
	ROSE_assert(destination->type == IDP_GROUP);
	ROSE_assert(source->type == IDP_GROUP);

	LISTBASE_FOREACH(IDProperty *, property, &source->data.group) {
		IDProperty *loop;
		for (loop = (IDProperty *)destination->data.group.first; loop; loop = loop->next) {
			if (STREQ(loop->name, property->name)) {
				LIB_insertlinkreplace(&destination->data.group, loop, IDP_DuplicateProperty(property));
				IDP_FreeProperty(loop);
				break;
			}
		}

		/* Only add at end if not added yet */
		if (loop == NULL) {
			IDProperty *copy = IDP_DuplicateProperty(property);
			destination->length++;
			LIB_addtail(&destination->data.group, copy);
		}
	}
}

void IDP_ReplaceInGroup(IDProperty *group, IDProperty *property) {
	IDProperty *exist = IDP_GetPropertyFromGroup(group, property->name);

	IDP_ReplaceInGroup_ex(group, property, exist);
}

void IDP_ReplaceInGroup_ex(IDProperty *group, IDProperty *property, IDProperty *exist) {
	ROSE_assert(property->type == IDP_GROUP);
	ROSE_assert(group->type == IDP_GROUP);

	if (exist != NULL) {
		LIB_insertlinkreplace(&group->data.group, exist, property);
		IDP_FreeProperty(exist);
	}
	else {
		group->length++;
		LIB_addtail(&group->data.group, property);
	}
}

void IDP_MergeGroup(IDProperty *destination, const IDProperty *source, bool overwrite) {
	IDP_MergeGroup_ex(destination, source, overwrite, 0);
}

void IDP_MergeGroup_ex(IDProperty *destination, const IDProperty *source, bool overwrite, int flag) {
	ROSE_assert(destination->type == IDP_GROUP);
	ROSE_assert(source->type == IDP_GROUP);

	if (overwrite) {
		LISTBASE_FOREACH(IDProperty *, property, &source->data.group) {
			if (property->type == IDP_GROUP) {
				IDProperty *exist = IDP_GetPropertyFromGroup(destination, property->name);

				if (exist != NULL) {
					IDP_MergeGroup_ex(exist, property, overwrite, flag);
					continue;
				}
			}

			IDProperty *newp = IDP_DuplicateProperty_ex(property, flag);
			IDP_ReplaceInGroup(destination, newp);
		}
	}
	else {
		LISTBASE_FOREACH(IDProperty *, property, &source->data.group) {
			IDProperty *exist = IDP_GetPropertyFromGroup(destination, property->name);
			if (exist != NULL) {
				if (property->type == IDP_GROUP) {
					IDP_MergeGroup_ex(exist, property, overwrite, flag);
					continue;
				}
			}
			else {
				IDProperty *newp = IDP_DuplicateProperty_ex(property, flag);
				destination->length++;
				LIB_addtail(&destination->data.group, newp);
			}
		}
	}
}

bool IDP_AddToGroup(IDProperty *group, IDProperty *property) {
	ROSE_assert(group->type == IDP_GROUP);

	if (IDP_GetPropertyFromGroup(group, property->name) == NULL) {
		group->length++;
		LIB_addtail(&group->data.group, property);
		return true;
	}

	return false;
}

bool IDP_InsertToGroup(IDProperty *group, IDProperty *previous, IDProperty *property) {
	ROSE_assert(group->type == IDP_GROUP);

	if (IDP_GetPropertyFromGroup(group, property->name) == NULL) {
		group->length++;
		LIB_insertlinkafter(&group->data.group, previous, property);
		return true;
	}

	return false;
}

void IDP_RemoveFromGroup(IDProperty *group, IDProperty *property) {
	ROSE_assert(group->type == IDP_GROUP);
	ROSE_assert(LIB_haslink(&group->data.group, property));

	group->length--;
	LIB_remlink(&group->data.group, property);
}

void IDP_FreeFromGroup(IDProperty *group, IDProperty *property) {
	IDP_RemoveFromGroup(group, property);
	IDP_FreeProperty(property);
}

IDProperty *IDP_GetPropertyFromGroup(const IDProperty *property, const char *name) {
	ROSE_assert(property->type == IDP_GROUP);

	return LIB_findstr(&property->data.group, name, offsetof(IDProperty, name));
}

IDProperty *IDP_GetPropertyTypeFromGroup(const IDProperty *property, const char *name, int type) {
	IDProperty *result = IDP_GetPropertyFromGroup(property, name);

	return (result && result->type == type) ? result : NULL;
}

void IDP_FreeGroup(IDProperty *property, const bool do_user) {
	ROSE_assert(property->type == IDP_GROUP);

	LISTBASE_FOREACH(IDProperty *, loop, &property->data.group) {
		IDP_FreePropertyContent_ex(loop, do_user);
	}
	LIB_freelistN(&property->data.group);
}

/** \} */
