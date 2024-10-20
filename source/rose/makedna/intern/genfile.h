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
struct SDNA *DNA_sdna_new_memory(const void *memory, size_t length);

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

/** Returns true when the endianess of the specified `sdna` is different than the current. */
bool DNA_sdna_needs_endian_swap(const struct SDNA *sdna);
bool DNA_sdna_check(const struct SDNA *sdna);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // DNA_GENFILE_H
