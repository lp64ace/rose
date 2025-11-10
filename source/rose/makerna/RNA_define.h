#ifndef RNA_DEFINE_H
#define RNA_DEFINE_H

#include "RNA_types.h"

struct RoseRNA;
struct StructRNA;

#define RNA_MAX_ARRAY_LENGTH 64
#define RNA_MAX_ARRAY_DIMENSION 3

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Rose RNA Definition
 * \{ */

struct RoseRNA *RNA_create(void);
void RNA_define_free(struct RoseRNA *brna);
void RNA_free(struct RoseRNA *brna);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Struct RNA Definition
 * \{ */

struct StructRNA *RNA_def_struct_ex(struct RoseRNA *brna, const char *identifier, struct StructRNA *srnafrom);
struct StructRNA *RNA_def_struct(struct RoseRNA *brna, const char *identifier, const char *from);
void RNA_struct_free(struct RoseRNA *rna, struct StructRNA *srna);

void RNA_def_struct_ui_text(struct StructRNA *srna, const char *name, const char *description);

void RNA_def_struct_flag(struct StructRNA *nstruct, int flag);
void RNA_def_struct_clear_flag(struct StructRNA *nstruct, int flag);

void RNA_def_struct_name_property(struct StructRNA *srna, struct PropertyRNA *prop);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Property RNA Definition
 * \{ */

struct PropertyRNA *RNA_def_property(void *container, const char *identifier, int type, int subtype);

void RNA_def_property_ui_text(struct PropertyRNA *prop, const char *name, const char *description);

void RNA_def_property_flag(struct PropertyRNA *prop, ePropertyFlag flag);
void RNA_def_property_clear_flag(struct PropertyRNA *prop, ePropertyFlag flag);

#define IS_DNATYPE_FLOAT_COMPAT(_str) (strcmp(_str, "float") == 0 || strcmp(_str, "double") == 0)
#define IS_DNATYPE_INT_COMPAT(_str) (strcmp(_str, "int") == 0 || strcmp(_str, "short") == 0 || strcmp(_str, "char") == 0 || strcmp(_str, "uchar") == 0 || strcmp(_str, "ushort") == 0 || strcmp(_str, "int8_t") == 0)
#define IS_DNATYPE_BOOLEAN_COMPAT(_str) (IS_DNATYPE_INT_COMPAT(_str) || strcmp(_str, "int64_t") == 0 || strcmp(_str, "uint64_t") == 0)

void RNA_def_property_boolean_sdna(struct PropertyRNA *prop, const char *structname, const char *propname, int64_t booleanbit);
void RNA_def_property_int_sdna(struct PropertyRNA *prop, const char *structname, const char *propname);
void RNA_def_property_float_sdna(struct PropertyRNA *prop, const char *structname, const char *propname);
void RNA_def_property_string_sdna(struct PropertyRNA *prop, const char *structname, const char *propname);
void RNA_def_property_pointer_sdna(struct PropertyRNA *prop, const char *structname, const char *propname);

void RNA_def_property_array(struct PropertyRNA *prop, int length);

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Property RNA Definition
 * \{ */

void RNA_def_property_string_maxlength(struct PropertyRNA *prop, int maxlength);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Helper Functions
 * \{ */

const char *RNA_property_typename(enum ePropertyType type);
const char *RNA_property_rawtypename(enum eRawPropertyType type);
const char *RNA_property_subtypename(enum ePropertySubType type);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RNA_DEFINE_H
