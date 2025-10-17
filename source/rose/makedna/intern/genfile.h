#ifndef DNA_GENFILE_H
#define DNA_GENFILE_H

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct SDNA;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct SDNA {
	struct RTContext *context;
	struct RTCParser *parser;

	struct GHash *types;
	struct GHash *visit;

	void *data;
	size_t length;
	size_t allocated;
} SDNA;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

struct SDNA *DNA_sdna_new_empty(void);
/** Will only init the buffer from memory will not actually build SDNA! */
struct SDNA *DNA_sdna_new_memory(const void *memory, size_t length);
struct SDNA *DNA_sdna_new_current(void);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Free Methods
 * \{ */

void DNA_sdna_free(struct SDNA *sdna);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Write DNA
 * \{ */

struct RTType;
struct RTToken;

bool DNA_sdna_write_word(struct SDNA *sdna, void **rest, void *ptr, int word);
bool DNA_sdna_write_i16(struct SDNA *sdna, void **rest, void *ptr, int16_t word);
bool DNA_sdna_write_i32(struct SDNA *sdna, void **rest, void *ptr, int32_t word);
bool DNA_sdna_write_i64(struct SDNA *sdna, void **rest, void *ptr, int64_t word);
bool DNA_sdna_write_u16(struct SDNA *sdna, void **rest, void *ptr, uint16_t word);
bool DNA_sdna_write_u32(struct SDNA *sdna, void **rest, void *ptr, uint32_t word);
bool DNA_sdna_write_u64(struct SDNA *sdna, void **rest, void *ptr, uint64_t word);

bool DNA_sdna_write_ull(struct SDNA *sdna, void **rest, void *ptr, unsigned long long word);
bool DNA_sdna_write_bool(struct SDNA *sdna, void **rest, void *ptr, bool word);
bool DNA_sdna_write_str(struct SDNA *sdna, void **rest, void *ptr, const char *word);

bool DNA_sdna_write_type(struct SDNA *sdna, void **rest, void *ptr, const struct RTType *tp);
bool DNA_sdna_write_token(struct SDNA *sdna, void **rest, void *ptr, const struct RTToken *tok);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Read DNA
 * \{ */

struct RTType;
struct RTToken;

bool DNA_sdna_read_word(const struct SDNA *sdna, const void **rest, const void *ptr, int word);
bool DNA_sdna_read_i16(const struct SDNA *sdna, const void **rest, const void *ptr, int16_t *word);
bool DNA_sdna_read_i32(const struct SDNA *sdna, const void **rest, const void *ptr, int32_t *word);
bool DNA_sdna_read_i64(const struct SDNA *sdna, const void **rest, const void *ptr, int64_t *word);
bool DNA_sdna_read_u16(const struct SDNA *sdna, const void **rest, const void *ptr, uint16_t *word);
bool DNA_sdna_read_u32(const struct SDNA *sdna, const void **rest, const void *ptr, uint32_t *word);
bool DNA_sdna_read_u64(const struct SDNA *sdna, const void **rest, const void *ptr, uint64_t *word);

bool DNA_sdna_read_ull(const struct SDNA *sdna, const void **rest, const void *ptr, unsigned long long *word);
bool DNA_sdna_read_bool(const struct SDNA *sdna, const void **rest, const void *ptr, bool *word);
bool DNA_sdna_read_str(const struct SDNA *sdna, const void **rest, const void *ptr, char *word);

bool DNA_sdna_read_type(const struct SDNA *sdna, const void **rest, const void *ptr, const struct RTType **tp);
bool DNA_sdna_read_token(const struct SDNA *sdna, const void **rest, const void *ptr, const struct RTToken **tok);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool DNA_sdna_needs_endian_swap(const struct SDNA *sdna);
bool DNA_sdna_build_struct_list(struct SDNA *sdna);

void DNA_struct_switch_endian(const struct SDNA *sdna, uint64_t struct_nr, void *data);

size_t DNA_sdna_struct_size(const struct SDNA *sdna, uint64_t struct_nr);

uint64_t DNA_sdna_struct_id(const struct SDNA *sdna, const char *name);

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNAType
 * \{ */

typedef enum eDNAType {
	DNA_VOID = 0,
	DNA_CHAR,
	DNA_SHORT,
	DNA_INT,
	DNA_LONG,
	DNA_LLONG,
	DNA_FLOAT,
	DNA_DOUBLE,
	DNA_LDOUBLE,
	DNA_ENUM,
	DNA_PTR,
	DNA_ARRAY,
	DNA_FUNC,
	DNA_STRUCT,
	DNA_UNION,
	DNA_QUALIFIED,
	DNA_VARIADIC,
	DNA_ELLIPSIS,
} eDNAType;

typedef struct DNAType DNAType;

/** Returns the kind of type this type resolves to, for casting! */
enum eDNAType DNA_sdna_type_kind(struct SDNA *sdna, const struct DNAType *type);

/** Returns true if the type is a basic kind of type and therefore can be casted to #DNATypeBasic */
bool DNA_sdna_type_basic(struct SDNA *sdna, const struct DNAType *type);

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeBasic
 * \{ */

typedef struct DNATypeBasic DNATypeBasic;

/** Returns true if the specified basic type is unsigned. */
bool DNA_sdna_basic_is_unsigned(struct SDNA *sdna, const struct DNATypeBasic *type);

/** Returns the size in bytes of the specfied basic type. */
size_t DNA_sdna_basic_rank(struct SDNA *sdna, const struct DNATypeBasic *type);

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeEnum
 * \{ */

typedef struct DNATypeEnum DNATypeEnum;
typedef struct DNATypeEnumItem DNATypeEnumItem;

// DNATypeEnumItem

/** Returns the name/identifier of the enum item. */
const char *DNA_sdna_enum_item_identifier(struct SDNA *sdna, const struct DNATypeEnum *type, const struct DNATypeEnumItem *item);

// DNATypeEnum

/** Returns the name/identifier of the enum tag. */
const char *DNA_sdna_enum_identifier(struct SDNA *sdna, const struct DNATypeEnum *type);

/** Returns the underlying type of the enum. */
const struct DNAType *DNA_sdna_enum_underlying_type(struct SDNA *sdna, const struct DNATypeEnum *type);

/** Returns a pointer to the listbase that stores the enum items. */
const ListBase *DNA_sdna_enum_items(struct SDNA *sdna, const struct DNATypeEnum *type);

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypePointer
 * \{ */

typedef struct DNATypePointer DNATypePointer;

const struct DNAType *DNA_sdna_pointer_pointee(struct SDNA *sdna, const struct DNATypePointer *type);

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeArray
 * \{ */

typedef struct DNATypeArray DNATypeArray;

typedef enum eDNAArrayBoundary {
	DNA_ARRAY_UNBOUNDED = 0,
	DNA_ARRAY_BOUNDED,
	DNA_ARRAY_BOUNDED_STATIC,
	DNA_ARRAY_VLA,
	DNA_ARRAY_VLA_STATIC,
} eDNAArrayBoundary;

/** Returns the base type of the array (the type of each element within the array). */
const struct DNAType *DNA_sdna_array_element(struct SDNA *sdna, const struct DNATypeArray *type);

/** Returns the boundary type of the specified array. */
enum eDNAArrayBoundary DNA_sdna_array_boundary(struct SDNA *sdna, const struct DNATypeArray *type);

/** Returns the constant evaluated length of the array if applicable. */
size_t DNA_sdna_array_length(struct SDNA *sdna, const struct DNATypeArray *type);

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeStruct
 * \{ */

typedef struct DNATypeStruct DNATypeStruct;
typedef struct DNATypeStructField DNATypeStructField;

// DNATypeStructField

/** Returns the identifier of the specified struct field. */
const char *DNA_sdna_struct_field_identifier(struct SDNA *sdna, const struct DNATypeStruct *type, const struct DNATypeStructField *field);

/** Returns the type of the specified struct field. */
const struct DNAType *DNA_sdna_struct_field_type(struct SDNA *sdna, const struct DNATypeStruct *type, const struct DNATypeStructField *field);

/** Returns the alignment of the specified struct field. */
size_t DNA_sdna_struct_field_alignment(struct SDNA *sdna, const struct DNATypeStruct *type, const struct DNATypeStructField *field);

// DNATypeStruct

/** Returns the tag of the specified struct. */
const char *DNA_sdna_struct_identifier(struct SDNA *sdna, const struct DNATypeStruct *type);

/** Returns the listbase containing the fields of the specified struct. */
const struct ListBase *DNA_sdna_struct_fields(struct SDNA *sdna, const struct DNATypeStruct *type);

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeQual
 * \{ */

typedef struct DNATypeQual DNATypeQual;

/** Returns the base type of the qualified type. */
const struct DNAType *DNA_sdna_qual_type(struct SDNA *sdna, const struct DNATypeQual *type);

bool DNA_sdna_qual_is_const(struct SDNA *sdna, const struct DNATypeQual *type);
bool DNA_sdna_qual_is_restrict(struct SDNA *sdna, const struct DNATypeQual *type);
bool DNA_sdna_qual_is_volatile(struct SDNA *sdna, const struct DNATypeQual *type);
bool DNA_sdna_qual_is_atomic(struct SDNA *sdna, const struct DNATypeQual *type);

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNA Util Methods
 * \{ */

const struct DNAType *DNA_sdna_type(struct SDNA *sdna, const char *name);
const size_t DNA_sdna_sizeof(struct SDNA *sdna, const struct DNAType *type);
const size_t DNA_sdna_offsetof(struct SDNA *sdna, const struct DNATypeStruct *type, const struct DNATypeStructField *field);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Reconstruction Methods
 * \{ */

/**
 * This function, `DNA_sdna_struct_reconstruct`, serves as the high-level entry point for reconstructing a data structure
 * based on version-specific metadata provided by `SDNA`. It leverages information about the source (`dna_old`) and target
 * (`dna_new`) DNA definitions to ensure that data from an older serialized version is correctly transformed into the format
 * required by the current version of the software. The process involves identifying corresponding types, allocating memory for
 * the new structure, and invoking lower-level reconstruction logic.
 *
 * During deserialization, this function identifies the mapping between `OldStruct` and `NewStruct`, allocates memory for the new
 * structure, and performs the necessary transformations:
 * - Init to Zero when a completely new field is added.
 * - Memcpy when a completely identical field is found in both the `OldStruct` and `NewStruct`.
 * - Cast when an identical field is found between `OldStruct` and `NewStruct` is found but the type is different,
 * in case of invalid type conversion the new field will be initialzed to zero.
 *
 * NOTE: This function will NOT cast float to integer type or the opposite since this could lead to unintentional behaviour,
 * when for example we are casting `unsigned char [3]` to `float [3]` for a color, then we would end up with unormalized
 * float values, instead a warning is thrown!
 */
void *DNA_sdna_struct_reconstruct(const struct SDNA *dna_old, const struct SDNA *dna_new, uint64_t struct_nr, const void *data_old, const char *blockname);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// DNA_GENFILE_H
