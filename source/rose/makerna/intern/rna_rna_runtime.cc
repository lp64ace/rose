#include "DNA_action_types.h"

#include "LIB_vector_set.hh"

#include "RNA_access.h"
#include "RNA_define.h"
#include "RNA_prototypes.h"

#include "rna_internal.h"
#include "rna_internal_types.h"

/* -------------------------------------------------------------------- */
/** \name Exposed Functions
 * \{ */

#ifdef RNA_USE_CANONICAL_PATH
ROSE_INLINE bool rna_builtin_properties_key_is_canonical(const char *key) {
	return key[0] == '&';
}

ROSE_INLINE unsigned int rna_builtin_properties_key_as_canonical(const char *key) {
	return *(const unsigned int *)POINTER_OFFSET(key, 1);
}
#endif

bool rna_builtin_properties_lookup_string(PointerRNA *ptr, const char *key, PointerRNA *r_ptr) {
#ifdef RNA_USE_CANONICAL_PATH
	const bool is_canonical = rna_builtin_properties_key_is_canonical(key);
	const unsigned int key_canonical = rna_builtin_properties_key_as_canonical(key);
#endif

	StructRNA *nstruct = ptr->type;
	do {
#ifdef RNA_USE_CANONICAL_PATH
		if (is_canonical && nstruct->container.property_canonical_lookup_set) {
			rose::CustomIDVectorSet<PropertyRNA *, PropertyRNACanonicalGetter> *ptr = static_cast<rose::CustomIDVectorSet<PropertyRNA *, PropertyRNACanonicalGetter> *>(nstruct->container.property_canonical_lookup_set);

			PropertyRNA *const *lookup_property = ptr->lookup_key_ptr_as(key_canonical);
			PropertyRNA *property;

			property = lookup_property ? *lookup_property : NULL;

			if (property) {
				*r_ptr = {
					NULL,
					&RNA_Property,
					property,
				};
				return true;
			}

			continue;
		}
#endif

		if (nstruct->container.property_identifier_lookup_set) {
			rose::CustomIDVectorSet<PropertyRNA *, PropertyRNAIdentifierGetter> *ptr = static_cast<rose::CustomIDVectorSet<PropertyRNA *, PropertyRNAIdentifierGetter> *>(nstruct->container.property_identifier_lookup_set);

			PropertyRNA *const *lookup_property = ptr->lookup_key_ptr_as(key);
			PropertyRNA *property;

			property = lookup_property ? *lookup_property : NULL;

			if (property) {
				*r_ptr = {
					NULL,
					&RNA_Property,
					property,
				};
				return true;
			}
		}
		else {
			LISTBASE_FOREACH(PropertyRNA *, property, &nstruct->container.properties) {
				if ((property->flagex & PROP_INTERN_BUILTIN)) {
					continue;
				}

				if (STREQ(property->name, key)) {
					*r_ptr = {
						NULL,
						&RNA_Property,
						property,
					};
					return true;
				}
			}
		}
	} while ((nstruct = nstruct->base) != NULL);

	*r_ptr = PointerRNA_NULL;
	return false;
}

/** \} */
