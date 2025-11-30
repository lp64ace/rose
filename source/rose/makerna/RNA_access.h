#ifndef RNA_ACCESS_H
#define RNA_ACCESS_H

#include "RNA_types.h"

struct ID;
struct StructRNA;

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


/** \} */

/* -------------------------------------------------------------------- */
/** \name Path API
 * \{ */

/**
 * Resolve the given RNA Path to find both the pointer AND property
 * indicated by fully resolving the path.
 *
 * This is a convenience method to avoid logic errors and ugly syntax.
 * \note Assumes all pointers provided are valid
 * \return True only if both a valid pointer and property are found after resolving the path
 */
bool RNA_path_resolve_property(const struct PointerRNA *ptr, const char *path, struct PointerRNA *r_ptr, struct PropertyRNA **r_property);

/** \} */

/* -------------------------------------------------------------------- */
/** \name PropertyRNA
 * \{ */

ePropertyType RNA_property_type(const struct PropertyRNA *property);

/**
 * A property is animateable if its ID and the RNA property itself are defined as editable.
 * It does not imply that user can _edit_ such animation though, see #RNA_property_anim_editable
 * for this.
 *
 * This check is only based on information stored in the data _types_ (IDTypeInfo and RNA property
 * definition), not on the actual data itself.
 */
bool RNA_property_animateable(const struct PointerRNA *ptr, struct PropertyRNA *property);

/** Returns the length of the array defined by the property. */
int RNA_property_array_length(const struct PointerRNA *ptr, struct PropertyRNA *property);

/**
 * Searches for the specified string key in a collection property.
 * \note If a direct string search function is not implemented we look in the name properties 
 * of the specified collection.
 */
bool RNA_property_collection_lookup_string(struct PointerRNA *ptr, struct PropertyRNA *property, const char *key, PointerRNA *r_ptr);
bool RNA_property_collection_lookup_int(struct PointerRNA *ptr, struct PropertyRNA *property, int key, struct PointerRNA *r_ptr);
bool RNA_property_collection_type_get(struct PointerRNA *ptr, struct PropertyRNA *property, struct PointerRNA *r_ptr);

/** \} */

/* -------------------------------------------------------------------- */
/** \name PropertyRNA Data
 * \{ */

int RNA_property_float_clamp(struct PointerRNA *ptr, struct PropertyRNA *property, float *value);

float RNA_property_float_get(struct PointerRNA *ptr, struct PropertyRNA *property);
void RNA_property_float_set(struct PointerRNA *ptr, struct PropertyRNA *property, float value);
void RNA_property_float_get_array(struct PointerRNA *ptr, struct PropertyRNA *property, float *r_value);
void RNA_property_float_set_array(struct PointerRNA *ptr, struct PropertyRNA *property, float *f_value);

float RNA_property_float_get_index(struct PointerRNA *ptr, struct PropertyRNA *property, int index);
void RNA_property_float_set_index(struct PointerRNA *ptr, struct PropertyRNA *property, int index, float value);

/** \} */

/* -------------------------------------------------------------------- */
/** \name StructRNA
 * \{ */

struct PropertyRNA *RNA_struct_iterator_property(struct StructRNA *type);

bool RNA_struct_is_ID(const struct StructRNA *type);

struct PropertyRNA *RNA_struct_find_property(struct PointerRNA *ptr, const char *identifier);

/**
 * Returns the IDProperty group stored in the given PointerRNA's ID, or NULL if none.
 */
struct IDProperty *RNA_struct_idprops(struct PointerRNA *ptr);

/** \} */

#ifdef __cplusplus
}
#endif

#endif
