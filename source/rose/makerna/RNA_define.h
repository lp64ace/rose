#ifndef RNA_DEFINE_H
#define RNA_DEFINE_H

#include "RNA_types.h"

struct DNAType;
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
void RNA_def_struct_sdna(struct StructRNA *srna, const char *structname);
void RNA_def_struct_flag(struct StructRNA *nstruct, int flag);
void RNA_def_struct_clear_flag(struct StructRNA *nstruct, int flag);

void RNA_def_struct_name_property(struct StructRNA *srna, struct PropertyRNA *prop);
void RNA_def_struct_refine_func(struct StructRNA *srna, const char *func);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Property RNA Definition
 * \{ */

#define RNA_TRANSLATION_PREC_DEFAULT 3

struct PropertyRNA *RNA_def_property(void *container, const char *identifier, int type, int subtype);

void RNA_def_property_ui_text(struct PropertyRNA *prop, const char *name, const char *description);
void RNA_def_property_ui_range(struct PropertyRNA *prop, double min, double max, double step, int precision);

void RNA_def_property_flag(struct PropertyRNA *prop, ePropertyFlag flag);
void RNA_def_property_clear_flag(struct PropertyRNA *prop, ePropertyFlag flag);

void RNA_def_property_boolean_sdna(struct PropertyRNA *prop, const char *structname, const char *propname, int64_t booleanbit);
void RNA_def_property_int_sdna(struct PropertyRNA *prop, const char *structname, const char *propname);
void RNA_def_property_float_sdna(struct PropertyRNA *prop, const char *structname, const char *propname);
void RNA_def_property_string_sdna(struct PropertyRNA *prop, const char *structname, const char *propname);
void RNA_def_property_pointer_sdna(struct PropertyRNA *prop, const char *structname, const char *propname);
void RNA_def_property_collection_sdna(struct PropertyRNA *prop, const char *structname, const char *propname, const char *lengthpropname);

void RNA_def_property_struct_type(struct PropertyRNA *prop, const char *type);
void RNA_def_property_array(struct PropertyRNA *prop, int length);

void RNA_def_property_string_funcs(struct PropertyRNA *prop, const char *get, const char *length, const char *set);
void RNA_def_property_collection_funcs(struct PropertyRNA *prop, const char *begin, const char *next, const char *end, const char *get, const char *length, const char *lookupint, const char *lookupstring);

/* Common arguments for defaults. */

extern const float rna_default_axis_angle[4];
extern const float rna_default_quaternion[4];
extern const float rna_default_scale_3d[3];

void RNA_def_property_float_array_default(struct PropertyRNA *prop, const float *defaultarray);

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

/* -------------------------------------------------------------------- */
/** \name RNA Exposed Functions
 * \{ */

void rna_Struct_properties_begin(struct CollectionPropertyIterator *iter, struct PointerRNA *ptr);
void rna_Struct_properties_next(struct CollectionPropertyIterator *iter);

struct PointerRNA rna_Struct_properties_get(struct CollectionPropertyIterator *iter);

/** \} */

/* -------------------------------------------------------------------- */
/** \name RNA Exposed Functions
 * \{ */

void rna_Property_name_get(struct PointerRNA *ptr, char *value);
int rna_Property_name_length(struct PointerRNA *ptr);

/** \} */

/* -------------------------------------------------------------------- */
/** \name ID Exposed Functions
 * \{ */

struct StructRNA *rna_ID_refine(struct PointerRNA *ptr);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Pose Exposed Functions
 * \{ */

bool rna_PoseBones_lookup_string(struct PointerRNA *ptr, const char *key, struct PointerRNA *r_ptr);

/** \} */

bool IS_DNATYPE_FLOAT_COMPAT(const struct DNAType *type);
bool IS_DNATYPE_INT_COMPAT(const struct DNAType *type);
bool IS_DNATYPE_BOOLEAN_COMPAT(const struct DNAType *type);

#ifdef __cplusplus
}
#endif

#endif	// RNA_DEFINE_H
