#ifndef RNA_DEFINE_INLINE_C
#define RNA_DEFINE_INLINE_C

#include "RNA_define.h"

#include "intern/genfile.h"

#include "rna_internal_types.h"
#include "rna_internal.h"

extern RoseDefRNA DefRNA;

#ifdef __cplusplus
extern "C" {
#endif

ROSE_INLINE bool IS_DNATYPE_FLOAT_COMPAT(const struct DNAType *type) {
	if (ELEM(DNA_sdna_type_kind(DefRNA.sdna, type), DNA_FLOAT, DNA_DOUBLE, DNA_CHAR)) {
		return true;
	}

	if (ELEM(DNA_sdna_type_kind(DefRNA.sdna, type), DNA_ARRAY)) {
		const  struct DNAType *element = DNA_sdna_array_element(DefRNA.sdna, (const struct DNATypeArray *)type);
		return IS_DNATYPE_FLOAT_COMPAT(element);
	}

	return false;
}

ROSE_INLINE bool IS_DNATYPE_INT_COMPAT(const struct DNAType *type) {
	if (ELEM(DNA_sdna_type_kind(DefRNA.sdna, type), DNA_INT, DNA_SHORT, DNA_CHAR)) {
		return true;
	}
	if (ELEM(DNA_sdna_type_kind(DefRNA.sdna, type), DNA_ARRAY)) {
		const struct DNAType *element = DNA_sdna_array_element(DefRNA.sdna, (const struct DNATypeArray *)type);
		return IS_DNATYPE_INT_COMPAT(element);
	}
	return false;
}

ROSE_INLINE bool IS_DNATYPE_BOOLEAN_COMPAT(const struct DNAType *type) {
	if (ELEM(DNA_sdna_type_kind(DefRNA.sdna, type), DNA_LLONG) || IS_DNATYPE_INT_COMPAT(type)) {
		return true;
	}
	if (ELEM(DNA_sdna_type_kind(DefRNA.sdna, type), DNA_ARRAY)) {
		const  struct DNAType *element = DNA_sdna_array_element(DefRNA.sdna, (const struct DNATypeArray *)type);
		return IS_DNATYPE_BOOLEAN_COMPAT(element);
	}
	return false;
}

#ifdef __cplusplus
}
#endif

#endif // RNA_DEFINE_INLINE_C
