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

bool rna_builtin_properties_lookup_string(PointerRNA *ptr, const char *key, PointerRNA *r_ptr) {
	StructRNA *nstruct = ptr->type;
	do {
		if (nstruct->container.property_lookup_set) {
			rose::CustomIDVectorSet<PropertyRNA *, PropertyRNAIdentifierGetter> *ptr = static_cast<rose::CustomIDVectorSet<PropertyRNA *, PropertyRNAIdentifierGetter> *>(nstruct->container.property_lookup_set);

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
				if ((property->flagex & PROP_INTERN_BUILTIN) != 0) {
					continue;
				}

				if (STREQ(property->identifier, key)) {
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
