#ifndef RNA_ACCESS_H
#define RNA_ACCESS_H

#include "RNA_types.h"

struct ID;
struct RoseRNA;
struct StructRNA;

struct rContext;

#ifdef __cplusplus
extern "C" {
#endif

struct StructRNA *RNA_id_code_to_rna_type(int idcode);

/* -------------------------------------------------------------------- */
/** \name Public API
 * \{ */

/**
 * Create a PointerRNA for an ID.
 *
 * \note By definition, currently these are always 'discrete' (have no ancestors). See
 * #PointerRNA::ancestors for details.
 */
struct PointerRNA RNA_id_pointer_create(struct ID *id);

void RNA_init();
void RNA_exit();

/** \} */

/* -------------------------------------------------------------------- */
/** \name Path API
 * \{ */

typedef struct StaticPathRNA StaticPathRNA;

/**
 * Resolve the given RNA Path to find both the pointer AND property
 * indicated by fully resolving the path.
 *
 * This is a convenience method to avoid logic errors and ugly syntax.
 * \note Assumes all pointers provided are valid
 * \return True only if both a valid pointer and property are found after resolving the path
 */
bool RNA_path_resolve_property(const struct PointerRNA *ptr, const char *path, struct PointerRNA *r_ptr, struct PropertyRNA **r_property);
bool RNA_static_path_resolve_property(const struct PointerRNA *ptr, const struct StaticPathRNA *path, struct PointerRNA *r_ptr, struct PropertyRNA **r_property);

bool RNA_path_can_do_static_compilation(const struct PointerRNA *ptr, const char *path);

struct StaticPathRNA *RNA_path_new(const struct PointerRNA *ptr, const char *path, struct PointerRNA *r_ptr, struct PropertyRNA **r_property);

void RNA_path_free(struct StaticPathRNA *path);

/** \} */

/* -------------------------------------------------------------------- */
/** \name PropertyRNA
 * \{ */

ePropertyType RNA_property_type(const struct PropertyRNA *property);
bool RNA_property_is_idprop(const struct PropertyRNA *property);

/**
 * A property is animateable if its ID and the RNA property itself are defined as editable.
 * It does not imply that user can _edit_ such animation though, see #RNA_property_anim_editable
 * for this.
 *
 * This check is only based on information stored in the data _types_ (IDTypeInfo and RNA property
 * definition), not on the actual data itself.
 */
bool RNA_property_animateable(const struct PointerRNA *ptr, struct PropertyRNA *property);
bool RNA_property_editable(const struct PointerRNA *ptr, struct PropertyRNA *property);

/**
 * Check if the #IDproperty exists, for operators.
 *
 * \param use_ghost: Internally an #IDProperty may exist,
 * without the RNA considering it to be "set", see #IDP_FLAG_GHOST.
 * This is used for operators, where executing an operator that has run previously
 * will re-use the last value (unless #PROP_SKIP_SAVE property is set).
 * In this case, the presence of an existing value shouldn't prevent it being initialized
 * from the context. Even though this value will be returned if it's requested,
 * it's not considered to be set (as it would if the menu item or key-map defined it's value).
 * Set `use_ghost` to true for default behavior, otherwise false to check if there is a value
 * exists internally and would be returned on request.
 */
bool RNA_property_is_set_ex(struct PointerRNA *ptr, struct PropertyRNA *prop, bool use_ghost);
bool RNA_property_is_set(struct PointerRNA *ptr, struct PropertyRNA *prop);
void RNA_property_unset(struct PointerRNA *ptr, struct PropertyRNA *prop);

void RNA_property_update(struct rContext *C, struct PointerRNA *ptr, struct PropertyRNA *prop);
/**
 * \note its possible this returns a false positive in the case of #PROP_CONTEXT_UPDATE
 * but this isn't likely to be a performance problem.
 */
bool RNA_property_update_check(struct PropertyRNA *prop);

/** Returns the length of the array defined by the property. */
int RNA_property_array_length(const struct PointerRNA *ptr, struct PropertyRNA *property);
bool RNA_property_array_check(const struct PropertyRNA *property);

/**
 * Searches for the specified string key in a collection property.
 * \note If a direct string search function is not implemented we look in the name properties 
 * of the specified collection.
 */
bool RNA_property_collection_lookup_string(struct PointerRNA *ptr, struct PropertyRNA *property, const char *key, PointerRNA *r_ptr);
bool RNA_property_collection_lookup_int(struct PointerRNA *ptr, struct PropertyRNA *property, int key, struct PointerRNA *r_ptr);
bool RNA_property_collection_type_get(struct PointerRNA *ptr, struct PropertyRNA *property, struct PointerRNA *r_ptr);

const char *RNA_property_identifier(const struct PropertyRNA *property);
const char *RNA_property_name(const struct PropertyRNA *property);
const char *RNA_property_description(const struct PropertyRNA *property);

/** \} */

/* -------------------------------------------------------------------- */
/** \name PropertyRNA Data
 * \{ */

/* boolean */

bool RNA_property_boolean_get(struct PointerRNA *ptr, struct PropertyRNA *property);
void RNA_property_boolean_set(struct PointerRNA *ptr, struct PropertyRNA *property, bool value);
// void RNA_property_boolean_get_array(struct PointerRNA *ptr, struct PropertyRNA *property, bool *r_value);
// void RNA_property_boolean_set_array(struct PointerRNA *ptr, struct PropertyRNA *property, const bool *i_value);

bool RNA_boolean_get(struct PointerRNA *ptr, const char *name);
void RNA_boolean_set(struct PointerRNA *ptr, const char *name, bool value);

/* int */

int RNA_property_int_get(struct PointerRNA *ptr, struct PropertyRNA *property);
void RNA_property_int_set(struct PointerRNA *ptr, struct PropertyRNA *property, int value);
void RNA_property_int_get_array(struct PointerRNA *ptr, struct PropertyRNA *property, int *r_value);
void RNA_property_int_set_array(struct PointerRNA *ptr, struct PropertyRNA *property, const int *i_value);

int RNA_int_get(struct PointerRNA *ptr, const char *name);
void RNA_int_set(struct PointerRNA *ptr, const char *name, int value);

void RNA_property_int_range(struct PointerRNA *ptr, struct PropertyRNA *property, int *r_hardmin, int *r_hardmax);
void RNA_property_int_ui_range(struct PointerRNA *ptr, struct PropertyRNA *property, int *r_softmin, int *r_softmax, int *r_step);

int RNA_property_int_clamp(struct PointerRNA *ptr, struct PropertyRNA *property, int *value);

/* float */

float RNA_property_float_get(struct PointerRNA *ptr, struct PropertyRNA *property);
void RNA_property_float_set(struct PointerRNA *ptr, struct PropertyRNA *property, float value);
void RNA_property_float_get_array(struct PointerRNA *ptr, struct PropertyRNA *property, float *r_value);
void RNA_property_float_set_array(struct PointerRNA *ptr, struct PropertyRNA *property, const float *f_value);

float RNA_property_float_get_index(struct PointerRNA *ptr, struct PropertyRNA *property, int index);
void RNA_property_float_set_index(struct PointerRNA *ptr, struct PropertyRNA *property, int index, float value);

void RNA_property_float_range(struct PointerRNA *ptr, struct PropertyRNA *property, float *r_hardmin, float *r_hardmax);
void RNA_property_float_ui_range(struct PointerRNA *ptr, struct PropertyRNA *property, float *r_softmin, float *r_softmax, float *r_step, float *r_precision);

int RNA_property_float_clamp(struct PointerRNA *ptr, struct PropertyRNA *property, float *value);

/* string */

void RNA_property_string_get(struct PointerRNA *ptr, struct PropertyRNA *property, char *value);
void RNA_property_string_set(struct PointerRNA *ptr, struct PropertyRNA *property, const char *value);
int RNA_property_string_length(struct PointerRNA *ptr, struct PropertyRNA *property);

void RNA_string_get(struct PointerRNA *ptr, const char *name, char *value);
void RNA_string_set(struct PointerRNA *ptr, const char *name, const char *value);
int RNA_string_length(struct PointerRNA *ptr, const char *name);

int RNA_property_string_max_length(struct PropertyRNA *property);

/* collection */

void RNA_property_collection_begin(PointerRNA *ptr, PropertyRNA *property, CollectionPropertyIterator *iter);
void RNA_property_collection_next(CollectionPropertyIterator *iter);
void RNA_property_collection_end(CollectionPropertyIterator *iter);

#define RNA_PROP_BEGIN(sptr, itemptr, prop)                                                                                                     \
	{                                                                                                                                           \
		CollectionPropertyIterator rna_macro_iter;                                                                                              \
		for (RNA_property_collection_begin(sptr, prop, &rna_macro_iter); rna_macro_iter.valid; RNA_property_collection_next(&rna_macro_iter)) { \
			PointerRNA itemptr = rna_macro_iter.ptr;

#define RNA_PROP_END								  \
		}                                             \
		RNA_property_collection_end(&rna_macro_iter); \
	}												  \
	((void)0)

/** \} */

/* -------------------------------------------------------------------- */
/** \name PointerRNA
 * \{ */

struct StructRNA *RNA_property_pointer_type(struct PointerRNA *ptr, struct PropertyRNA *property);

/** \} */

/* -------------------------------------------------------------------- */
/** \name StructRNA
 * \{ */

struct PropertyRNA *RNA_struct_iterator_property(struct StructRNA *type);

bool RNA_struct_is_ID(const struct StructRNA *type);
bool RNA_struct_is_a(const struct StructRNA *type, const struct StructRNA *srna);

struct PropertyRNA *RNA_struct_find_property(struct PointerRNA *ptr, const char *identifier);
struct PropertyRNA *RNA_struct_type_find_property(struct StructRNA *srna, const char *identifier);

/**
 * Same as `RNA_struct_find_property` but returns `nullptr` if the property type is no same to
 * `property_type_check`.
 */
struct PropertyRNA *RNA_struct_find_property_check(struct PointerRNA *pointer, const char *name, const ePropertyType property_type_check);
/**
 * Same as `RNA_struct_find_property` but returns `nullptr` if the property type is not
 * #PropertyType::PROP_COLLECTION or property struct type is different to `struct_type_check`.
 */
struct PropertyRNA *RNA_struct_find_collection_property_check(struct PointerRNA *pointer, const char *name, const struct StructRNA *struct_type_check);

/**
 * Returns the IDProperty group stored in the given PointerRNA's ID, or NULL if none.
 */
struct IDProperty *RNA_struct_idprops(struct PointerRNA *ptr);

const char *RNA_struct_identifier(const struct StructRNA *type);
const char *RNA_struct_name(const struct StructRNA *type);

bool RNA_struct_property_is_set_ex(struct PointerRNA *ptr, const char *identifier, bool use_ghost);
bool RNA_struct_property_is_set(struct PointerRNA *ptr, const char *identifier);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Pointer Handling
 * \{ */

struct PointerRNA RNA_pointer_create_discrete(struct ID *id, struct StructRNA *type, void *data);
struct PointerRNA RNA_pointer_create_with_parent(const struct PointerRNA *parent, struct StructRNA *type, void *data);
struct PointerRNA RNA_property_pointer_get(struct PointerRNA *ptr, struct PropertyRNA *property);

/** \} */

extern struct RoseRNA ROSE_RNA;

#ifdef __cplusplus
}
#endif

#endif
