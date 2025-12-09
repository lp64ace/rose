#include "MEM_guardedalloc.h"

#include "RNA_define.h"

#include "rna_internal_types.h"
#include "rna_internal.h"

#include "LIB_bitmap.h"
#include "LIB_math_bit.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include <limits.h>
#include <stdio.h>
#include <ctype.h>

#ifdef RNA_RUNTIME
#	include "RNA_prototypes.h"
#endif

#include "intern/genfile.h"

RoseDefRNA DefRNA = {
	.sdna = NULL,
	.structs =
		{
			.first = NULL,
			.last = NULL,
		},
	.nstruct = NULL,
	.silent = 0,
	.error = 0,
	.preprocess = 0,
	.animate = 1,
};

/* -------------------------------------------------------------------- */
/** \name Rose RNA Definition
 * \{ */

RoseRNA *RNA_create(void) {
	RoseRNA *rna = (RoseRNA *)MEM_callocN(sizeof(RoseRNA), "RoseRNA");

	LIB_listbase_clear(&DefRNA.structs);
	rna->srnahash = LIB_ghash_str_new_ex("rose::rna::StructHash", 2048);

	DefRNA.error = false;
	DefRNA.preprocess = true;

	DefRNA.sdna = DNA_sdna_new_current();
	if (DefRNA.sdna == NULL) {
		DefRNA.error = true;
	}

	return rna;
}

void RNA_define_free(RoseRNA *rna) {
	for (StructDefRNA *defstruct = (StructDefRNA *)DefRNA.structs.first; defstruct; defstruct = (StructDefRNA *)(defstruct->container.next)) {
		LIB_freelistN(&defstruct->container.properties);
	}
	LIB_freelistN(&DefRNA.structs);
	for (AllocDefRNA *defalloc = (AllocDefRNA *)DefRNA.allocs.first; defalloc; defalloc = (AllocDefRNA *)(defalloc->next)) {
		MEM_freeN(defalloc->mem);
	}
	LIB_freelistN(&DefRNA.allocs);

	if (DefRNA.sdna) {
		DNA_sdna_free(DefRNA.sdna);
	}

	memset(&DefRNA.sdna, 0, sizeof(RoseDefRNA));
}

void RNA_free(RoseRNA *rna) {
	if (DefRNA.preprocess) {
		RNA_define_free(rna);
		for (StructRNA *itrstruct = (StructRNA *)rna->srnabase.first; itrstruct; itrstruct = itrstruct->container.next) {
			LIB_freelistN(&itrstruct->container.properties);
		}
		LIB_freelistN(&rna->srnabase);
		LIB_ghash_free(rna->srnahash, NULL, NULL);
		MEM_freeN(rna);
	}
	else {
		StructRNA *nextstruct;
		for (StructRNA *itrstruct = (StructRNA *)rna->srnabase.first; itrstruct; itrstruct = nextstruct) {
			nextstruct = (StructRNA *)itrstruct->container.next;
			RNA_struct_free(rna, itrstruct);
		}
		LIB_ghash_free(rna->srnahash, NULL, NULL);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Common RNA Definition
 * \{ */

ROSE_INLINE bool rna_validate_identifier(const char *identifier, bool property, const char **r_error) {
	/* clang-format off */

	static const char *kwlist[] = {
		"and", "as", "assert", "async", "await", "break", "class", "continue", "def",
		"del", "elif", "else", "except", "finally", "for", "from", "global", "if",
		"import", "in", "is", "lambda", "nonlocal", "not", "or", "pass", "raise",
		"return", "try", "while", "with", "yield", NULL
	};

	/* clang-format on */

	if (!isalpha(identifier[0])) {
		*r_error = "first characher failed #isalpha";
		return false;
	}

	for (size_t i = 0; identifier[i]; i++) {
		if (DefRNA.preprocess && property) {
			if (isalpha(identifier[i]) && isupper(identifier[i])) {
				*r_error = "property names must contain lower case characters only";
				return false;
			}
		}

		if (identifier[i] == '_') {
			continue;
		}

		if (identifier[i] == ' ') {
			*r_error = "spaces are not okay in identifier names";
			return false;
		}

		if (isalnum(identifier[i]) == 0) {
			*r_error = "one of the characters failed an #isalnum check and is not an underscore";
			return false;
		}
	}

	for (size_t i = 0; kwlist[i]; i++) {
		if (STREQ(identifier, kwlist[i])) {
			*r_error = "this keyword is reserved by Python";
			return false;
		}
	}

	if (property) {
		/* clang-format off */

		static const char *kwlist_prop[] = {
			/* not keywords but reserved all the same because py uses */
			"keys", "values", "items", "get", NULL,
		};

		/* clang-format on */

		for (size_t i = 0; kwlist_prop[i]; i++) {
			if (STREQ(identifier, kwlist_prop[i])) {
				*r_error = "this keyword is reserved by Python";
				return false;
			}
		}
	}

	return true;
}

ROSE_INLINE void rna_sanitize_identifier(char *identifier, bool property) {
	/* clang-format off */

	static const char *kwlist[] = {
		"and", "as", "assert", "async", "await", "break", "class", "continue", "def",
		"del", "elif", "else", "except", "finally", "for", "from", "global", "if",
		"import", "in", "is", "lambda", "nonlocal", "not", "or", "pass", "raise",
		"return", "try", "while", "with", "yield", NULL
	};

	/* clang-format on */

	if (!isalpha(identifier[0])) {
		identifier[0] = '_';
	}

	for (size_t i = 0; identifier[i]; i++) {
		if (DefRNA.preprocess && property) {
			if (isalpha(identifier[i]) && isupper(identifier[i])) {
				/* property names must contain lower case characters only */
				identifier[i] = tolower(identifier[i]);
			}
		}

		if (identifier[i] == '_') {
			continue;
		}

		if (identifier[i] == ' ') {
			/* spaces are not okay in identifier names */
			identifier[i] = '_';
		}

		if (isalnum(identifier[i]) == 0) {
			/* one of the characters failed an isalnum() check and is not an underscore */
			identifier[i] = '_';
		}
	}

	for (size_t i = 0; kwlist[i]; i++) {
		if (STREQ(identifier, kwlist[i])) {
			/** this keyword is reserved by python, just replace the last character by '_' to keep it readable. */
			identifier[LIB_strlen(identifier) - 1] = '_';
		}
	}

	if (property) {
		/* clang-format off */

		static const char *kwlist_prop[] = {
			/* not keywords but reserved all the same because py uses */
			"keys", "values", "items", "get", NULL,
		};

		/* clang-format on */

		for (size_t i = 0; kwlist_prop[i]; i++) {
			if (STREQ(identifier, kwlist_prop[i])) {
				/**
				 * this keyword is reserved by python, just replace the last character by '_' to keep it readable.
				 */
				identifier[strlen(identifier) - 1] = '_';
			}
		}
	}
}

/* -------------------------------------------------------------------- */
/** \name Struct RNA Definition
 * \{ */

ROSE_INLINE StructDefRNA *rna_find_def_struct(StructRNA *nstruct) {
	if (!DefRNA.preprocess) {
		/* we should never get here */
		fprintf(stderr, "[RNA] #%s is only at preprocess time.\n", __func__);
		return NULL;
	}

	LISTBASE_FOREACH(StructDefRNA *, dstruct, &DefRNA.structs) {
		if (dstruct->ptr == nstruct) {
			return dstruct;
		}
	}
	return NULL;
}

void RNA_def_struct_sdna(StructRNA *srna, const char *structname) {
	StructDefRNA *defstruct;

	if (!DefRNA.preprocess) {
		fprintf(stderr, "[RNA] #%s is available only during preprocessing.\n", __func__);
		return;
	}

	defstruct = rna_find_def_struct(srna);
	defstruct->dnaname = structname;
}

ROSE_STATIC void rna_structs_add(RoseRNA *rna, StructRNA *srna) {
	LIB_addtail(&rna->srnabase, srna);
	rna->totsrna += 1;

	/* This exception is only needed for pre-processing.
	 * otherwise we don't allow empty names. */
	if ((srna->flag & STRUCT_PUBLIC_NAMESPACE) && (srna->identifier[0] != '\0')) {
		LIB_ghash_insert(rna->srnahash, (void *)srna->identifier, srna);
	}
}

StructRNA *RNA_def_struct_ex(RoseRNA *rna, const char *identifier, StructRNA *fromstruct) {
	StructDefRNA *defstruct = NULL, *deffrom = NULL;
	StructRNA *newstruct = MEM_callocN(sizeof(StructRNA), "StructRNA");

	if (DefRNA.preprocess) {
		const char *error = NULL;

		if (!rna_validate_identifier(identifier, false, &error)) {
			DefRNA.error = true;
		}
	}
	DefRNA.nstruct = newstruct;

	if (fromstruct) {
		memcpy(newstruct, fromstruct, sizeof(StructRNA));
		LIB_listbase_clear(&newstruct->container.properties);

		newstruct->base = fromstruct;

		if (DefRNA.preprocess) {
			deffrom = rna_find_def_struct(fromstruct);
		}
		else {
			if (fromstruct->flag & STRUCT_PUBLIC_NAMESPACE_INHERIT) {
				RNA_def_struct_flag(newstruct, STRUCT_PUBLIC_NAMESPACE | STRUCT_PUBLIC_NAMESPACE_INHERIT);
			}
			else {
				RNA_def_struct_clear_flag(newstruct, STRUCT_PUBLIC_NAMESPACE | STRUCT_PUBLIC_NAMESPACE_INHERIT);
			}
		}
	}

	newstruct->identifier = identifier;
	newstruct->name = identifier;
	newstruct->description = "";

	if (!fromstruct) {
		RNA_def_struct_flag(newstruct, STRUCT_UNDO);
	}

	if (DefRNA.preprocess) {
		RNA_def_struct_flag(newstruct, STRUCT_PUBLIC_NAMESPACE);
	}

	rna_structs_add(rna, newstruct);

	if (DefRNA.preprocess) {
		defstruct = MEM_callocN(sizeof(StructDefRNA), "StructDefRNA");
		defstruct->ptr = newstruct;
		LIB_addtail(&DefRNA.structs, defstruct);

		if (deffrom) {
			defstruct->dnafromname = deffrom->dnaname;
		}
	}

	if (DefRNA.preprocess) {
		RNA_def_struct_sdna(newstruct, newstruct->identifier);
	}
	else {
		RNA_def_struct_flag(newstruct, STRUCT_RUNTIME);
	}

	if (fromstruct) {
		newstruct->nameproperty = fromstruct->nameproperty;
		newstruct->iteratorproperty = fromstruct->iteratorproperty;
	}
	else {
		/* Define some builtin properties */
		PropertyRNA *property = RNA_def_property(&newstruct->container, "rna_properties", PROP_COLLECTION, PROP_NONE);
		property->flagex |= PROP_INTERN_BUILTIN;

		RNA_def_property_ui_text(property, "RNA Properties", "RNA property collection");

		if (DefRNA.preprocess) {
			RNA_def_property_struct_type(property, "Property");

			/* clang-format off */

			RNA_def_property_collection_funcs(property,
				"rna_builtin_properties_begin",
				"rna_builtin_properties_next",
				"rna_builtin_properties_end",
				"rna_builtin_properties_get",
				NULL,
				NULL,
				NULL
			);

			/* clang-format on */
		}
		else {
#ifdef RNA_RUNTIME
			CollectionPropertyRNA *cproperty = (CollectionPropertyRNA *)property;
			cproperty->begin = rna_builtin_properties_begin;
			cproperty->next = rna_builtin_properties_next;
			cproperty->get = rna_builtin_properties_get;
			cproperty->element = &RNA_Property;
#endif
		}
	}

	return newstruct;
}

StructRNA *RNA_def_struct(RoseRNA *rna, const char *identifier, const char *from) {
	StructRNA *srnafrom = NULL;

	/* only use RNA_def_struct() while pre-processing, otherwise use RNA_def_struct_ptr() */
	ROSE_assert(DefRNA.preprocess);

	if (from) {
		/* find struct to derive from */
		/* Inline RNA_struct_find(...) because it won't link from here. */
		srnafrom = LIB_ghash_lookup(rna->srnahash, from);
		if (!srnafrom) {
			fprintf(stderr, "[RNA] struct %s not found to define %s.\n", from, identifier);
			DefRNA.error = true;
		}
	}

	return RNA_def_struct_ex(rna, identifier, srnafrom);
}

ROSE_STATIC void RNA_def_struct_free_pointers(RoseRNA *rna, StructRNA *nstruct) {
	if (nstruct->flag & STRUCT_FREE_POINTERS) {
		if (nstruct->identifier) {
			if (nstruct->flag & STRUCT_PUBLIC_NAMESPACE) {
				if (rna != NULL) {
					LIB_ghash_remove(rna->srnahash, (void *)nstruct->identifier, NULL, NULL);
				}
			}
			MEM_freeN((void *)nstruct->identifier);
		}
		if (nstruct->name) {
			MEM_freeN((void *)nstruct->name);
		}
		if (nstruct->description) {
			MEM_freeN((void *)nstruct->description);
		}
	}
}

ROSE_STATIC void RNA_def_property_free_pointers(RoseRNA *rna, PropertyRNA *property) {
	if (property->flagex & PROP_INTERN_FREE_POINTERS) {
		if (property->identifier) {
			MEM_freeN((void *)property->identifier);
		}
		if (property->name) {
			MEM_freeN((void *)property->name);
		}
		if (property->description) {
			MEM_freeN((void *)property->description);
		}

		switch (property->type) {
			case PROP_BOOLEAN: {
				BoolPropertyRNA *bprop = (BoolPropertyRNA *)property;
				if (bprop->defaultarray) {
					MEM_freeN((void *)bprop->defaultarray);
				}
				break;
			} break;
			case PROP_INT: {
				IntPropertyRNA *iprop = (IntPropertyRNA *)property;
				if (iprop->defaultarray) {
					MEM_freeN((void *)iprop->defaultarray);
				}
				break;
			} break;
			case PROP_FLOAT: {
				FloatPropertyRNA *fprop = (FloatPropertyRNA *)property;
				if (fprop->defaultarray) {
					MEM_freeN((void *)fprop->defaultarray);
				}
			} break;
			case PROP_STRING: {
				StringPropertyRNA *sprop = (StringPropertyRNA *)property;
				if (sprop->defaultvalue) {
					MEM_freeN((void *)sprop->defaultvalue);
				}
				break;
			}
		}
	}
}

ROSE_STATIC void rna_structs_remove_and_free(RoseRNA *rna, StructRNA *nstruct) {
	if ((nstruct->flag & STRUCT_PUBLIC_NAMESPACE) && rna->srnahash) {
		if (nstruct->identifier[0] != '\0') {
			LIB_ghash_remove(rna->srnahash, (void *)nstruct->identifier, NULL, NULL);
		}
	}

	RNA_def_struct_free_pointers(NULL, nstruct);

	if (nstruct->flag & STRUCT_RUNTIME) {
		LIB_freelinkN(&rna->srnabase, nstruct);
	}
	rna->totsrna--;
}

void RNA_struct_free(RoseRNA *rna, StructRNA *nstruct) {
	if (nstruct->flag & STRUCT_RUNTIME) {
		PropertyRNA *nextproperty;
		for (PropertyRNA *property = (PropertyRNA *)(nstruct->container.properties.first); property; property = nextproperty) {
			nextproperty = property->next;

			RNA_def_property_free_pointers(rna, property);
		}
	}

	rna_structs_remove_and_free(rna, nstruct);
}

void RNA_def_struct_ui_text(StructRNA *srna, const char *name, const char *description) {
	srna->name = name;
	srna->description = description;
}

void RNA_def_struct_flag(StructRNA *nstruct, int flag) {
	nstruct->flag |= flag;
}

void RNA_def_struct_clear_flag(StructRNA *nstruct, int flag) {
	nstruct->flag &= ~flag;
}

void RNA_def_struct_name_property(StructRNA *srna, PropertyRNA *prop) {
	if (prop->type != PROP_STRING) {
		fprintf(stderr, "[RNA] \"%s.%s\", must be a string property.\n", srna->identifier, prop->identifier);
		DefRNA.error = true;
	}
	else if (srna->nameproperty != NULL) {
		fprintf(stderr, "[RNA] \"%s.%s\", name property is already set.\n", srna->identifier, prop->identifier);
		DefRNA.error = true;
	}
	else {
		srna->nameproperty = prop;
	}
}

void RNA_def_struct_refine_func(StructRNA *srna, const char *func) {
	if (!DefRNA.preprocess) {
		fprintf(stderr, "[RNA] #%s is available only during preprocessing.\n", __func__);
		return;
	}

	if (func) {
		srna->refine = (StructRefineFunc)func;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Property RNA Definition
 * \{ */

const char *RNA_property_typename(ePropertyType type) {
	const char *compileropt[] = {
		[PROP_BOOLEAN] = "PROP_BOOLEAN",
		[PROP_INT] = "PROP_INT",
		[PROP_FLOAT] = "PROP_FLOAT",
		[PROP_STRING] = "PROP_STRING",
		[PROP_ENUM] = "PROP_ENUM",
		[PROP_POINTER] = "PROP_POINTER",
		[PROP_COLLECTION] = "PROP_COLLECTION",
	};
	return compileropt[type];
}

const char *RNA_property_rawtypename(eRawPropertyType type) {
	const char *compileropt[] = {
		[PROP_RAW_BOOLEAN] = "PROP_RAW_BOOLEAN",
		[PROP_RAW_CHAR] = "PROP_RAW_CHAR",
		[PROP_RAW_SHORT] = "PROP_RAW_SHORT",
		[PROP_RAW_INT] = "PROP_RAW_INT",
		[PROP_RAW_FLOAT] = "PROP_RAW_FLOAT",
		[PROP_RAW_DOUBLE] = "PROP_RAW_DOUBLE",
		[PROP_RAW_INT8] = "PROP_RAW_INT8",
		[PROP_RAW_UINT8] = "PROP_RAW_UINT8",
		[PROP_RAW_UINT16] = "PROP_RAW_UINT16",
		[PROP_RAW_INT64] = "PROP_RAW_INT64",
		[PROP_RAW_UINT64] = "PROP_RAW_UINT64",
	};
	return compileropt[type];
}

const char *RNA_property_subtypename(enum ePropertySubType type) {
	/* clang-format off */
	switch (RNA_SUBTYPE_VALUE(type)) {
		case RNA_SUBTYPE_VALUE(PROP_NONE): return "PROP_NONE";
		case RNA_SUBTYPE_VALUE(PROP_UNSIGNED): return "PROP_UNSIGNED";
		case RNA_SUBTYPE_VALUE(PROP_PIXEL): return "PROP_PIXEL";
		case RNA_SUBTYPE_VALUE(PROP_DISTANCE): return "PROP_DISTANCE";
		case RNA_SUBTYPE_VALUE(PROP_FACTOR): return "PROP_FACTOR";
		case RNA_SUBTYPE_VALUE(PROP_COLOR): return "PROP_COLOR";
		case RNA_SUBTYPE_VALUE(PROP_TRANSLATION): return "PROP_TRANSLATION";
		case RNA_SUBTYPE_VALUE(PROP_EULER): return "PROP_EULER";
		case RNA_SUBTYPE_VALUE(PROP_AXISANGLE): return "PROP_AXISANGLE";
		case RNA_SUBTYPE_VALUE(PROP_XYZ): return "PROP_XYZ";
		case RNA_SUBTYPE_VALUE(PROP_XYZ_LENGTH): return "PROP_XYZ_LENGTH";
		case RNA_SUBTYPE_VALUE(PROP_COORDS): return "PROP_COORDS";
	}
	/* clang-format on */

	ROSE_assert_unreachable();
	return "PROP_NONE";
}

ROSE_STATIC size_t rna_property_type_sizeof(int type) {
	/* clang-format off */
	
	switch (type) {
		case PROP_BOOLEAN: return sizeof(BoolPropertyRNA);
		case PROP_INT: return sizeof(IntPropertyRNA);
		case PROP_FLOAT: return sizeof(FloatPropertyRNA);
		case PROP_STRING: return sizeof(StringPropertyRNA);
		case PROP_POINTER: return sizeof(PointerPropertyRNA);
		case PROP_COLLECTION: return sizeof(CollectionPropertyRNA);
	}
	
	/* clang-format on */

	return 0;
}

ROSE_STATIC ContainerDefRNA *rna_find_container(ContainerRNA *container) {
	if (!DefRNA.preprocess) {
		/* we should never get here */
		fprintf(stderr, "[RNA] #%s is only at preprocess time.\n", __func__);
		return NULL;
	}

	StructDefRNA *defstruct = rna_find_def_struct((StructRNA *)container);
	if (defstruct) {
		return &defstruct->container;
	}

	return NULL;
}

ROSE_STATIC PropertyDefRNA *rna_def_property_sdna(PropertyRNA *prop, const char *structname, const char *fieldname) {
	PropertyDefRNA *defproperty = rna_find_struct_property_def(DefRNA.nstruct, prop);
	if (defproperty == NULL) {
		return NULL;
	}

	StructDefRNA *defstruct = rna_find_def_struct((StructRNA *)defproperty->container);

	if (!structname) {
		structname = defstruct->dnaname;
	}
	if (!fieldname) {
		fieldname = prop->identifier;
	}

	const DNAType *vdnatype = DNA_sdna_type(DefRNA.sdna, structname);
	if (!vdnatype || DNA_sdna_type_kind(DefRNA.sdna, vdnatype) != DNA_STRUCT) {
		if (!DefRNA.silent) {
			fprintf(stderr, "[RNA] Compilation error, \"%s\" is not a struct.\n", structname);
			DefRNA.error = true;
		}
		return NULL;
	}

	const DNATypeStruct *dnatype = (const DNATypeStruct *)vdnatype;
	const DNATypeStructField *dnafield = DNA_sdna_struct_field_find(DefRNA.sdna, dnatype, fieldname);
	if (!dnafield) {
		if (!DefRNA.silent) {
			fprintf(stderr, "[RNA] Compilation error, \"%s.%s\" does not exist.\n", structname, fieldname);
			DefRNA.error = true;
		}
		return NULL;
	}

	const DNAType *dnafieldtype = DNA_sdna_struct_field_type(DefRNA.sdna, dnatype, dnafield);
	if (!dnafieldtype) {
		if (!DefRNA.silent) {
			fprintf(stderr, "[RNA] Compilation error, \"%s.%s\" has no type.\n", structname, fieldname);
			DefRNA.error = true;
		}
		return NULL;
	}

	defproperty->dnastructname = structname;
	defproperty->dnaname = fieldname;

	size_t dnaoffset = DNA_sdna_offsetof(DefRNA.sdna, dnatype, dnafield);
	size_t dnasize = DNA_sdna_sizeof(DefRNA.sdna, dnafieldtype);
	size_t level = DNA_sdna_pointer_level(DefRNA.sdna, dnafieldtype);
	size_t length = 1;

	if (DNA_sdna_type_kind(DefRNA.sdna, dnafieldtype) == DNA_ARRAY) {
		length = DNA_sdna_array_length(DefRNA.sdna, (const DNATypeArray *)dnafieldtype);
	}

	if (length > 1) {
		size_t length = DNA_sdna_array_length(DefRNA.sdna, (const DNATypeArray *)dnafieldtype);
		prop->arraylength[0] = length;
		prop->totarraylength = length;
		prop->arraydimension = 1;
	}
	else {
		prop->arraydimension = 0;
		prop->totarraylength = 0;
	}

	defproperty->dnastructname = structname;
	defproperty->dnastructfromname = defstruct->dnafromname;
	defproperty->dnastructfromprop = defstruct->dnafromprop;
	defproperty->dnaname = fieldname;
	defproperty->dnatype = dnafieldtype;
	defproperty->dnaarraylength = length;
	defproperty->dnapointerlevel = level;
	defproperty->dnaoffset = dnaoffset;
	defproperty->dnasize = dnasize;

	return defproperty;
}

/**
 * Several types cannot use all their bytes to store a bit-set (bit-shift operations on negative
 * numbers are "arithmetic", i.e. preserve the sign, i.e. are not "pure" binary shifting).
 *
 * Currently, all signed types and `uint64_t` cannot use their left-most bit (i.e. sign bit).
 */
#define IS_DNATYPE_BOOLEAN_BITSHIFT_FULLRANGE_COMPAT(type) ELEM(DNA_sdna_type_kind(DefRNA.sdna, type), DNA_CHAR, DNA_SHORT, DNA_INT)

ROSE_INLINE void rna_def_property_boolean_sdna(PropertyRNA *property, const char *structname, const char *propname, const int64_t booleanbit, const bool booleannegative, const int length) {
	BoolPropertyRNA *bproperty = (BoolPropertyRNA *)property;
	StructRNA *nstruct = DefRNA.nstruct;

	if (!DefRNA.preprocess) {
		fprintf(stderr, "[RNA] #%s is only available during preprocessing.\n", __func__);
		return;
	}

	if (property->type != PROP_BOOLEAN) {
		fprintf(stderr, "[RNA] \"%s.%s\", type is not boolean.\n", nstruct->identifier, property->identifier);
		DefRNA.error = true;
		return;
	}

	ROSE_assert(length > 0);

	uint bit_index = 0;
	if (length > 1) {
		if (booleanbit <= 0) {
			fprintf(stderr, "%s.%s is using a null or negative 'booleanbit' value of %lld, which is invalid for 'bitset arrays' boolean properties.\n", nstruct->identifier, property->identifier, booleanbit);
			DefRNA.error = true;
			return;
		}

		bit_index = _lib_forward_scan_u64(*(const uint64_t *)(&booleanbit));
		if ((booleanbit & ~(1 << bit_index)) != 0) {
			fprintf(stderr, "[RNA] %s.%s is using a multi-bit 'booleanbit' value of %lld, which is invalid for 'bitset arrays' boolean properties.\n", nstruct->identifier, property->identifier, booleanbit);
			DefRNA.error = true;
			return;
		}
	}

	PropertyDefRNA *defproperty = rna_def_property_sdna(property, structname, propname);
	if (!defproperty) {
		return;
	}

	/* Error check to ensure floats are not wrapped as integers/booleans. */
	if (defproperty->dnatype && IS_DNATYPE_BOOLEAN_COMPAT(defproperty->dnatype) == 0) {
		fprintf(stderr, "[RNA] %s.%s is wrapped as type '%s'.\n", nstruct->identifier, property->identifier, RNA_property_typename(property->type));
		DefRNA.error = true;
		return;
	}

	const bool is_bitset_array = (length > 1);
	if (is_bitset_array) {
		const short max_length = (defproperty->dnasize * 8) - (IS_DNATYPE_BOOLEAN_BITSHIFT_FULLRANGE_COMPAT(defproperty->dnatype) ? 0 : 1);
		if ((bit_index + length) > max_length) {
			fprintf(stderr, "[RNA] %s.%s is wrapped as type '%s' 'bitset array' of %d items starting at bit %u.\n", nstruct->identifier, property->identifier, RNA_property_typename(property->type), length, bit_index);
			DefRNA.error = true;
			return;
		}
		RNA_def_property_array(property, length);
	}

	defproperty->booleanbit = booleanbit;
	defproperty->booleannegative = booleannegative;

	UNUSED_VARS(bproperty);

#if 0

	/* Set the default if possible. */
	if (defproperty->dnaoffset != -1) {
		int SDNAnr = DNA_struct_find_index_wrapper(DefRNA.sdna, defproperty->dnastructname);
		if (SDNAnr != -1) {
			const void *default_data = DNA_default_table[SDNAnr];
			if (default_data) {
				default_data = POINTER_OFFSET(default_data, defproperty->dnaoffset);
				bool has_default = true;
				if (property->totarraylength > 0) {
					fprintf(stderr, "[RNA] %s default: unsupported boolean array default\n", __func__);
					has_default = false;
				}
				else {
					if (STREQ(defproperty->dnatype, "char")) {
						bproperty->defaultvalue = *(const char *)default_data & booleanbit;
					}
					else if (STREQ(defproperty->dnatype, "short")) {
						bproperty->defaultvalue = *(const short *)default_data & booleanbit;
					}
					else if (STREQ(defproperty->dnatype, "int")) {
						bproperty->defaultvalue = *(const int *)default_data & booleanbit;
					}
					else {
						fprintf(stderr, "[RNA] %s default: unsupported boolean type (%s)\n", __func__, defproperty->dnatype);
						has_default = false;
					}

					if (has_default) {
						if (defproperty->booleannegative) {
							bproperty->defaultvalue = !bproperty->defaultvalue;
						}
					}
				}
			}
		}
	}

#endif
}

void RNA_def_property_boolean_sdna(PropertyRNA *prop, const char *structname, const char *propname, int64_t booleanbit) {
	rna_def_property_boolean_sdna(prop, structname, propname, booleanbit, false, 1);
}

ROSE_STATIC bool rna_range_from_int_type(const void *dnatype, int r_range[2]) {
	/* Type `char` is unsigned too. */
	if (ELEM(DNA_sdna_type_kind(DefRNA.sdna, dnatype), DNA_CHAR)) {
		if (DNA_sdna_basic_is_unsigned(DefRNA.sdna, dnatype)) {
			r_range[0] = 0;
			r_range[1] = CHAR_MAX;
		}
		else {
			r_range[0] = CHAR_MIN;
			r_range[1] = CHAR_MAX;
		}
		return true;
	}
	if (ELEM(DNA_sdna_type_kind(DefRNA.sdna, dnatype), DNA_SHORT)) {
		if (DNA_sdna_basic_is_unsigned(DefRNA.sdna, dnatype)) {
			r_range[0] = 0;
			r_range[1] = SHRT_MAX;
		}
		else {
			r_range[0] = SHRT_MIN;
			r_range[1] = SHRT_MAX;
		}
		return true;
	}
	if (ELEM(DNA_sdna_type_kind(DefRNA.sdna, dnatype), DNA_INT)) {
		if (DNA_sdna_basic_is_unsigned(DefRNA.sdna, dnatype)) {
			r_range[0] = 0;
			r_range[1] = INT_MAX;
		}
		else {
			r_range[0] = INT_MIN;
			r_range[1] = INT_MAX;
		}
		return true;
	}
	return false;
}

void RNA_def_property_int_sdna(PropertyRNA *property, const char *structname, const char *propname) {
	PropertyDefRNA *defproperty;
	IntPropertyRNA *iproperty = (IntPropertyRNA *)property;
	StructRNA *nstruct = DefRNA.nstruct;

	if (!DefRNA.preprocess) {
		fprintf(stderr, "[RNA] #%s is only available during preprocessing.\n", __func__);
		return;
	}

	if (property->type != PROP_INT) {
		fprintf(stderr, "[RNA] \"%s.%s\", type is not int.\n", nstruct->identifier, property->identifier);
		DefRNA.error = true;
		return;
	}

	if ((defproperty = rna_def_property_sdna(property, structname, propname))) {
		/* Error check to ensure floats are not wrapped as integers/booleans. */
		if (defproperty->dnatype && IS_DNATYPE_INT_COMPAT(defproperty->dnatype) == 0) {
			fprintf(stderr, "[RNA] %s.%s is wrapped as type '%s'.\n", nstruct->identifier, property->identifier, RNA_property_typename(property->type));
			DefRNA.error = true;
			return;
		}

		/* SDNA doesn't pass us unsigned unfortunately. */
		if (defproperty->dnatype != NULL) {
			int range[2];
			if (rna_range_from_int_type(defproperty->dnatype, range)) {
				iproperty->hardmin = iproperty->softmin = range[0];
				iproperty->hardmax = iproperty->softmax = range[1];
			}
			else {
				fprintf(stderr, "[RNA] \"%s.%s\", range not known.\n", nstruct->identifier, property->identifier);
				DefRNA.error = true;
			}

			/* Rather arbitrary that this is only done for one type. */
			if (STREQ(defproperty->dnatype, "int")) {
				iproperty->softmin = -10000;
				iproperty->softmax = 10000;
			}
		}

		if (ELEM(property->subtype, PROP_UNSIGNED, PROP_FACTOR)) {
			iproperty->hardmin = iproperty->softmin = 0;
		}

#if 0
		/* Set the default if possible. */
		if (defproperty->dnaoffset != -1) {
			int SDNAnr = DNA_struct_find_index_wrapper(DefRNA.sdna, defproperty->dnastructname);
			if (SDNAnr != -1) {
				const void *default_data = DNA_default_table[SDNAnr];
				if (default_data) {
					default_data = POINTER_OFFSET(default_data, defproperty->dnaoffset);
					/* NOTE: Currently doesn't store sign, assume chars are unsigned because
					 * we build with this enabled, otherwise check 'PROP_UNSIGNED'. */
					bool has_default = true;
					if (property->totarraylength > 0) {
						const void *default_data_end = POINTER_OFFSET(default_data, defproperty->dnasize);
						const int size_final = sizeof(int) * property->totarraylength;
						if (STREQ(defproperty->dnatype, "char")) {
							int *defaultarray = (int *)MEM_callocN(size_final, "RNA");
							for (int i = 0; i < property->totarraylength && default_data < default_data_end; i++) {
								defaultarray[i] = *(const char *)default_data;
								default_data = POINTER_OFFSET(default_data, sizeof(char));
							}
							iproperty->defaultarray = defaultarray;
						}
						else if (STREQ(defproperty->dnatype, "short")) {

							int *defaultarray = (int *)MEM_callocN(size_final, "RNA");
							for (int i = 0; i < property->totarraylength && default_data < default_data_end; i++) {
								defaultarray[i] = (property->subtype != PROP_UNSIGNED) ? *(const short *)default_data : *(const ushort *)default_data;
								default_data = POINTER_OFFSET(default_data, sizeof(short));
							}
							iproperty->defaultarray = defaultarray;
						}
						else if (STREQ(defproperty->dnatype, "int")) {
							int *defaultarray = (int *)MEM_callocN(size_final, "RNA");
							memcpy(defaultarray, default_data, ROSE_MIN(size_final, defproperty->dnasize));
							iproperty->defaultarray = defaultarray;
						}
						else {
							has_default = false;
							fprintf(stderr, "[RNA] %s default: unsupported int array type (%s)\n", __func__, defproperty->dnatype);
						}
					}
					else {
						if (STREQ(defproperty->dnatype, "char")) {
							iproperty->defaultvalue = *(const char *)default_data;
						}
						else if (STREQ(defproperty->dnatype, "short")) {
							iproperty->defaultvalue = (property->subtype != PROP_UNSIGNED) ? *(const short *)default_data : *(const ushort *)default_data;
						}
						else if (STREQ(defproperty->dnatype, "int")) {
							iproperty->defaultvalue = (property->subtype != PROP_UNSIGNED) ? *(const int *)default_data : *(const uint *)default_data;
						}
						else {
							has_default = false;
							fprintf(stderr, "[RNA] %s default: unsupported int type (%s)\n", __func__, defproperty->dnatype);
						}
					}
				}
			}
		}
#endif
	}
}

void RNA_def_property_float_sdna(PropertyRNA *property, const char *structname, const char *propname) {
	FloatPropertyRNA *fprop = (FloatPropertyRNA *)property;
	StructRNA *srna = DefRNA.nstruct;

	if (!DefRNA.preprocess) {
		fprintf(stderr, "[RNA] #%s is only available during preprocessing.\n", __func__);
		return;
	}

	if (property->type != PROP_FLOAT) {
		fprintf(stderr, "[RNA] \"%s.%s\", type is not float.\n", srna->identifier, property->identifier);
		DefRNA.error = true;
		return;
	}

	PropertyDefRNA *defproperty;
	if ((defproperty = rna_def_property_sdna(property, structname, propname))) {
		/* silent is for internal use */
		if (defproperty->dnatype && IS_DNATYPE_FLOAT_COMPAT(defproperty->dnatype) == 0) {
			fprintf(stderr, "[RNA] %s.%s is wrapped as type '%s'.\n", srna->identifier, property->identifier, RNA_property_typename(property->type));
			DefRNA.error = true;
			return;
		}

		int dnakind = DNA_sdna_type_kind(DefRNA.sdna, defproperty->dnatype);
		if (defproperty->dnatype && ELEM(dnakind, DNA_CHAR)) {
			fprop->hardmin = fprop->softmin = 0.0f;
			fprop->hardmax = fprop->softmax = 1.0f;
		}

#if 0
		/* Set the default if possible. */
		if (defproperty->dnaoffset != -1) {
			int SDNAnr = DNA_struct_find_index_wrapper(DefRNA.sdna, defproperty->dnastructname);
			if (SDNAnr != -1) {
				const void *default_data = DNA_default_table[SDNAnr];
				if (default_data) {
					default_data = POINTER_OFFSET(default_data, defproperty->dnaoffset);
					bool has_default = true;
					if (property->totarraylength > 0) {
						if (STREQ(defproperty->dnatype, "float")) {
							const int size_final = sizeof(float) * property->totarraylength;
							float *defaultarray = (float *)MEM_callocN(size_final, "RNA");
							memcpy(defaultarray, default_data, ROSE_MIN(size_final, defproperty->dnasize));
							fprop->defaultarray = defaultarray;
						}
						else {
							has_default = false;
							fprintf(stderr, "[RNA] %s default: unsupported float array type (%s)\n", __func__, defproperty->dnatype);
						}
					}
					else {
						if (ELEM(dnakind, DNA_FLOAT)) {
							fprop->defaultvalue = *(const float *)default_data;
						}
						else if (ELEM(dnakind, DNA_CHAR)) {
							fprop->defaultvalue = ((float)(*(const char *)default_data)) * (1.0f / 255.0f);
						}
						else {
							has_default = false;
							fprintf(stderr, "[RNA] %s default: unsupported float type (%s)\n", __func__, dp->dnatype);
						}
					}
				}
			}
		}
#endif
	}

	rna_def_property_sdna(property, structname, propname);
}

void RNA_def_property_string_sdna(PropertyRNA *property, const char *structname, const char *propname) {
	StringPropertyRNA *sproperty = (StringPropertyRNA *)property;
	StructRNA *srna = DefRNA.nstruct;

	if (!DefRNA.preprocess) {
		fprintf(stderr, "[RNA] #%s is only available during preprocessing.", __func__);
		return;
	}

	if (property->type != PROP_STRING) {
		fprintf(stderr, "[RNA] \"%s.%s\", type is not string.", srna->identifier, property->identifier);
		DefRNA.error = true;
		return;
	}

	PropertyDefRNA *defproperty;
	if ((defproperty = rna_def_property_sdna(property, structname, propname))) {
		if (property->arraydimension) {
			sproperty->maxlength = property->totarraylength;
			property->arraydimension = 0;
			property->totarraylength = 0;
		}

#if 0
		/* Set the default if possible. */
		if ((defproperty->dnaoffset != -1) && (defproperty->dnapointerlevel != 0)) {
			int SDNAnr = DNA_struct_find_index_wrapper(DefRNA.sdna, defproperty->dnastructname);
			if (SDNAnr != -1) {
				const void *default_data = DNA_default_table[SDNAnr];
				if (default_data) {
					default_data = POINTER_OFFSET(default_data, defproperty->dnaoffset);
					sprop->defaultvalue = static_cast<const char *>(default_data);
				}
			}
		}
#endif
	}
}

void RNA_def_property_pointer_sdna(PropertyRNA *prop, const char *structname, const char *propname) {
	StructRNA *srna = DefRNA.nstruct;

	if (!DefRNA.preprocess) {
		fprintf(stderr, "[RNA] #%s is only available during preprocessing.", __func__);
		return;
	}

	if (prop->type != PROP_POINTER) {
		fprintf(stderr, "[RNA] \"%s.%s\", type is not pointer.", srna->identifier, prop->identifier);
		DefRNA.error = true;
		return;
	}

	if (rna_def_property_sdna(prop, structname, propname)) {
		if (prop->arraydimension) {
			prop->arraydimension = 0;
			prop->totarraylength = 0;

			fprintf(stderr, "[RNA] \"%s.%s\", array not supported for pointer type.\n", structname, propname);
			DefRNA.error = true;
		}
	}
}

void RNA_def_property_struct_type(PropertyRNA *prop, const char *type) {
	StructRNA *srna = DefRNA.nstruct;

	if (!DefRNA.preprocess) {
		fprintf(stderr, "[RNA] #%s is only available during preprocessing.", __func__);
		return;
	}

	switch (prop->type) {
		case PROP_POINTER: {
			PointerPropertyRNA *pprop = (PointerPropertyRNA *)prop;
			pprop->srna = (StructRNA *)type;
			break;
		}
		case PROP_COLLECTION: {
			CollectionPropertyRNA *cprop = (CollectionPropertyRNA *)prop;
			cprop->element = (StructRNA *)type;
			break;
		}
		default:
			fprintf(stderr, "[RNA] \"%s.%s\", invalid type for struct type.", srna->identifier, prop->identifier);
			DefRNA.error = true;
			break;
	}
}

void RNA_def_property_collection_sdna(PropertyRNA *property, const char *structname, const char *propname, const char *lengthpropname) {
	CollectionPropertyRNA *cproperty = (CollectionPropertyRNA *)property;
	StructRNA *srna = DefRNA.nstruct;

	if (!DefRNA.preprocess) {
		fprintf(stderr, "[RNA] #%s is only available during preprocessing.", __func__);
		return;
	}

	PropertyDefRNA *defproperty;
	if ((defproperty = rna_def_property_sdna(property, structname, propname))) {
		if (property->arraydimension) {
			property->arraydimension = 0;
			property->totarraylength = 0;

			fprintf(stderr, "[RNA] \"%s.%s\", array not supported for collection type.\n", structname, propname);
			DefRNA.error = true;
		}
		
		if (defproperty->dnatype && DNA_sdna_type_kind(DefRNA.sdna, defproperty->dnatype) == DNA_STRUCT) {
			if (STREQ(DNA_sdna_struct_identifier(DefRNA.sdna, defproperty->dnatype), "ListBase")) {
				cproperty->next = (PropCollectionNextFunc)"rna_iterator_listbase_next";
				cproperty->get = (PropCollectionGetFunc)"rna_iterator_listbase_get";
				cproperty->end = (PropCollectionEndFunc)"rna_iterator_listbase_end";
			}
		}
	}

	if (defproperty && lengthpropname) {
		/**
		 * Not implemented yet, we need to define array iteration here!
		 */
		ROSE_assert_unreachable();
	}
}

ROSE_INLINE PropertyDefRNA *rna_def_findlink(ListBase *lb, const char *identifier) {
	LISTBASE_FOREACH(PropertyDefRNA *, link, lb) {
		if (link->ptr && STREQ(link->ptr->identifier, identifier)) {
			return link;
		}
	}
	return NULL;
}

PropertyRNA *RNA_def_property(void *vcontainer, const char *identifier, int type, int subtype) {
	ContainerRNA *container = (ContainerRNA *)vcontainer;

	ContainerDefRNA *defcontainer = NULL;
	PropertyDefRNA *defproperty = NULL;

	if (DefRNA.preprocess) {
		const char *error = NULL;

		if (!rna_validate_identifier(identifier, true, &error)) {
			fprintf(stderr, "[RNA] Property identifier \"%s.%s\" - %s\n", CONTAINER_RNA_ID(container), identifier, error);
			DefRNA.error = true;
		}

		defcontainer = rna_find_container(container);

		if (rna_def_findlink(&defcontainer->properties, identifier)) {
			fprintf(stderr, "[RNA] Duplicate identifier \"%s.%s\"\n", CONTAINER_RNA_ID(container), identifier);
			DefRNA.error = true;
		}

		defproperty = MEM_callocN(sizeof(PropertyDefRNA), "PropertyDefRNA");
		LIB_addtail(&defcontainer->properties, defproperty);
	}
	else {
		const char *error = NULL;
		if (!rna_validate_identifier(identifier, true, &error)) {
			fprintf(stderr, "[RNA] Runtime property identifier \"%s.%s\" - %s\n", CONTAINER_RNA_ID(container), identifier, error);
			DefRNA.error = true;
		}
	}

	PropertyRNA *property = MEM_callocN(rna_property_type_sizeof(type), "PropertyRNA");

	switch (type) {
		case PROP_BOOLEAN: {
			if (DefRNA.preprocess) {
			}
		} break;
		case PROP_INT: {
			if (DefRNA.preprocess) {
				IntPropertyRNA *iproperty = (IntPropertyRNA *)property;

				if (subtype == PROP_DISTANCE) {
					fprintf(stderr, "[RNA] Subtype does not apply to 'PROP_INT' \"%s.%s\"\n", CONTAINER_RNA_ID(container), identifier);
					DefRNA.error = true;
				}

				iproperty->hardmin = (subtype == PROP_UNSIGNED) ? 0 : INT_MIN;
				iproperty->hardmax = INT_MAX;
				iproperty->softmin = (subtype == PROP_UNSIGNED) ? 0 : -10000;
				iproperty->hardmax = +10000;
				iproperty->step = 1;
			}
		} break;
		case PROP_FLOAT: {
			if (DefRNA.preprocess) {
				FloatPropertyRNA *fproperty = (FloatPropertyRNA *)property;

				fproperty->hardmin = (subtype == PROP_UNSIGNED) ? 0.0f : -FLT_MAX;
				fproperty->hardmax = FLT_MAX;

				if (subtype == PROP_COLOR) {
					fproperty->softmin = fproperty->hardmin = 0.0f;
					fproperty->softmax = 1.0f;
				}
				else if (subtype == PROP_FACTOR) {
					fproperty->softmin = fproperty->hardmin = 0.0f;
					fproperty->softmax = fproperty->hardmax = 1.0f;
				}
				else {
					/* rather arbitrary. */
					fproperty->softmin = (subtype == PROP_UNSIGNED) ? 0.0f : -10000.0f;
					fproperty->softmax = 10000.0f;
				}
				fproperty->step = 10;
				fproperty->precision = 3;
			}
		} break;
		case PROP_STRING: {
			StringPropertyRNA *sproperty = (StringPropertyRNA *)property;

			/* By default don't allow nullptr string args, callers may clear.
			 * Used so generated 'get/length/set' functions skip a nullptr check
			 * in some cases we want it */
			RNA_def_property_flag(property, PROP_NEVER_NULL);
			sproperty->defaultvalue = "";
		} break;
		case PROP_POINTER: {
			PointerPropertyRNA *pproperty = (PointerPropertyRNA *)property;

		} break;
		case PROP_COLLECTION: {
			CollectionPropertyRNA *cproperty = (CollectionPropertyRNA *)property;

		} break;
		default: {
			fprintf(stderr, "[RNA] \"%s.%s\", invalid property type.\n", CONTAINER_RNA_ID(container), identifier);
			DefRNA.error = true;
			return NULL;
		} break;
	}

	if (DefRNA.preprocess) {
		defproperty->container = container;
		defproperty->ptr = property;
	}

	property->magic = RNA_MAGIC;
	property->identifier = identifier;
	property->type = type;
	property->subtype = subtype;
	property->name = identifier;
	property->description = "";
	property->rawtype = -1;

	if (!ELEM(type, PROP_COLLECTION, PROP_POINTER)) {
		RNA_def_property_flag(property, PROP_EDITABLE);

		if (type != PROP_STRING) {
			if (DefRNA.animate) {
				RNA_def_property_flag(property, PROP_ANIMATABLE);
			}
		}
	}

	if (DefRNA.preprocess) {
		switch (type) {
			case PROP_BOOLEAN: {
				DefRNA.silent = 1;
				RNA_def_property_boolean_sdna(property, NULL, identifier, 0);
				DefRNA.silent = 0;
			} break;
			case PROP_INT: {
				DefRNA.silent = 1;
				RNA_def_property_int_sdna(property, NULL, identifier);
				DefRNA.silent = 0;
			} break;
			case PROP_FLOAT: {
				DefRNA.silent = 1;
				RNA_def_property_float_sdna(property, NULL, identifier);
				DefRNA.silent = 0;
			} break;
			case PROP_STRING: {
				DefRNA.silent = 1;
				RNA_def_property_string_sdna(property, NULL, identifier);
				DefRNA.silent = 0;
			} break;
			case PROP_POINTER: {
				DefRNA.silent = 1;
				RNA_def_property_pointer_sdna(property, NULL, identifier);
				DefRNA.silent = 0;
			} break;
			case PROP_COLLECTION: {
				DefRNA.silent = 1;
				RNA_def_property_collection_sdna(property, NULL, identifier, NULL);
				DefRNA.silent = 0;
			} break;
		}
	}
	else {
		RNA_def_property_flag(property, PROP_IDPROPERTY);

		/**
		 * Flag that this property was not defined during preprocessing but during runtime instead.
		 */
		property->flagex |= PROP_INTERN_RUNTIME;
	}

	LIB_addtail(&container->properties, property);

	return property;
}

void RNA_def_property_ui_text(PropertyRNA *prop, const char *name, const char *description) {
	prop->name = name;
	prop->description = description;
}

void RNA_def_property_ui_range(PropertyRNA *prop, double min, double max, double step, int precision) {
	StructRNA *srna = DefRNA.nstruct;

	switch (prop->type) {
		case PROP_INT: {
			IntPropertyRNA *iproperty = (IntPropertyRNA *)prop;
			iproperty->softmin = (int)min;
			iproperty->softmax = (int)max;
			iproperty->step = (int)step;
		} break;
		case PROP_FLOAT: {
			FloatPropertyRNA *fproperty = (FloatPropertyRNA *)prop;
			fproperty->softmin = (float)min;
			fproperty->softmax = (float)max;
			fproperty->step = (float)step;
			fproperty->precision = precision;
		} break;
		default: {
			fprintf(stderr, "\"%s.%s\", invalid type for ui range.", srna->identifier, prop->identifier);
			DefRNA.error = true;
		} break;
	}
}

void RNA_def_property_flag(PropertyRNA *property, ePropertyFlag flag) {
	property->flag |= flag;
}

void RNA_def_property_clear_flag(PropertyRNA *property, ePropertyFlag flag) {
	property->flag &= ~flag;
}

void RNA_def_property_array(PropertyRNA *prop, int length) {
	StructRNA *srna = DefRNA.nstruct;

	if (length < 0) {
		fprintf(stderr, "[RNA] \"%s.%s\", array length must be zero of greater.\n", srna->identifier, prop->identifier);
		DefRNA.error = true;
		return;
	}

	if (length > RNA_MAX_ARRAY_LENGTH) {
		fprintf(stderr, "[RNA] \"%s.%s\", array length must be smaller than %d.\n", srna->identifier, prop->identifier, RNA_MAX_ARRAY_LENGTH);
		DefRNA.error = true;
		return;
	}

	if (prop->arraydimension > 1) {
		fprintf(stderr, "[RNA] \"%s.%s\", array dimensions has been set to %u but would be overwritten as 1.\n", srna->identifier, prop->identifier, prop->arraydimension);
		DefRNA.error = true;
		return;
	}

	/**
	 * For boolean arrays using bitflags, ensure that the DNA member is an array, and not a scalar
	 * value.
	 *
	 * NOTE: when using #RNA_def_property_boolean_bitset_array_sdna, #RNA_def_property_array will be
	 * called _before_ defining #dp->booleanbit, so this check won't be triggered. */
	if (DefRNA.preprocess && prop->type == PROP_BOOLEAN) {
		PropertyDefRNA *dp = rna_find_struct_property_def(DefRNA.nstruct, prop);
		if (dp && dp->booleanbit && dp->dnaarraylength < length) {
			fprintf(stderr, "[RNA] \"%s.%s\", cannot define a bitflags boolean array wrapping a scalar DNA member. #RNA_def_property_boolean_bitset_array_sdna should be used instead.\n", srna->identifier, prop->identifier);
			DefRNA.error = true;
			return;
		}
	}

	switch (prop->type) {
		case PROP_BOOLEAN:
		case PROP_INT:
		case PROP_FLOAT:
			prop->arraylength[0] = length;
			prop->totarraylength = length;
			prop->arraydimension = 1;
			break;
		default:
			fprintf(stderr, "[RNA] \"%s.%s\", only boolean/int/float can be array.\n", srna->identifier, prop->identifier);
			DefRNA.error = true;
			break;
	}
}

void RNA_def_property_string_funcs(PropertyRNA *property, const char *get, const char *length, const char *set) {
	StructRNA *srna = DefRNA.nstruct;

	if (!DefRNA.preprocess) {
		fprintf(stderr, "[RNA] #%s is only available during preprocessing.", __func__);
		return;
	}

	switch (property->type) {
		case PROP_STRING: {
			StringPropertyRNA *sproperty = (StringPropertyRNA *)property;

			if (get) {
				sproperty->get = (PropStringGetFunc)get;
			}
			if (length) {
				sproperty->length = (PropStringLengthFunc)length;
			}
			if (set) {
				sproperty->set = (PropStringSetFunc)set;
			}
		} break;
		default: {
			fprintf(stderr, "\"%s.%s\", type is not string.", srna->identifier, property->identifier);
			DefRNA.error = true;
		} break;
	}
}

void RNA_def_property_collection_funcs(PropertyRNA *property, const char *begin, const char *next, const char *end, const char *get, const char *length, const char *lookupint, const char *lookupstring) {
	StructRNA *srna = DefRNA.nstruct;

	if (!DefRNA.preprocess) {
		fprintf(stderr, "[RNA] #%s is only available during preprocessing.", __func__);
		return;
	}

	switch (property->type) {
		case PROP_COLLECTION: {
			CollectionPropertyRNA *cproperty = (CollectionPropertyRNA *)property;

			if (begin) {
				cproperty->begin = (PropCollectionBeginFunc)begin;
			}
			if (next) {
				cproperty->next = (PropCollectionNextFunc)next;
			}
			if (end) {
				cproperty->end = (PropCollectionEndFunc)end;
			}
			if (get) {
				cproperty->get = (PropCollectionGetFunc)get;
			}
			if (length) {
				cproperty->length = (PropCollectionLengthFunc)length;
			}
			if (lookupint) {
				cproperty->lookupint = (PropCollectionLookupIntFunc)lookupint;
			}
			if (lookupstring) {
				cproperty->lookupstring = (PropCollectionLookupStringFunc)lookupstring;
			}
		} break;
		default: {
			fprintf(stderr, "\"%s.%s\", type is not collection.", srna->identifier, property->identifier);
			DefRNA.error = true;
		} break;
	}
}

void RNA_def_property_float_array_default(PropertyRNA *property, const float *array) {
	StructRNA *srna = DefRNA.nstruct;

	switch (property->type) {
		case PROP_FLOAT: {
			FloatPropertyRNA *fproperty = (FloatPropertyRNA *)property;
#ifndef RNA_RUNTIME
			if (fproperty->defaultarray != NULL) {
				fprintf(stderr, "[RNA] Warning \"%s.%s\", set from DNA.", srna->identifier, property->identifier);
			}
#endif
			fproperty->defaultarray = array; /* WARNING, this array must not come from the stack and lost */
			break;
		}
		default:
			fprintf(stderr, "[RNA] \"%s.%s\", type is not float.\n", srna->identifier, property->identifier);
			DefRNA.error = true;
			break;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Property RNA Definition Functions
 * \{ */

PropertyDefRNA *rna_find_struct_property_def(StructRNA *nstruct, PropertyRNA *property) {
	if (!DefRNA.preprocess) {
		/* we should never get here */
		fprintf(stderr, "[RNA] #%s is only available at preprocess time.\n", __func__);
		return NULL;
	}

	StructDefRNA *defstruct = rna_find_def_struct(nstruct);
	PropertyDefRNA *defproperty = (PropertyDefRNA *)(defstruct->container.properties.last);
	for (; defproperty; defproperty = defproperty->prev) {
		if (defproperty->ptr == property) {
			return defproperty;
		}
	}

	defstruct = (StructDefRNA *)DefRNA.structs.last;
	for (; defstruct; defstruct = (StructDefRNA *)defstruct->container.prev) {
		defproperty = (PropertyDefRNA *)defstruct->container.properties.last;
		for (; defproperty; defproperty = defproperty->prev) {
			if (defproperty->ptr == property) {
				return defproperty;
			}
		}
	}

	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Property RNA Definition
 * \{ */

void RNA_def_property_string_maxlength(PropertyRNA *property, int maxlength) {
	StructRNA *nstruct = DefRNA.nstruct;

	switch (property->type) {
		case PROP_STRING: {
			StringPropertyRNA *sprop = (StringPropertyRNA *)property;
			sprop->maxlength = maxlength;
			break;
		}
		default:
			fprintf(stderr, "[RNA] \"%s.%s\", type is not string.\n", nstruct->identifier, property->identifier);
			DefRNA.error = true;
			break;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Common Values
 * \{ */

const float rna_default_quaternion[4] = {1, 0, 0, 0};
const float rna_default_axis_angle[4] = {0, 0, 1, 0};
const float rna_default_scale_3d[3] = {1, 1, 1};

/** \} */
