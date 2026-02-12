#ifndef RNA_INTERNAL_H
#define RNA_INTERNAL_H

#include "rna_internal_types.h"

#define RNA_MAGIC ((int)~0)

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Struct RNA Definition Structures
 * \{ */

typedef struct ContainerDefRNA {
	void *prev, *next;

	struct ContainerRNA *ptr;
	struct ListBase properties;
} ContainerDefRNA;

typedef struct StructDefRNA {
	struct ContainerDefRNA container;

	struct StructRNA *ptr;

	const char *filename;
	const char *dnaname;

	const char *dnafromname;
	const char *dnafromprop;
} StructDefRNA;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Property RNA Definition Structures
 * \{ */

typedef struct PropertyDefRNA {
	struct PropertyDefRNA *prev, *next;

	struct ContainerRNA *container;
	struct PropertyRNA *ptr;

	/* struct */
	const char *dnastructname;
	const char *dnastructfromname;
	const char *dnastructfromprop;

	/* property */
	const char *dnaname;
	const void *dnatype;
	int dnaarraylength;
	int dnapointerlevel;
	/**
	 * Offset in bytes within `dnastructname`.
	 * -1 when unusable (follows pointer for example).
	 */
	int dnaoffset;
	int dnasize;

	/* for finding length of array collections */
	const char *dnalengthstructname;
	const char *dnalengthname;
	int dnalengthfixed;

	int64_t booleanbit;
	bool booleannegative;
} PropertyDefRNA;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Alloc RNA Definition Structures
 * \{ */

typedef struct AllocDefRNA {
	struct AllocDefRNA *prev, *next;
	void *mem;
} AllocDefRNA;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Rose RNA Definition Structures
 * \{ */

typedef struct RoseDefRNA {
	struct SDNA *sdna;

	struct ListBase structs;
	struct ListBase allocs;
	struct StructRNA *nstruct;

	struct {
		unsigned int silent : 1;
		unsigned int error : 1;
		unsigned int preprocess : 1;
		unsigned int animate : 1;
	};
} RoseDefRNA;

/** \} */

/* -------------------------------------------------------------------- */
/** \name RNA Path Functions
 * \{ */

char *rna_path_token(const char **path, char *fixedbuf, size_t fixedlen);
char *rna_path_token_in_brackets(const char **path, char *fixedbuf, size_t fixedlen, bool *quoted);

/** \} */

void RNA_def_rna(struct RoseRNA *rna);
void RNA_def_ID(struct RoseRNA *rna);
void RNA_def_Object(struct RoseRNA *rna);
void RNA_def_Pose(struct RoseRNA *rna);
void RNA_def_space(struct RoseRNA *rna);
void RNA_def_wm(struct RoseRNA *rna);

/* -------------------------------------------------------------------- */
/** \name IDProperty RNA Definition Functions
 * \{ */

void rna_IDPArray_begin(struct CollectionPropertyIterator *iter, struct PointerRNA *ptr);
void rna_IDPArray_length(struct PointerRNA *ptr);

struct IDProperty **rna_PropertyGroup_idprops(struct PointerRNA *ptr);
struct StructRNA *rna_PropertyGroup_refine(struct PointerRNA *ptr);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Property RNA Definition Functions
 * \{ */

struct PropertyDefRNA *rna_find_struct_property_def(struct StructRNA *nstruct, struct PropertyRNA *proprety);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Iteraton
 * \{ */

void rna_iterator_array_begin(CollectionPropertyIterator *iter, PointerRNA *ptr, void *data, size_t itemsize, int64_t length, bool free_ptr, IteratorSkipFunc skip);
void rna_iterator_array_next(CollectionPropertyIterator *iter);
void *rna_iterator_array_get(CollectionPropertyIterator *iter);
void rna_iterator_array_end(CollectionPropertyIterator *iter);

void rna_iterator_listbase_begin(struct CollectionPropertyIterator *iter, struct PointerRNA *ptr, struct ListBase *lb, IteratorSkipFunc skip);
void rna_iterator_listbase_next(struct CollectionPropertyIterator *iter);
void rna_iterator_listbase_end(struct CollectionPropertyIterator *iter);

void *rna_iterator_listbase_get(struct CollectionPropertyIterator *iter);

void rna_builtin_properties_begin(struct CollectionPropertyIterator *iter, struct PointerRNA *ptr);
void rna_builtin_properties_next(struct CollectionPropertyIterator *iter);
void rna_builtin_properties_end(struct CollectionPropertyIterator *iter);

PointerRNA rna_builtin_properties_get(struct CollectionPropertyIterator *iter);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Pointer Handling
 * \{ */

/**
 * Internal implementation for #RNA_pointer_create_with_parent.
 *
 * Only exposed to RNA code because custom collection lookup functions get an existing PointerRNA
 * data to modify, instead of returning a new one.
 */
void rna_pointer_create_with_ancestors(const struct PointerRNA *parent, struct StructRNA *type, void *data, struct PointerRNA *r_ptr);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RNA_INTERNAL_H
