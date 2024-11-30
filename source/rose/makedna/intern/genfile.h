#ifndef DNA_GENFILE_H
#define DNA_GENFILE_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct SDNA;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct SDNA {
	struct RCContext *context;

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

bool DNA_sdna_write_type(struct SDNA *sdna, void **rest, void *ptr, const RCCType *tp);
bool DNA_sdna_write_token(struct SDNA *sdna, void **rest, void *ptr, const RCCToken *tok);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Read DNA
 * \{ */

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

bool DNA_sdna_read_type(const struct SDNA *sdna, const void **rest, const void *ptr, const RCCType **tp);
bool DNA_sdna_read_token(const struct SDNA *sdna, const void **rest, const void *ptr, const RCCToken **tok);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool DNA_sdna_needs_endian_swap(const struct SDNA *sdna);
bool DNA_sdna_build_struct_list(const struct SDNA *sdna);

void DNA_struct_switch_endian(const struct SDNA *sdna, uint64_t struct_nr, void *data);

size_t DNA_sdna_struct_size(const struct SDNA *sdna, uint64_t struct_nr);
ptrdiff_t DNA_sdna_field_offset(const struct SDNA *sdna, uint64_t struct_nr, const char *field);
/**
 * Returns the unique identifier for the struct within the specified SDNA,
 * not unique across different (possibly completely same) SDNA configurations!
 */
uint64_t DNA_sdna_struct_ex(const struct SDNA *sdna, const struct RCCType *type);
uint64_t DNA_sdna_struct_id(const struct SDNA *sdna, const char *name);

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
