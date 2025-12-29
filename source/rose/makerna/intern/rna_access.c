#include "MEM_guardedalloc.h"

#include "LIB_string.h"

#include "KER_idtype.h"
#include "KER_idprop.h"
#include "KER_lib_id.h"

#include "RNA_access.h"
#include "RNA_types.h"

#ifdef RNA_RUNTIME
#	include "RNA_prototypes.h"
#endif

#include "rna_internal_types.h"
#include "rna_internal.h"

#include <limits.h>
#include <stdio.h>

/* NOTE: Initializing this object here is find for now, as it should not allocate any memory. */
const PointerRNA PointerRNA_NULL = {
	NULL,
	NULL,
	NULL,
};

ROSE_STATIC void rna_pointer_refine(PointerRNA *ptr) {
	while (ptr->type->refine) {
		StructRNA *type = ptr->type->refine(ptr);
		if (type == ptr->type) {
			break;
		}
		ptr->type = type;
	}
}

/* -------------------------------------------------------------------- */
/** \name Public API
 * \{ */

PointerRNA RNA_id_pointer_create(ID *id) {
	if (id) {
		PointerRNA ptr = {
			.data = id,
			.owner = id,
			.type = RNA_id_code_to_rna_type(GS(id->name)),
		};

		rna_pointer_refine(&ptr);
		return ptr;
	}

	return PointerRNA_NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Private API
 * \{ */

IDProperty **RNA_struct_system_idprops_p(PointerRNA *ptr) {
	StructRNA *type = ptr->type;
	if (type == NULL) {
		return NULL;
	}
	if (type->system_idproperties == NULL) {
		return NULL;
	}

	return type->system_idproperties(ptr);
}

IDProperty *RNA_struct_system_idprops(PointerRNA *ptr, bool create) {
	IDProperty **property_ptr = RNA_struct_system_idprops_p(ptr);
	if (property_ptr == NULL) {
		return NULL;
	}

	if (create && *property_ptr == NULL) {
		*property_ptr = IDP_New(IDP_GROUP, NULL, 0, "system_properties", 0);
	}

	return *property_ptr;
}

IDProperty *rna_system_idproperty_find(PointerRNA *ptr, const char *name) {
	IDProperty *group = RNA_struct_system_idprops(ptr, false);

	if (group == NULL) {
		return NULL;
	}

	if (group->type == IDP_GROUP) {
		return IDP_GetPropertyFromGroup(group, name);
	}

	return NULL;
}

ROSE_INLINE int rna_ensure_property_array_length(const PointerRNA *ptr, PropertyRNA *property) {
	if (property->magic == RNA_MAGIC) {
		/** We need to add custom get length here if one is implemented! */
		return property->totarraylength;
	}

	/**
	 * We ought to map IDProperty too!
	 */
	ROSE_assert_unreachable();

	return 0;
}

ROSE_INLINE bool rna_idproperty_verify_valid(PointerRNA *ptr, PropertyRNA *prop, IDProperty *idprop) {
	/* this verifies if the idproperty actually matches the property
	 * description and otherwise removes it. this is to ensure that
	 * rna property access is type safe, e.g. if you defined the rna
	 * to have a certain array length you can count on that staying so */

	switch (idprop->type) {
		case IDP_IDPARRAY:
			if (prop->type != PROP_COLLECTION) {
				return false;
			}
			break;
		case IDP_ARRAY:
			if (rna_ensure_property_array_length(ptr, prop) != idprop->length) {
				return false;
			}

			if (idprop->subtype == IDP_FLOAT && prop->type != PROP_FLOAT) {
				return false;
			}
			if (idprop->subtype == IDP_BOOLEAN && prop->type != PROP_BOOLEAN) {
				return false;
			}
			if (idprop->subtype == IDP_INT && !ELEM(prop->type, PROP_BOOLEAN, PROP_INT, PROP_ENUM)) {
				return false;
			}

			break;
		case IDP_INT:
			if (!ELEM(prop->type, PROP_BOOLEAN, PROP_INT, PROP_ENUM)) {
				return false;
			}
			break;
		case IDP_BOOLEAN:
			if (prop->type != PROP_BOOLEAN) {
				return false;
			}
			break;
		case IDP_FLOAT:
		case IDP_DOUBLE:
			if (prop->type != PROP_FLOAT) {
				return false;
			}
			break;
		case IDP_STRING:
			if (prop->type != PROP_STRING) {
				return false;
			}
			break;
		case IDP_GROUP:
		case IDP_ID:
			if (prop->type != PROP_POINTER) {
				return false;
			}
			break;
		default:
			return false;
	}

	return true;
}

void rna_property_rna_or_id_get(PropertyRNA *prop, PointerRNA *ptr, PropertyRNAorID *r_info) {
	memset(r_info, 0, sizeof(PropertyRNAorID));

	r_info->ptr = ptr;
	r_info->rawprop = prop;

	if (prop->magic == RNA_MAGIC) {
		r_info->rnaprop = prop;
		r_info->identifier = prop->identifier;
		r_info->is_array = prop->totarraylength > 0;

		if (r_info->is_array) {
			r_info->length = prop->totarraylength;
		}

		if (prop->flag & PROP_IDPROPERTY) {
			IDProperty *idprop = rna_system_idproperty_find(ptr, prop->identifier);

			if (idprop != NULL && !rna_idproperty_verify_valid(ptr, prop, idprop)) {
				IDProperty *group = RNA_struct_system_idprops(ptr, false);

				IDP_FreeFromGroup(group, idprop);
				idprop = NULL;
			}

			r_info->idprop = idprop;
			r_info->is_rna_storage_idprop = true;
			r_info->is_set = (idprop != NULL);
		}
		else {
			r_info->is_set = true;
		}
	}
	else {
		ROSE_assert_unreachable();
	}
}

IDProperty *rna_idproperty_check(PropertyRNA **prop, PointerRNA *ptr) {
	PropertyRNAorID info;
	rna_property_rna_or_id_get(*prop, ptr, &info);
	*prop = info.rnaprop;
	return info.idprop;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Path API
 * \{ */

char *rna_path_token(const char **path, char *fixedbuf, size_t fixedlen) {
	size_t length = 0;

	const char *p = *path;
	while (*p && !ELEM(*p, '.', '[')) {
#ifdef RNA_USE_CANONICAL_PATH
		if (ELEM(*p, '&')) {
			length += sizeof(unsigned int);
			p += sizeof(unsigned int);
		}
#endif
		length++;
		p++;
	}

	if (length == 0) {
		return NULL;
	}

	char *buf = (length + 1 < fixedlen) ? fixedbuf : MEM_mallocN(length + 1, __func__);
	memcpy(buf, *path, length);
	buf[length] = '\0';

	if (*p == '.') {
		p++;
	}
	*path = p;

	return buf;
}

char *rna_path_token_in_brackets(const char **path, char *fixedbuf, size_t fixedlen, bool *quoted) {
	size_t length = 0;

	ROSE_assert(quoted != NULL);
	*quoted = false;

	if (**path != '[') {
		return NULL;
	}

	(*path)++;
	const char *p = *path;

	/* 2 kinds of look-ups now, quoted or unquoted. */
	if (*p == '\"') {
		(*path)++;
		p = *path;
		const char *end = LIB_str_escape_find_quote(p);
		if (end == NULL) {
			return NULL;
		}

		length += (end - p);

		end++;
		p = end;
		*quoted = true;
	}
	else {
		while (*p && (*p != ']')) {
			length++;
			p++;
		}
	}

	if (*p != ']') {
		return NULL;
	}

	if (!quoted) {
		if (length == 0) {
			return NULL;
		}
	}

	char *buf = (length + 1 < fixedlen) ? fixedbuf : MEM_mallocN(length + 1, __func__);

	if (quoted) {
		LIB_strcpy_unescape_ex(buf, *path, length);
		p = (*path) + length + 1;
	}
	else {
		memcpy(buf, *path, length);
		buf[length] = '\0';
	}

	if (*p == ']') {
		p++;
	}
	if (*p == '.') {
		p++;
	}

	*path = p;

	return buf;
}

bool rna_path_parse_collection_key(const char **path, PointerRNA *ptr, PropertyRNA *prop, PointerRNA *r_nextptr) {
	char fixedbuf[256];
	int intkey;

	*r_nextptr = *ptr;

	/* end of path, ok */
	if (!(**path)) {
		return true;
	}

	 bool found = false;
	if (**path == '[') {
		bool quoted;
		char *token;

		/* resolve the lookup with [] brackets */
		token = rna_path_token_in_brackets(path, fixedbuf, ARRAY_SIZE(fixedbuf), &quoted);

		if (!token) {
			return false;
		}

		/* check for "" to see if it is a string */
		if (quoted) {
			if (RNA_property_collection_lookup_string(ptr, prop, token, r_nextptr)) {
				found = true;
			}
			else {
				r_nextptr->data = NULL;
			}
		}
		else {
			/* otherwise do int lookup */
			intkey = atoi(token);
			if (intkey == 0 && (token[0] != '0' || token[1] != '\0')) {
				return false; /* we can be sure the fixedbuf was used in this case */
			}
			if (RNA_property_collection_lookup_int(ptr, prop, intkey, r_nextptr)) {
				found = true;
			}
			else {
				r_nextptr->data = NULL;
			}
		}

		if (token != fixedbuf) {
			MEM_freeN(token);
		}
	}
	else {
		if (RNA_property_collection_type_get(ptr, prop, r_nextptr)) {
			found = true;
		}
		else {
			/* ensure we quit on invalid values */
			r_nextptr->data = NULL;
		}
	}

	return found;
}

ROSE_INLINE bool rna_path_parse(const PointerRNA *ptr, const char *path, PointerRNA *r_ptr, PropertyRNA **r_property) {
	PointerRNA curptr, nextptr;
	PropertyRNA *property = NULL;

	curptr = *ptr;

	/* look up property name in current struct */
	bool quoted = false;

	char fixedbuf[256];
	while (*path) {
		const bool brackets = (*path == '[');

		char *token;
		if (brackets) {
			token = rna_path_token_in_brackets(&path, fixedbuf, ARRAY_SIZE(fixedbuf), &quoted);
		}
		else {
			token = rna_path_token(&path, fixedbuf, ARRAY_SIZE(fixedbuf));
		}

		if (!token) {
			return false;
		}

		if (brackets) {
			/** Look up property name in current struct. */
			IDProperty *group = RNA_struct_idprops(&curptr);

			if (group && quoted) {
				property = (PropertyRNA *)IDP_GetPropertyFromGroup(group, token);
			}
		}
		else {
			property = RNA_struct_find_property(&curptr, token);
		}

		if (token != fixedbuf) {
			MEM_freeN(token);
		}

		if (!property) {
			return false;
		}

		ePropertyType type = RNA_property_type(property);

		switch (type) {
			case PROP_POINTER: {
				if (*path != '\0') {
					curptr = RNA_property_pointer_get(&curptr, property);

					/* now we have a PointerRNA, the #property is our parent so forget it */
					property = NULL;
				}
			} break;
			case PROP_COLLECTION: {
				/* Resolve pointer if further path elements follow.
				 * Note that if path is empty, rna_path_parse_collection_key will do nothing anyway,
				 * so do_item_ptr is of no use in that case.
				 */
				if (*path) {
					if (!rna_path_parse_collection_key(&path, &curptr, property, &nextptr)) {
						return false;
					}

					if (*path != '\0') {
						curptr = nextptr;

						/* now we have a PointerRNA, the prop is our parent so forget it */
						property = NULL;
					}
				}
			} break;
		}
	}

	if (r_ptr) {
		*r_ptr = curptr;
	}
	if (r_property) {
		*r_property = property;
	}

	return true;
}

bool RNA_path_resolve_property(const PointerRNA *ptr, const char *path, PointerRNA *r_ptr, PropertyRNA **r_property) {
	if (!rna_path_parse(ptr, path, r_ptr, r_property)) {
		return false;
	}

	return r_ptr->data != NULL && *r_property != NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name StringPropertyRNA
 * \{ */

ROSE_INLINE size_t property_string_length_storage(PointerRNA *ptr, PropertyRNAorID prop_rna_or_id) {
	if (prop_rna_or_id.is_idprop) {
		IDProperty *idprop = prop_rna_or_id.idprop;
		ROSE_assert(LIB_strlen(IDP_String(idprop)) == idprop->length - 1);
		return (size_t)(idprop->length - 1);
	}
	
	StringPropertyRNA *sproperty = (StringPropertyRNA *)prop_rna_or_id.rnaprop;
	if (sproperty->length) {
		return sproperty->length(ptr);
	}
	if (sproperty->length_ex) {
		return sproperty->length_ex(ptr, &sproperty->property);
	}

	return LIB_strlen(sproperty->defaultvalue);
}

char *RNA_property_string_get_alloc(PointerRNA *ptr, PropertyRNA *property, char *fixedbuf, size_t fixedlen, int *r_length) {
	PropertyRNAorID prop_rna_or_id;
	rna_property_rna_or_id_get(property, ptr, &prop_rna_or_id);
	
	/* Make initial `prop` pointer invalid, to ensure that it is not used anywhere below. */
	property = NULL;

	const size_t length = property_string_length_storage(ptr, prop_rna_or_id);

	if (r_length) {
		*r_length = length;
	}

	char *buf = (length + 1 < fixedlen) ? fixedbuf : MEM_mallocN(length + 1, __func__);

	if (prop_rna_or_id.idprop) {
		LIB_strncpy(buf, length + 1, IDP_String(prop_rna_or_id.idprop), length);
		return buf;
	}
	else {
		StringPropertyRNA *sproperty = (StringPropertyRNA *)prop_rna_or_id.rnaprop;

		if (sproperty->get) {
			sproperty->get(ptr, buf);
			return buf;
		}
		if (sproperty->get_ex) {
			sproperty->get_ex(ptr, &sproperty->property, buf);
			return buf;
		}

		LIB_strncpy(buf, length + 1, sproperty->defaultvalue, length);
		return buf;
	}

	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name PropertyRNA
 * \{ */

ROSE_INLINE PropertyRNA *rna_ensure_property(PropertyRNA *original) {
	if (original->magic == RNA_MAGIC) {
		return original;
	}

	/**
	 * We ought to map IDProperty too!
	 */
	ROSE_assert_unreachable();

	return NULL;
}

ROSE_INLINE bool id_type_can_have_animdata(const short id_type) {
	const IDTypeInfo *typeinfo = KER_idtype_get_info_from_idcode(id_type);
	if (typeinfo != NULL) {
		return (typeinfo->flag & IDTYPE_FLAGS_NO_ANIMDATA) == 0;
	}
	return false;
}

ROSE_INLINE bool id_can_have_animdata(ID *id) {
	if (id == NULL) {
		return false;
	}

	return id_type_can_have_animdata(GS(id->name));
}

bool RNA_property_animateable(const PointerRNA *ptr, PropertyRNA *property) {
	if (!id_can_have_animdata(ptr->owner)) {
		return false;
	}

	PropertyRNA *p = rna_ensure_property(property);

	if (!(p->flag & PROP_ANIMATABLE)) {
		return false;
	}

	return true;
}

ROSE_INLINE bool rna_property_editable_do(const PointerRNA *ptr, PropertyRNA *original, const int index) {
	ID *id = ptr->owner;

	PropertyRNA *property = rna_ensure_property(original);

	/* Early return if the property itself is not editable. */
	if ((property->flag & PROP_EDITABLE) == 0) {
		return false;
	}

	/* If there is no owning ID, the property is editable at this point. */
	if (id == NULL) {
		return true;
	}

	/* At this point, property is owned by a local ID and therefore fully editable. */
	return true;
}

bool RNA_property_editable(const PointerRNA *ptr, PropertyRNA *property) {
	return rna_property_editable_do(ptr, property, -1);
}

ePropertyType RNA_property_type(const PropertyRNA *property) {
	PropertyRNA *p = rna_ensure_property((PropertyRNA *)property);

	return p->type;
}

int RNA_property_array_length(const PointerRNA *ptr, PropertyRNA *property) {
	return rna_ensure_property_array_length(ptr, property);
}

bool RNA_property_array_check(const PropertyRNA *property) {
	if (property->magic == RNA_MAGIC) {
		return property->totarraylength > 0;
	}
	IDProperty *idprop = (IDProperty *)property;

	return (idprop->type == IDP_ARRAY);
}

void RNA_property_collection_begin(PointerRNA *ptr, PropertyRNA *property, CollectionPropertyIterator *iter) {
	ROSE_assert(RNA_property_type(property) == PROP_COLLECTION);

	memset(iter, 0, sizeof(CollectionPropertyIterator));

	IDProperty *idprop;
	if ((idprop = rna_idproperty_check(&property, ptr)) || (property->flag & PROP_IDPROPERTY)) {
		iter->parent = *ptr;
		iter->property = property;

		if (idprop) {
			ROSE_assert_unreachable();
		}
	}
	else {
		CollectionPropertyRNA *cproperty = (CollectionPropertyRNA *)property;

		cproperty->begin(iter, ptr);
	}
}

void RNA_property_collection_next(CollectionPropertyIterator *iter) {
	CollectionPropertyRNA *cproperty = (CollectionPropertyRNA *)rna_ensure_property(iter->property);

	cproperty->next(iter);
}

void RNA_property_collection_end(CollectionPropertyIterator *iter) {
	CollectionPropertyRNA *cprop = (CollectionPropertyRNA *)rna_ensure_property(iter->property);

	cprop->end(iter);
}

bool RNA_property_collection_lookup_string_ex(PointerRNA *ptr, PropertyRNA *property, const char *key, PointerRNA *r_ptr, int *r_index) {
	CollectionPropertyRNA *cproperty = (CollectionPropertyRNA *)rna_ensure_property(property);

	ROSE_assert(RNA_property_type(property) == PROP_COLLECTION);

	if (!key) {
		*r_ptr = PointerRNA_NULL;
		return false;
	}

	if (cproperty->lookupstring) {
		return cproperty->lookupstring(ptr, key, r_ptr);
	}

	CollectionPropertyIterator iter;

	int index = 0;

	int keylength = LIB_strlen(key);
	int vallength = 0;

	RNA_property_collection_begin(ptr, property, &iter);
	for (; iter.valid; RNA_property_collection_next(&iter)) {
		if (iter.ptr.data && iter.ptr.type->nameproperty) {
			PropertyRNA *name = iter.ptr.type->nameproperty;

			char inlinebuf[256];

			char *namebuf = RNA_property_string_get_alloc(&iter.ptr, name, inlinebuf, ARRAY_SIZE(inlinebuf), &vallength);

			bool found = false;
			if ((keylength == vallength) && STREQ(namebuf, key)) {
				*r_ptr = iter.ptr;
				found = true;
			}

			if (namebuf != inlinebuf) {
				MEM_freeN(namebuf);
			}

			if (found) {
				break;
			}
		}
	}
	RNA_property_collection_end(&iter);

	if (!iter.valid) {
		*r_ptr = PointerRNA_NULL;
		*r_index = -1;
	}
	else {
		*r_index = index;
	}

	return iter.valid;
}

bool RNA_property_collection_lookup_string(PointerRNA *ptr, PropertyRNA *property, const char *key, PointerRNA *r_ptr) {
	int index;
	return RNA_property_collection_lookup_string_ex(ptr, property, key, r_ptr, &index);
}

bool RNA_property_collection_lookup_int(PointerRNA *ptr, PropertyRNA *property, int key, PointerRNA *r_ptr) {
	CollectionPropertyRNA *cprop = (CollectionPropertyRNA *)rna_ensure_property(property);

	ROSE_assert(RNA_property_type(property) == PROP_COLLECTION);

	if (cprop->lookupint) {
		/* we have a callback defined, use it */
		return cprop->lookupint(ptr, key, r_ptr);
	}
	/* no callback defined, just iterate and find the nth item */
	CollectionPropertyIterator iter;
	int i;

	RNA_property_collection_begin(ptr, property, &iter);
	for (i = 0; iter.valid; RNA_property_collection_next(&iter), i++) {
		if (i == key) {
			*r_ptr = iter.ptr;
			break;
		}
	}
	RNA_property_collection_end(&iter);

	if (!iter.valid) {
		*r_ptr = PointerRNA_NULL;
	}

	return iter.valid;
}

bool RNA_property_collection_type_get(PointerRNA *ptr, PropertyRNA *property, PointerRNA *r_ptr) {
	ROSE_assert(RNA_property_type(property) == PROP_COLLECTION);

	*r_ptr = *ptr;
	return ((r_ptr->type = rna_ensure_property(property)->srna) ? 1 : 0);
}

const char *RNA_property_identifier(const PropertyRNA *property) {
	return property->identifier;
}

const char *RNA_property_name(const PropertyRNA *property) {
	return property->name;
}

const char *RNA_property_description(const PropertyRNA *property) {
	return property->description;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name PropertyRNA Data
 * \{ */

/* boolean */

bool RNA_property_boolean_get(struct PointerRNA *ptr, struct PropertyRNA *property) {
	ROSE_assert(RNA_property_type(property) == PROP_BOOLEAN);

	PropertyRNAorID p;
	rna_property_rna_or_id_get(property, ptr, &p);

	ROSE_assert(!p.is_array);

	if (p.idprop) {
		IDProperty *idproperty = p.idprop;
		return IDP_Bool(idproperty);
	}

	BoolPropertyRNA *bproperty = (BoolPropertyRNA *)p.rnaprop;

	if (bproperty->get) {
		return bproperty->get(ptr);
	}
	if (bproperty->get_ex) {
		return bproperty->get_ex(ptr, p.rnaprop);
	}
	return bproperty->defaultvalue;
}

/* int */

int RNA_property_int_get(PointerRNA *ptr, PropertyRNA *property) {
	ROSE_assert(RNA_property_type(property) == PROP_INT);

	PropertyRNAorID p;
	rna_property_rna_or_id_get(property, ptr, &p);

	ROSE_assert(!p.is_array);

	if (p.idprop) {
		IDProperty *idproperty = p.idprop;
		return IDP_Int(idproperty);
	}

	FloatPropertyRNA *fproperty = (FloatPropertyRNA *)p.rnaprop;

	if (fproperty->get) {
		return fproperty->get(ptr);
	}
	if (fproperty->get_ex) {
		return fproperty->get_ex(ptr, p.rnaprop);
	}
	return fproperty->defaultvalue;
}

void RNA_property_int_set(PointerRNA *ptr, PropertyRNA *property, int value) {
	ROSE_assert(RNA_property_type(property) == PROP_INT);

	PropertyRNAorID p;
	rna_property_rna_or_id_get(property, ptr, &p);

	ROSE_assert(!p.is_array);

	IntPropertyRNA *iproperty = (IntPropertyRNA *)p.rnaprop;

	if (p.idprop) {
		RNA_property_int_clamp(ptr, &iproperty->property, &value);

		IDProperty *idproperty = p.idprop;
		IDP_Int(idproperty) = (int)value;
	}
	else if (iproperty->set) {
		iproperty->set(ptr, value);
	}
	else if (iproperty->set_ex) {
		iproperty->set_ex(ptr, &iproperty->property, value);
	}
	else if (iproperty->property.flag & PROP_EDITABLE) {
		IDProperty *group = RNA_struct_system_idprops(ptr, true);
		if (group) {
			IDP_AddToGroup(group, IDP_New(IDP_INT, &value, 0, p.identifier, 0));
		}
	}
}

void RNA_property_int_get_array(PointerRNA *ptr, PropertyRNA *property, int *r_value) {
	ROSE_assert(RNA_property_type(property) == PROP_INT);

	PropertyRNAorID p;
	rna_property_rna_or_id_get(property, ptr, &p);

	ROSE_assert(p.is_array);

	if (p.idprop) {
		ROSE_assert_unreachable();

		return;
	}

	IntPropertyRNA *iproperty = (IntPropertyRNA *)p.rnaprop;

	if (iproperty->getarray) {
		iproperty->getarray(ptr, r_value);

		return;
	}
	if (iproperty->get_ex) {
		iproperty->getarray_ex(ptr, property, r_value);

		return;
	}
	if (iproperty->get_default_array) {
		iproperty->get_default_array(ptr, property, r_value);

		return;
	}

	int length = RNA_property_array_length(ptr, property);

	if (length) {
		memcpy(r_value, iproperty->defaultarray, length * sizeof(int));

		return;
	}
}

void RNA_property_int_set_array(PointerRNA *ptr, PropertyRNA *property, const int *i_value) {
	ROSE_assert(RNA_property_type(property) == PROP_INT);

	PropertyRNAorID p;
	rna_property_rna_or_id_get(property, ptr, &p);

	ROSE_assert(p.is_array);

	if (p.idprop) {
		ROSE_assert_unreachable();

		return;
	}

	IntPropertyRNA *iproperty = (IntPropertyRNA *)p.rnaprop;

	if (iproperty->setarray) {
		iproperty->setarray(ptr, i_value);

		return;
	}
	if (iproperty->setarray_ex) {
		iproperty->setarray_ex(ptr, property, i_value);

		return;
	}
}

int RNA_int_get(PointerRNA *ptr, const char *name) {
	PropertyRNA *property = RNA_struct_find_property(ptr, name);

	if (property) {
		return RNA_property_int_get(ptr, property);
	}
	fprintf(stderr, "[RNA] %s: \"%s.%s\" not found.\n", __func__, ptr->type->identifier, name);
	return 0;
}

void RNA_int_set(PointerRNA *ptr, const char *name, int value) {
	PropertyRNA *property = RNA_struct_find_property(ptr, name);

	if (property) {
		RNA_property_int_set(ptr, property, value);
	}
	else {
		fprintf(stderr, "[RNA] %s: \"%s.%s\" not found.\n", __func__, ptr->type->identifier, name);
	}
}

void RNA_property_int_range(PointerRNA *ptr, PropertyRNA *property, int *r_hardmin, int *r_hardmax) {
	IntPropertyRNA *iproperty = (IntPropertyRNA *)rna_ensure_property(property);
	int softmin, softmax;

	if (property->magic != RNA_MAGIC) {
		const IDProperty *idprop = (IDProperty *)property;

		*r_hardmin = INT_MIN;
		*r_hardmax = INT_MAX;
		return;
	}

	if (iproperty->range) {
		*r_hardmin = INT_MIN;
		*r_hardmax = INT_MAX;

		iproperty->range(ptr, r_hardmin, r_hardmax, &softmin, &softmax);
	}
	else if (iproperty->range_ex) {
		*r_hardmin = INT_MIN;
		*r_hardmax = INT_MAX;

		iproperty->range_ex(ptr, property, r_hardmin, r_hardmax, &softmin, &softmax);
	}
	else {
		*r_hardmin = iproperty->hardmin;
		*r_hardmax = iproperty->hardmax;
	}
}

void RNA_property_int_ui_range(PointerRNA *ptr, PropertyRNA *property, int *r_softmin, int *r_softmax, int *r_step) {
	IntPropertyRNA *iproperty = (IntPropertyRNA *)rna_ensure_property(property);
	int hardmin, hardmax;

	if (property->magic != RNA_MAGIC) {
		const IDProperty *idprop = (IDProperty *)property;
		*r_softmin = INT_MIN;
		*r_softmax = INT_MAX;
		*r_step = 1;
		return;
	}

	*r_softmin = iproperty->softmin;
	*r_softmax = iproperty->softmax;

	if (iproperty->range) {
		hardmin = INT_MIN;
		hardmax = INT_MAX;

		iproperty->range(ptr, &hardmin, &hardmax, r_softmin, r_softmax);

		*r_softmin = ROSE_MAX(*r_softmin, hardmin);
		*r_softmax = ROSE_MIN(*r_softmax, hardmax);
	}
	else if (iproperty->range_ex) {
		hardmin = INT_MIN;
		hardmax = INT_MAX;

		iproperty->range_ex(ptr, property, &hardmin, &hardmax, r_softmin, r_softmax);

		*r_softmin = ROSE_MAX(*r_softmin, hardmin);
		*r_softmax = ROSE_MIN(*r_softmax, hardmax);
	}

	*r_step = iproperty->step;
}

int RNA_property_int_clamp(PointerRNA *ptr, PropertyRNA *property, int *value) {
	int min, max;

	RNA_property_int_range(ptr, property, &min, &max);

	if (*value < min) {
		*value = min;
		return -1;
	}
	if (*value > max) {
		*value = max;
		return 1;
	}
	return 0;
}

/* float */

float RNA_property_float_get(PointerRNA *ptr, PropertyRNA *property) {
	ROSE_assert(RNA_property_type(property) == PROP_FLOAT);

	PropertyRNAorID p;
	rna_property_rna_or_id_get(property, ptr, &p);

	ROSE_assert(!p.is_array);

	if (p.idprop) {
		IDProperty *idproperty = p.idprop;
		if (idproperty->type == IDP_FLOAT) {
			return IDP_Float(idproperty);
		}
		return IDP_Double(idproperty);
	}

	FloatPropertyRNA *fproperty = (FloatPropertyRNA *)p.rnaprop;

	if (fproperty->get) {
		return fproperty->get(ptr);
	}
	if (fproperty->get_ex) {
		return fproperty->get_ex(ptr, p.rnaprop);
	}
	return fproperty->defaultvalue;
}

void RNA_property_float_set(PointerRNA *ptr, PropertyRNA *property, float value) {
	ROSE_assert(RNA_property_type(property) == PROP_FLOAT);

	PropertyRNAorID p;
	rna_property_rna_or_id_get(property, ptr, &p);

	ROSE_assert(!p.is_array);

	FloatPropertyRNA *fproperty = (FloatPropertyRNA *)p.rnaprop;

	if (p.idprop) {
		RNA_property_float_clamp(ptr, &fproperty->property, &value);

		IDProperty *idproperty = p.idprop;
		if (idproperty->type == IDP_FLOAT) {
			IDP_Float(idproperty) = (float)value;
		}
		else {
			IDP_Double(idproperty) = (double)value;
		}
	}
	else if (fproperty->set) {
		fproperty->set(ptr, value);
	}
	else if (fproperty->set_ex) {
		fproperty->set_ex(ptr, &fproperty->property, value);
	}
	else if (fproperty->property.flag & PROP_EDITABLE) {
		IDProperty *group = RNA_struct_system_idprops(ptr, true);
		if (group) {
			IDP_AddToGroup(group, IDP_New(IDP_FLOAT, &value, 0, p.identifier, 0));
		}
	}
}

void RNA_property_float_get_array(PointerRNA *ptr, PropertyRNA *property, float *r_value) {
	ROSE_assert(RNA_property_type(property) == PROP_FLOAT);

	PropertyRNAorID p;
	rna_property_rna_or_id_get(property, ptr, &p);

	ROSE_assert(p.is_array);

	if (p.idprop) {
		ROSE_assert_unreachable();

		return;
	}

	FloatPropertyRNA *fproperty = (FloatPropertyRNA *)p.rnaprop;

	if (fproperty->getarray) {
		fproperty->getarray(ptr, r_value);

		return;
	}
	if (fproperty->get_ex) {
		fproperty->getarray_ex(ptr, property, r_value);

		return;
	}
	if (fproperty->get_default_array) {
		fproperty->get_default_array(ptr, property, r_value);

		return;
	}
	
	int length = RNA_property_array_length(ptr, property);

	if (length) {
		memcpy(r_value, fproperty->defaultarray, length * sizeof(float));

		return;
	}
}

void RNA_property_float_set_array(PointerRNA *ptr, PropertyRNA *property, const float *f_value) {
	ROSE_assert(RNA_property_type(property) == PROP_FLOAT);

	PropertyRNAorID p;
	rna_property_rna_or_id_get(property, ptr, &p);

	ROSE_assert(p.is_array);

	if (p.idprop) {
		ROSE_assert_unreachable();

		return;
	}

	FloatPropertyRNA *fproperty = (FloatPropertyRNA *)p.rnaprop;

	if (fproperty->setarray) {
		fproperty->setarray(ptr, f_value);

		return;
	}
	if (fproperty->setarray_ex) {
		fproperty->setarray_ex(ptr, property, f_value);

		return;
	}
}

float RNA_property_float_get_index(PointerRNA *ptr, PropertyRNA *property, int index) {
	float tmp[RNA_MAX_ARRAY_LENGTH];
	int len = rna_ensure_property_array_length(ptr, property);

	ROSE_assert(RNA_property_type(property) == PROP_FLOAT);
	ROSE_assert(RNA_property_array_check(property) != false);
	ROSE_assert(index >= 0);
	ROSE_assert(index < len);

	if (len <= RNA_MAX_ARRAY_LENGTH) {
		RNA_property_float_get_array(ptr, property, tmp);
		return tmp[index];
	}
	float *tmparray, value;

	tmparray = MEM_mallocN(sizeof(float) * len, __func__);
	RNA_property_float_get_array(ptr, property, tmparray);
	value = tmparray[index];
	MEM_freeN(tmparray);

	return value;
}

void RNA_property_float_set_index(PointerRNA *ptr, PropertyRNA *property, int index, float value) {
	float tmp[RNA_MAX_ARRAY_LENGTH];
	int len = rna_ensure_property_array_length(ptr, property);

	ROSE_assert(RNA_property_type(property) == PROP_FLOAT);
	ROSE_assert(RNA_property_array_check(property) != false);
	ROSE_assert(index >= 0);
	ROSE_assert(index < len);

	if (len <= RNA_MAX_ARRAY_LENGTH) {
		RNA_property_float_get_array(ptr, property, tmp);
		tmp[index] = value;
		RNA_property_float_set_array(ptr, property, tmp);
	}
	else {
		float *tmparray;

		tmparray = MEM_mallocN(sizeof(float) * len, __func__);
		RNA_property_float_get_array(ptr, property, tmparray);
		tmparray[index] = value;
		RNA_property_float_set_array(ptr, property, tmparray);
		MEM_freeN(tmparray);
	}
}

void RNA_property_float_range(PointerRNA *ptr, PropertyRNA *property, float *r_hardmin, float *r_hardmax) {
	FloatPropertyRNA *fproperty = (FloatPropertyRNA *)rna_ensure_property(property);
	float softmin, softmax;

	if (property->magic != RNA_MAGIC) {
		const IDProperty *idprop = (IDProperty *)property;
		*r_hardmin = -FLT_MAX;
		*r_hardmax = FLT_MAX;
		return;
	}

	if (fproperty->range) {
		*r_hardmin = -FLT_MAX;
		*r_hardmax = FLT_MAX;

		fproperty->range(ptr, r_hardmin, r_hardmax, &softmin, &softmax);
	}
	else if (fproperty->range_ex) {
		*r_hardmin = -FLT_MAX;
		*r_hardmax = FLT_MAX;

		fproperty->range_ex(ptr, property, r_hardmin, r_hardmax, &softmin, &softmax);
	}
	else {
		*r_hardmin = fproperty->hardmin;
		*r_hardmax = fproperty->hardmax;
	}
}

void RNA_property_float_ui_range(PointerRNA *ptr, PropertyRNA *property, float *r_softmin, float *r_softmax, float *r_step, float *r_precision) {
	FloatPropertyRNA *fproperty = (FloatPropertyRNA *)rna_ensure_property(property);
	float hardmin, hardmax;

	if (property->magic != RNA_MAGIC) {
		const IDProperty *idprop = (IDProperty *)property;
		*r_softmin = -FLT_MAX;
		*r_softmax = FLT_MAX;
		*r_step = 1.0f;
		*r_precision = 3.0f;
		return;
	}

	*r_softmin = fproperty->softmin;
	*r_softmax = fproperty->softmax;

	if (fproperty->range) {
		hardmin = -FLT_MAX;
		hardmax = FLT_MAX;

		fproperty->range(ptr, &hardmin, &hardmax, r_softmin, r_softmax);

		*r_softmin = ROSE_MAX(*r_softmin, hardmin);
		*r_softmax = ROSE_MIN(*r_softmax, hardmax);
	}
	else if (fproperty->range_ex) {
		hardmin = -FLT_MAX;
		hardmax = FLT_MAX;

		fproperty->range_ex(ptr, property, &hardmin, &hardmax, r_softmin, r_softmax);

		*r_softmin = ROSE_MAX(*r_softmin, hardmin);
		*r_softmax = ROSE_MIN(*r_softmax, hardmax);
	}

	*r_step = fproperty->step;
	*r_precision = (float)fproperty->precision;
}

int RNA_property_float_clamp(PointerRNA *ptr, PropertyRNA *property, float *value) {
	float min, max;

	RNA_property_float_range(ptr, property, &min, &max);

	if (*value < min) {
		*value = min;
		return -1;
	}
	if (*value > max) {
		*value = max;
		return 1;
	}
	return 0;
}

/* string */

int RNA_property_string_max_length(PropertyRNA *property) {
	StringPropertyRNA *sproperty = (StringPropertyRNA *)rna_ensure_property(property);
	return sproperty->maxlength;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name StructRNA
 * \{ */

PropertyRNA *RNA_struct_iterator_property(StructRNA *type) {
	return type->iteratorproperty;
}

bool RNA_struct_is_ID(const StructRNA *type) {
	return (type->flag & STRUCT_ID) != 0;
}

PropertyRNA *RNA_struct_find_property(PointerRNA *ptr, const char *identifier) {
	if (identifier[0] == '[' && identifier[1] == '"') {
		PropertyRNA *newprop = NULL;

		/* only support single level props */

		PointerRNA newptr;
		if (RNA_path_resolve_property(ptr, identifier, &newptr, &newprop) && (newptr.type == ptr->type) && (newptr.data == ptr->data)) {
			return newprop;
		}
	}
	else {
		/* most common case */
		PropertyRNA *iterprop = RNA_struct_iterator_property(ptr->type);
		PointerRNA propptr;

		if (!iterprop) {
			return NULL;
		}

		if (RNA_property_collection_lookup_string(ptr, iterprop, identifier, &propptr)) {
			return (PropertyRNA *)propptr.data;
		}
	}

	return NULL;
}

IDProperty *RNA_struct_idprops(PointerRNA *ptr) {
	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Iteraton
 * \{ */

void rna_iterator_listbase_begin(CollectionPropertyIterator *iter, PointerRNA *ptr, ListBase *lb, IteratorSkipFunc skip) {
	iter->parent = *ptr;

	ListBaseIterator *internal = &iter->internal.listbase;

	internal->link = (lb) ? (Link *)(lb->first) : NULL;
	internal->skip = skip;

	iter->valid = (internal->link != NULL);

	if (skip && iter->valid && skip(iter, internal->link)) {
		rna_iterator_listbase_next(iter);
	}
}

void rna_iterator_listbase_next(CollectionPropertyIterator *iter) {
	ListBaseIterator *internal = &iter->internal.listbase;

	if (internal->skip) {
		do {
			internal->link = internal->link->next;
			iter->valid = (internal->link != NULL);
		} while (iter->valid && internal->skip(iter, internal->link));
	}
	else {
		internal->link = internal->link->next;
		iter->valid = (internal->link != NULL);
	}
}

void rna_iterator_listbase_end(CollectionPropertyIterator *iter) {
}

void *rna_iterator_listbase_get(CollectionPropertyIterator *iter) {
	ListBaseIterator *internal = &iter->internal.listbase;

	return internal->link;
}

#ifdef RNA_RUNTIME

void rna_builtin_properties_begin(CollectionPropertyIterator *iter, PointerRNA *ptr) {
	PointerRNA newptr;

	/* we create a new pointer with the type as the data */
	newptr.type = &RNA_Struct;
	newptr.data = ptr->type;

	if (ptr->type->flag & STRUCT_ID) {
		newptr.owner = (ID *)(ptr->data);
	}
	else {
		newptr.owner = NULL;
	}

	iter->parent = newptr;
	iter->builtin_parent = *ptr;

	rna_Struct_properties_begin(iter, &newptr);
}

void rna_builtin_properties_next(CollectionPropertyIterator *iter) {
	rna_Struct_properties_next(iter);
}

void rna_builtin_properties_end(CollectionPropertyIterator *iter) {
}

PointerRNA rna_builtin_properties_get(CollectionPropertyIterator *iter) {
	return rna_Struct_properties_get(iter);
}

#endif

/** \} */

/* -------------------------------------------------------------------- */
/** \name Pointer Handling
 * \{ */

void rna_pointer_create_with_ancestors(const PointerRNA *parent, StructRNA *type, void *data, PointerRNA *r_ptr) {
	if (data) {
		if (type && type->flag & STRUCT_ID) {
			/* Currently, assume that an ID PointerRNA never has an ancestor.
			 * NOTE: This may become an issue for embedded IDs in the future, see also
			 * #PointerRNA::ancestors docs. */
			r_ptr->owner = data;
			r_ptr->type = type;
			r_ptr->data = data;
		}
		else {
			r_ptr->owner = parent->owner;
			r_ptr->type = type;
			r_ptr->data = data;
		}
		rna_pointer_refine(r_ptr);
	}
	else {
		r_ptr->owner = NULL;
		r_ptr->type = NULL;
		r_ptr->data = NULL;
	}
}

PointerRNA property_pointer_get(PointerRNA *ptr, PropertyRNA *prop) {
	PointerPropertyRNA *pprop = (PointerPropertyRNA *)prop;
	IDProperty *idprop;

	ROSE_assert(RNA_property_type(prop) == PROP_POINTER);

	if ((idprop = rna_idproperty_check(&prop, ptr))) {
		pprop = (PointerPropertyRNA *)prop;

		if (RNA_struct_is_ID(pprop->srna)) {
			/* ID PointerRNA should not have ancestors currently. */
			return RNA_id_pointer_create(idprop->type == IDP_GROUP ? NULL : IDP_Id(idprop));
		}

		/* for groups, data is idprop itself */
		if (pprop->type) {
			return RNA_pointer_create_with_parent(ptr, pprop->type(ptr), idprop);
		}

		return RNA_pointer_create_with_parent(ptr, pprop->srna, idprop);
	}

	if (pprop->get) {
		return pprop->get(ptr);
	}

	return PointerRNA_NULL;
}

PointerRNA RNA_pointer_create_discrete(ID *id, StructRNA *type, void *data) {
	PointerRNA ptr = {
		.owner = id,
		.type = type,
		.data = data,
	};

	if (data) {
		rna_pointer_refine(&ptr);
	}

	return ptr;
}

PointerRNA RNA_pointer_create_with_parent(const PointerRNA *parent, StructRNA *type, void *data) {
	PointerRNA result;
	rna_pointer_create_with_ancestors(parent, type, data, &result);
	return result;
}

PointerRNA RNA_property_pointer_get(PointerRNA *ptr, PropertyRNA *prop) {
	return property_pointer_get(ptr, prop);
}

/** \} */
