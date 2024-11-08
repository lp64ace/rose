#include "MEM_guardedalloc.h"

#include "LIB_endian_switch.h"
#include "LIB_ghash.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "RT_context.h"
#include "RT_parser.h"
#include "RT_token.h"

#include "genfile.h"

#ifdef __BIG_ENDIAN__
/* Big Endian */
#	define MAKE_ID4(a, b, c, d) ((a) << 24 | (b) << 16 | (c) << 8 | (d))
#else
/* Little Endian */
#	define MAKE_ID4(a, b, c, d) ((d) << 24 | (c) << 16 | (b) << 8 | (a))
#endif

#define DATA_GROW 1024

/* -------------------------------------------------------------------- */
/** \name Internal Util Methods
 * \{ */

ROSE_INLINE bool sdna_grow(struct SDNA *sdna, size_t nlength) {
	while (sdna->allocated <= nlength) {
		sdna->allocated = sdna->allocated + ROSE_MAX(sdna->allocated, nlength);
		if (!(sdna->data = MEM_reallocN(sdna->data, sdna->allocated))) {
			return false;
		}
	}
	return true;
}

ROSE_INLINE bool sdna_write(struct SDNA *sdna, void **rest, void *ptr, const void *mem, size_t length, bool fix) {
	ptrdiff_t offset = ((const char *)ptr) - ((const char *)sdna->data);
	if (sdna_grow(sdna, offset + length)) {
		memcpy(POINTER_OFFSET(sdna->data, offset), mem, length);
		if (fix && DNA_sdna_needs_endian_swap(sdna)) {
			LIB_endian_switch_rank(POINTER_OFFSET(sdna->data, offset), length);
		}
		sdna->length = ROSE_MAX(sdna->length, offset + length);
		*rest = POINTER_OFFSET(sdna->data, offset + length);
		return true;
	}
	return false;
}
ROSE_INLINE bool sdna_read(const struct SDNA *sdna, const void **rest, const void *ptr, void *mem, size_t length, bool fix) {
	ptrdiff_t offset = ((const char *)ptr) - ((const char *)sdna->data);
	if (POINTER_OFFSET(sdna->data, offset + length) <= POINTER_OFFSET(sdna->data, sdna->length)) {
		memcpy(mem, POINTER_OFFSET(sdna->data, offset), length);
		if (fix && DNA_sdna_needs_endian_swap(sdna)) {
			LIB_endian_switch_rank(mem, length);
		}
		*rest = POINTER_OFFSET(sdna->data, offset + length);
		return true;
	}
	return false;
}

ROSE_INLINE bool sdna_write_type_array(struct SDNA *sdna, void **rest, void *ptr, const RCCTypeArray *array) {
	bool status = true;
	status &= DNA_sdna_write_type(sdna, &ptr, ptr, array->element_type);
	status &= DNA_sdna_write_i32(sdna, &ptr, ptr, array->boundary);
	status &= DNA_sdna_write_bool(sdna, &ptr, ptr, array->qualification.is_constant);
	status &= DNA_sdna_write_bool(sdna, &ptr, ptr, array->qualification.is_restricted);
	status &= DNA_sdna_write_bool(sdna, &ptr, ptr, array->qualification.is_volatile);
	status &= DNA_sdna_write_bool(sdna, &ptr, ptr, array->qualification.is_atomic);
	status &= DNA_sdna_write_ull(sdna, &ptr, ptr, array->length);
	if (status) {
		*rest = ptr;
	}
	return status;
}
ROSE_INLINE bool sdna_read_type_array(const struct SDNA *sdna, const void **rest, const void *ptr, RCCTypeArray *array) {
	bool status = true;
	status &= DNA_sdna_read_type(sdna, &ptr, ptr, &array->element_type);
	status &= DNA_sdna_read_i32(sdna, &ptr, ptr, &array->boundary);
	status &= DNA_sdna_read_bool(sdna, &ptr, ptr, &array->qualification.is_constant);
	status &= DNA_sdna_read_bool(sdna, &ptr, ptr, &array->qualification.is_restricted);
	status &= DNA_sdna_read_bool(sdna, &ptr, ptr, &array->qualification.is_volatile);
	status &= DNA_sdna_read_bool(sdna, &ptr, ptr, &array->qualification.is_atomic);
	status &= DNA_sdna_read_ull(sdna, &ptr, ptr, &array->length);
	if (status) {
		*rest = ptr;
	}
	return status;
}

ROSE_INLINE bool sdna_write_type_basic(struct SDNA *sdna, void **rest, void *ptr, const RCCTypeBasic *basic) {
	bool status = true;
	status &= DNA_sdna_write_bool(sdna, &ptr, ptr, basic->is_unsigned);
	status &= DNA_sdna_write_i32(sdna, &ptr, ptr, basic->rank);
	if (status) {
		*rest = ptr;
	}
	return status;
}
ROSE_INLINE bool sdna_read_type_basic(const struct SDNA *sdna, const void **rest, const void *ptr, RCCTypeBasic *basic) {
	bool status = true;
	status &= DNA_sdna_read_bool(sdna, &ptr, ptr, &basic->is_unsigned);
	status &= DNA_sdna_read_i32(sdna, &ptr, ptr, &basic->rank);
	if (status) {
		*rest = ptr;
	}
	return status;
}

ROSE_INLINE bool sdna_write_type_ptr(struct SDNA *sdna, void **rest, void *ptr, const RCCType *base) {
	return DNA_sdna_write_type(sdna, rest, ptr, base);
}
ROSE_INLINE bool sdna_read_type_ptr(const struct SDNA *sdna, const void **rest, const void *ptr, const RCCType **base) {
	return DNA_sdna_read_type(sdna, rest, ptr, base);
}

ROSE_INLINE bool sdna_write_type_qual(struct SDNA *sdna, void **rest, void *ptr, const RCCTypeQualified *qual) {
	bool status = true;
	status &= DNA_sdna_write_type(sdna, &ptr, ptr, qual->base);
	status &= DNA_sdna_write_bool(sdna, &ptr, ptr, qual->qualification.is_constant);
	status &= DNA_sdna_write_bool(sdna, &ptr, ptr, qual->qualification.is_restricted);
	status &= DNA_sdna_write_bool(sdna, &ptr, ptr, qual->qualification.is_volatile);
	status &= DNA_sdna_write_bool(sdna, &ptr, ptr, qual->qualification.is_atomic);
	if (status) {
		*rest = ptr;
	}
	return status;
}
ROSE_INLINE bool sdna_read_type_qual(const struct SDNA *sdna, const void **rest, const void *ptr, RCCTypeQualified *qual) {
	bool status = true;
	status &= DNA_sdna_read_type(sdna, &ptr, ptr, &qual->base);
	status &= DNA_sdna_read_bool(sdna, &ptr, ptr, &qual->qualification.is_constant);
	status &= DNA_sdna_read_bool(sdna, &ptr, ptr, &qual->qualification.is_restricted);
	status &= DNA_sdna_read_bool(sdna, &ptr, ptr, &qual->qualification.is_volatile);
	status &= DNA_sdna_read_bool(sdna, &ptr, ptr, &qual->qualification.is_atomic);
	if (status) {
		*rest = ptr;
	}
	return status;
}

ROSE_INLINE bool sdna_write_type_struct(struct SDNA *sdna, void **rest, void *ptr, const RCCTypeStruct *strct) {
	bool status = true;

	int length = (int)LIB_listbase_count(&strct->fields);
	status &= DNA_sdna_write_token(sdna, &ptr, ptr, strct->identifier);
	status &= DNA_sdna_write_bool(sdna, &ptr, ptr, strct->is_complete);
	status &= DNA_sdna_write_i32(sdna, &ptr, ptr, length);
	LISTBASE_FOREACH(const RCCField *, field, &strct->fields) {
		status &= DNA_sdna_write_token(sdna, &ptr, ptr, field->identifier);
		status &= DNA_sdna_write_type(sdna, &ptr, ptr, field->type);
		status &= DNA_sdna_write_i32(sdna, &ptr, ptr, field->alignment);
		status &= DNA_sdna_write_bool(sdna, &ptr, ptr, field->properties.is_bitfield);
		status &= DNA_sdna_write_i32(sdna, &ptr, ptr, field->properties.width);
	}
	if (status) {
		*rest = ptr;
	}
	return status;
}
ROSE_INLINE bool sdna_read_type_struct(const struct SDNA *sdna, const void **rest, const void *ptr, RCCTypeStruct *strct) {
	bool status = true;

	int length;
	status &= DNA_sdna_read_token(sdna, &ptr, ptr, &strct->identifier);
	status &= DNA_sdna_read_bool(sdna, &ptr, ptr, &strct->is_complete);
	status &= DNA_sdna_read_i32(sdna, &ptr, ptr, &length);
	while (length--) {
		RCCField *field = RT_context_calloc(sdna->context, sizeof(RCCField));

		status &= DNA_sdna_read_token(sdna, &ptr, ptr, &field->identifier);
		status &= DNA_sdna_read_type(sdna, &ptr, ptr, &field->type);
		status &= DNA_sdna_read_i32(sdna, &ptr, ptr, &field->alignment);
		status &= DNA_sdna_read_bool(sdna, &ptr, ptr, &field->properties.is_bitfield);
		status &= DNA_sdna_read_i32(sdna, &ptr, ptr, &field->properties.width);

		LIB_addtail(&strct->fields, field);
	}
	if (status) {
		*rest = ptr;
	}
	return status;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

SDNA *DNA_sdna_new_empty(void) {
	SDNA *sdna = MEM_mallocN(sizeof(SDNA), "SDNA");

	sdna->context = RT_context_new();
	sdna->types = LIB_ghash_str_new("SDNA::types");
	sdna->visit = LIB_ghash_ptr_new("SDNA::visit");

	sdna->data = NULL;
	sdna->length = 0;
	sdna->allocated = 0;

	void *ptr = sdna->data;
	DNA_sdna_write_word(sdna, &ptr, ptr, MAKE_ID4('S', 'D', 'N', 'A'));

#ifdef __BIG_ENDIAN__
	DNA_sdna_write_word(sdna, &ptr, ptr, MAKE_ID4('B', 'I', 'G', 'E'));
#else
	DNA_sdna_write_word(sdna, &ptr, ptr, MAKE_ID4('L', 'T', 'L', 'E'));
#endif

	return sdna;
}

SDNA *DNA_sdna_new_memory(const void *memory, size_t length) {
	SDNA *sdna = MEM_mallocN(sizeof(SDNA), "SDNA");

	sdna->context = RT_context_new();
	sdna->types = LIB_ghash_str_new("SDNA::types");
	sdna->visit = LIB_ghash_ptr_new("SDNA::visit");

	sdna->data = (void *)memory;
	sdna->length = length;
	sdna->allocated = 0;

	return sdna;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Free Methods
 * \{ */

void DNA_sdna_free(SDNA *sdna) {
	if (sdna->types) {
		LIB_ghash_free(sdna->types, NULL, NULL);
	}
	if (sdna->visit) {
		LIB_ghash_free(sdna->visit, NULL, NULL);
	}
	if (sdna->context) {
		RT_context_free(sdna->context);
	}
	if (sdna->allocated) {
		MEM_freeN(sdna->data);
	}

	MEM_freeN(sdna);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Write DNA
 * \{ */

bool DNA_sdna_write_word(struct SDNA *sdna, void **rest, void *ptr, int word) {
	return sdna_write(sdna, rest, ptr, &word, 4, false);
}
bool DNA_sdna_write_i16(struct SDNA *sdna, void **rest, void *ptr, int16_t word) {
	return sdna_write(sdna, rest, ptr, &word, sizeof(word), true);
}
bool DNA_sdna_write_i32(struct SDNA *sdna, void **rest, void *ptr, int32_t word) {
	return sdna_write(sdna, rest, ptr, &word, sizeof(word), true);
}
bool DNA_sdna_write_i64(struct SDNA *sdna, void **rest, void *ptr, int64_t word) {
	return sdna_write(sdna, rest, ptr, &word, sizeof(word), true);
}
bool DNA_sdna_write_u16(struct SDNA *sdna, void **rest, void *ptr, uint16_t word) {
	return sdna_write(sdna, rest, ptr, &word, sizeof(word), true);
}
bool DNA_sdna_write_u32(struct SDNA *sdna, void **rest, void *ptr, uint32_t word) {
	return sdna_write(sdna, rest, ptr, &word, sizeof(word), true);
}
bool DNA_sdna_write_u64(struct SDNA *sdna, void **rest, void *ptr, uint64_t word) {
	return sdna_write(sdna, rest, ptr, &word, sizeof(word), true);
}

bool DNA_sdna_write_ull(struct SDNA *sdna, void **rest, void *ptr, unsigned long long word) {
	uint64_t u64 = (uint64_t)word;
	return DNA_sdna_write_u64(sdna, rest, ptr, u64);
}
bool DNA_sdna_write_bool(struct SDNA *sdna, void **rest, void *ptr, bool word) {
	return DNA_sdna_write_i32(sdna, rest, ptr, (word) ? 1 : 0);
}
bool DNA_sdna_write_str(struct SDNA *sdna, void **rest, void *ptr, const char *word) {
	size_t length = LIB_strnlen(word, 65);
	if (length <= 64) {
		return sdna_write(sdna, rest, ptr, word, length + 1, false);
	}
	return false;
}

bool DNA_sdna_write_type(struct SDNA *sdna, void **rest, void *ptr, const RCCType *tp) {
	bool status = true;

	uint64_t addr = (uint64_t)tp;
	status &= DNA_sdna_write_word(sdna, &ptr, ptr, MAKE_ID4('T', 'Y', 'P', 'E'));
	status &= DNA_sdna_write_u64(sdna, &ptr, ptr, addr);
	if (LIB_ghash_haskey(sdna->visit, (void *)addr)) {
		// Nothing to do here, we have already written thistype!
	}
	else {
		LIB_ghash_insert(sdna->visit, (void *)addr, NULL);

		status &= DNA_sdna_write_i32(sdna, &ptr, ptr, tp->kind);
		status &= DNA_sdna_write_bool(sdna, &ptr, ptr, tp->is_basic);

		if (tp->is_basic) {
			status &= sdna_write_type_basic(sdna, &ptr, ptr, &tp->tp_basic);
		}
		else {
			switch (tp->kind) {
				case TP_ARRAY: {
					status &= sdna_write_type_array(sdna, &ptr, ptr, &tp->tp_array);
				} break;
				case TP_PTR: {
					status &= sdna_write_type_ptr(sdna, &ptr, ptr, tp->tp_base);
				} break;
				case TP_STRUCT: {
					status &= sdna_write_type_struct(sdna, &ptr, ptr, &tp->tp_struct);
				} break;
				default: {
					ROSE_assert_unreachable();
					status &= false;
				} break;
			}
		}
	}
	if (status) {
		*rest = ptr;
	}
	return status;
}
bool DNA_sdna_write_token(struct SDNA *sdna, void **rest, void *ptr, const RCCToken *tok) {
	bool status = true;

	{
		const char *identifier = RT_token_as_string(tok);
		status &= DNA_sdna_write_word(sdna, &ptr, ptr, MAKE_ID4('T', 'O', 'K', 'N'));
		status &= DNA_sdna_write_str(sdna, &ptr, ptr, identifier);
	}
	if (status) {
		*rest = ptr;
	}
	return status;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Read DNA
 * \{ */

bool DNA_sdna_read_word(const struct SDNA *sdna, const void **rest, const void *ptr, int word) {
	int read;
	if (sdna_read(sdna, rest, ptr, &read, 4, false)) {
		return read == word;
	}
	return false;
}
bool DNA_sdna_read_i16(const struct SDNA *sdna, const void **rest, const void *ptr, int16_t *word) {
	return sdna_read(sdna, rest, ptr, word, sizeof(*word), true);
}
bool DNA_sdna_read_i32(const struct SDNA *sdna, const void **rest, const void *ptr, int32_t *word) {
	return sdna_read(sdna, rest, ptr, word, sizeof(*word), true);
}
bool DNA_sdna_read_i64(const struct SDNA *sdna, const void **rest, const void *ptr, int64_t *word) {
	return sdna_read(sdna, rest, ptr, word, sizeof(*word), true);
}
bool DNA_sdna_read_u16(const struct SDNA *sdna, const void **rest, const void *ptr, uint16_t *word) {
	return sdna_read(sdna, rest, ptr, word, sizeof(*word), true);
}
bool DNA_sdna_read_u32(const struct SDNA *sdna, const void **rest, const void *ptr, uint32_t *word) {
	return sdna_read(sdna, rest, ptr, word, sizeof(*word), true);
}
bool DNA_sdna_read_u64(const struct SDNA *sdna, const void **rest, const void *ptr, uint64_t *word) {
	return sdna_read(sdna, rest, ptr, word, sizeof(*word), true);
}

bool DNA_sdna_read_ull(const struct SDNA *sdna, const void **rest, const void *ptr, unsigned long long *word) {
	uint64_t u64;
	if (DNA_sdna_read_u64(sdna, rest, ptr, &u64)) {
		*word = (unsigned long long)u64;
		return true;
	}
	return false;
}
bool DNA_sdna_read_bool(const struct SDNA *sdna, const void **rest, const void *ptr, bool *word) {
	int32_t value = 0;
	if (DNA_sdna_read_i32(sdna, rest, ptr, &value)) {
		*word = (value) ? true : false;
		return true;
	}
	return false;
}
bool DNA_sdna_read_str(const struct SDNA *sdna, const void **rest, const void *ptr, char *word) {
	size_t length = LIB_strnlen(ptr, 65);
	if (length <= 64) {
		return sdna_read(sdna, rest, ptr, word, length + 1, false);
	}
	return false;
}

bool DNA_sdna_read_type(const struct SDNA *sdna, const void **rest, const void *ptr, const RCCType **tp) {
	bool status = true;

	uint64_t addr = (uint64_t)NULL;
	status &= DNA_sdna_read_word(sdna, &ptr, ptr, MAKE_ID4('T', 'Y', 'P', 'E'));
	status &= DNA_sdna_read_u64(sdna, &ptr, ptr, &addr);
	if (LIB_ghash_haskey(sdna->visit, (void *)addr)) {
		*tp = LIB_ghash_lookup(sdna->visit, (void *)addr);
	}
	else {
		RCCType *ntp = RT_context_calloc(sdna->context, sizeof(RCCType));

		LIB_ghash_insert(sdna->visit, (void *)addr, ntp);

		status &= DNA_sdna_read_i32(sdna, &ptr, ptr, &ntp->kind);
		status &= DNA_sdna_read_bool(sdna, &ptr, ptr, &ntp->is_basic);

		if (ntp->is_basic) {
			status &= sdna_read_type_basic(sdna, &ptr, ptr, &ntp->tp_basic);
		}
		else {
			switch (ntp->kind) {
				case TP_ARRAY: {
					status &= sdna_read_type_array(sdna, &ptr, ptr, &ntp->tp_array);
				} break;
				case TP_PTR: {
					status &= sdna_read_type_ptr(sdna, &ptr, ptr, &ntp->tp_base);
				} break;
				case TP_STRUCT: {
					status &= sdna_read_type_struct(sdna, &ptr, ptr, &ntp->tp_struct);
				} break;
				default: {
					status &= false;
				} break;
			}
		}

		*tp = ntp;
	}
	if (status) {
		*rest = ptr;
	}
	return status;
}
bool DNA_sdna_read_token(const struct SDNA *sdna, const void **rest, const void *ptr, const RCCToken **tok) {
	bool status = true;

	{
		char identifier[65];
		status &= DNA_sdna_read_word(sdna, &ptr, ptr, MAKE_ID4('T', 'O', 'K', 'N'));
		status &= DNA_sdna_read_str(sdna, &ptr, ptr, identifier);
		*tok = RT_token_new_name(sdna->context, identifier);
	}
	if (status) {
		*rest = ptr;
	}
	return status;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool DNA_sdna_needs_endian_swap(const struct SDNA *sdna) {
	const void *ptr = sdna->data;
	if (!DNA_sdna_read_word(sdna, &ptr, ptr, MAKE_ID4('S', 'D', 'N', 'A'))) {
		ROSE_assert_msg(0, "Invalid SDNA data!");
	}
#ifdef __BIG_ENDIAN__
	if (DNA_sdna_read_word(sdna, &ptr, ptr, MAKE_ID4('B', 'I', 'G', 'E'))) {
		return true;
	}
	ROSE_assert(DNA_sdna_read_word(sdna, &ptr, POINTER_OFFSET(sdna->data, 4), MAKE_ID4('L', 'T', 'L', 'E')));
	return false;
#else
	if (DNA_sdna_read_word(sdna, &ptr, ptr, MAKE_ID4('L', 'T', 'L', 'E'))) {
		return true;
	}
	ROSE_assert(DNA_sdna_read_word(sdna, &ptr, POINTER_OFFSET(sdna->data, 4), MAKE_ID4('B', 'I', 'G', 'E')));
	return false;
#endif
}

bool DNA_sdna_check(const struct SDNA *sdna) {
	SDNA *ndna = DNA_sdna_new_memory(sdna->data, sdna->length);

	const void *ptr = POINTER_OFFSET(ndna->data, 8);
	while (ptr < POINTER_OFFSET(ndna->data, ndna->length)) {
		const RCCToken *token;
		if (!DNA_sdna_read_token(ndna, &ptr, ptr, &token)) {
			return false;
		}
		const RCCType *type;
		if (!DNA_sdna_read_type(ndna, &ptr, ptr, &type)) {
			return false;
		}
	}

	DNA_sdna_free(ndna);
	return true;
}

/** \} */
