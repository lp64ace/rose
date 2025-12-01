#include "MEM_guardedalloc.h"

#include "LIB_string.h"
#include "LIB_vector_set.hh"

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

/* Types */
extern "C" RoseRNA ROSE_RNA;

void RNA_init() {
	ROSE_RNA.srnahash = LIB_ghash_str_new(__func__);
	ROSE_RNA.totsrna = 0;

	LISTBASE_FOREACH(StructRNA *, nstruct, &ROSE_RNA.srnabase) {
		size_t nproperties = LIB_listbase_count(&nstruct->container.properties);

		if (!nstruct->container.property_lookup_set && nproperties >= 8) {
			rose::CustomIDVectorSet<PropertyRNA *, PropertyRNAIdentifierGetter> *ptr = MEM_new<rose::CustomIDVectorSet<PropertyRNA *, PropertyRNAIdentifierGetter>>(__func__);

			LISTBASE_FOREACH(PropertyRNA *, property, &nstruct->container.properties) {
				if (!(property->flag & PROP_INTERN_BUILTIN)) {
					ptr->add(property);
				}
			}

			nstruct->container.property_lookup_set = (void *)ptr;
		}

		LIB_ghash_insert(ROSE_RNA.srnahash, (void *)nstruct->identifier, nstruct);
		ROSE_RNA.totsrna++;
	}
}

void RNA_exit() {
	LISTBASE_FOREACH(StructRNA *, nstruct, &ROSE_RNA.srnabase) {
		rose::CustomIDVectorSet<PropertyRNA *, PropertyRNAIdentifierGetter> *ptr = static_cast<rose::CustomIDVectorSet<PropertyRNA *, PropertyRNAIdentifierGetter> *>(nstruct->container.property_lookup_set);
		if (ptr) {
			MEM_delete<rose::CustomIDVectorSet<PropertyRNA *, PropertyRNAIdentifierGetter>>(ptr);

			nstruct->container.property_lookup_set = NULL;
		}
	}

	LIB_ghash_free(ROSE_RNA.srnahash, NULL, NULL);
}
