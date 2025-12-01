#include "DNA_action_types.h"

#include "KER_action.h"
#include "KER_idprop.h"

#include "RNA_access.h"
#include "RNA_define.h"
#include "RNA_prototypes.h"

#include "rna_internal.h"
#include "rna_internal_types.h"

/* -------------------------------------------------------------------- */
/** \name Exposed Functions
 * \{ */

ROSE_INLINE bool rna_property_builtin(CollectionPropertyIterator *iter, void *data) {
	PropertyRNA *prop = (PropertyRNA *)data;

	/* function to skip builtin rna properties */

	return (prop->flagex & PROP_INTERN_BUILTIN) != 0;
}

ROSE_INLINE bool rna_idproperty_known(CollectionPropertyIterator *iter, void *data) {
	IDProperty *idprop = (IDProperty *)data;
	PropertyRNA *prop;
	StructRNA *ptype = iter->builtin_parent.type;

	/* Function to skip any id properties that are already known by RNA,
	 * for the second loop where we go over unknown id properties.
	 *
	 * Note that only dynamically-defined RNA properties (the ones actually using IDProperties as
	 * storage back-end) should be checked here. If a custom property is named the same as a 'normal'
	 * RNA property, they are different data. */
	do {
		for (prop = (PropertyRNA *)(ptype->container.properties.first); prop; prop = prop->next) {
			if ((prop->flagex & PROP_INTERN_BUILTIN) == 0 && (prop->flag & PROP_IDPROPERTY) != 0 && STREQ(prop->identifier, idprop->name)) {
				return true;
			}
		}
	} while ((ptype = ptype->base));

	return false;
}

ROSE_INLINE void rna_inheritance_next_level_restart(CollectionPropertyIterator *iter, PointerRNA *ptr, IteratorSkipFunc skip) {
	/* RNA struct inheritance */
	while (!iter->valid && iter->level > 0) {
		StructRNA *srna = (StructRNA *)iter->parent.data;
		iter->level--;
		for (unsigned int i = iter->level; i > 0; i--) {
			srna = srna->base;
		}

		rna_iterator_listbase_end(iter);

		rna_iterator_listbase_begin(iter, ptr, &srna->container.properties, skip);
	}
}

ROSE_INLINE void rna_inheritance_properties_listbase_begin(CollectionPropertyIterator *iter, PointerRNA *ptr, ListBase *lb, IteratorSkipFunc skip) {
	rna_iterator_listbase_begin(iter, ptr, lb, skip);
	rna_inheritance_next_level_restart(iter, ptr, skip);
}

ROSE_INLINE void rna_inheritance_properties_listbase_next(CollectionPropertyIterator *iter, IteratorSkipFunc skip) {
	rna_iterator_listbase_next(iter);
	rna_inheritance_next_level_restart(iter, &iter->parent, skip);
}

void rna_Struct_properties_begin(CollectionPropertyIterator *iter, PointerRNA *ptr) {
	StructRNA *srna;

	/* here ptr->data should always be the same as iter->parent.type */
	srna = (StructRNA *)ptr->data;

	while (srna->base) {
		iter->level++;
		srna = srna->base;
	}

	rna_inheritance_properties_listbase_begin(iter, ptr, &srna->container.properties, rna_property_builtin);
}

void rna_Struct_properties_next(CollectionPropertyIterator *iter) {
	ListBaseIterator *internal = &iter->internal.listbase;
	IDProperty *group;

	if (internal->flag) {
		/* id properties */
		rna_iterator_listbase_next(iter);
	}
	else {
		/* regular properties */
		rna_inheritance_properties_listbase_next(iter, rna_property_builtin);

		/* Try IDProperties (i.e. custom data).
		 *
		 * NOTE: System IDProperties should not need to be handled here, as they are expected to have a
		 * valid (runtime-defined) RNA property to wrap them, which will have been processed above as
		 * part of `rna_inheritance_properties_listbase_next`. */
		if (!iter->valid) {
			group = RNA_struct_idprops(&iter->builtin_parent);

			if (group) {
				rna_iterator_listbase_end(iter);
				rna_iterator_listbase_begin(iter, &iter->parent, &group->data.group, rna_idproperty_known);
				internal = &iter->internal.listbase;
				internal->flag = 1;
			}
		}
	}
}

struct PointerRNA rna_Struct_properties_get(CollectionPropertyIterator *iter) {
	ListBaseIterator *internal = &iter->internal.listbase;

	/**
	 * we return either PropertyRNA* or IDProperty*, the rna_access.c
	 * functions can handle both as PropertyRNA* with some tricks
	 */
	return RNA_pointer_create_discrete(NULL, &RNA_Property, internal->link);
}

void rna_Property_name_get(PointerRNA *ptr, char *value) {
	PropertyRNA *data = (PropertyRNA *)ptr->data;
	strcpy(value, data->name ? data->name : "");
}

int rna_Property_name_length(PointerRNA *ptr) {
	PropertyRNA *data = (PropertyRNA *)ptr->data;
	return strlen(data->name);
}

/** \} */
