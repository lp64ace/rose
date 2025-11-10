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

	ContainerRNA *ptr;
	ListBase properties;
} ContainerDefRNA;

typedef struct StructDefRNA {
	ContainerDefRNA container;

	StructRNA *ptr;

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

	ContainerRNA *container;
	PropertyRNA *ptr;

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
/** \name Rose RNA Definition Structures
 * \{ */

typedef struct RoseDefRNA {
	struct SDNA *sdna;

	ListBase structs;
	StructRNA *nstruct;

	struct {
		unsigned int error : 1;
		unsigned int preprocess : 1;
		unsigned int animate : 1;
	};
} RoseDefRNA;

/** \} */

void RNA_def_ID(RoseRNA *rna);

/* -------------------------------------------------------------------- */
/** \name Property RNA Definition Functions
 * \{ */

struct PropertyDefRNA *rna_find_struct_property_def(struct StructRNA *nstruct, struct PropertyRNA *proprety);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RNA_INTERNAL_H
