#include "MEM_guardedalloc.h"

#include "LIB_endian_switch.h"
#include "LIB_ghash.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "RT_context.h"
#include "RT_parser.h"
#include "RT_token.h"

#include "genfile.h"

#include <stdio.h>

#ifdef __BIG_ENDIAN__
/* Big Endian */
#	define MAKE_ID4(a, b, c, d) ((int)(a) << 24 | (int)(b) << 16 | (int)(c) << 8 | (int)(d))
#else
/* Little Endian */
#	define MAKE_ID4(a, b, c, d) ((int)(d) << 24 | (int)(c) << 16 | (int)(b) << 8 | (int)(a))
#endif

#define DATA_GROW 1024

/* -------------------------------------------------------------------- */
/** \name Internal Util Methods
 * \{ */

ROSE_INLINE bool sdna_grow(SDNA *sdna, size_t nlength) {
	while (sdna->allocated <= nlength) {
		sdna->allocated = sdna->allocated + ROSE_MAX(sdna->allocated, nlength);
		if (!(sdna->data = MEM_reallocN(sdna->data, sdna->allocated))) {
			return false;
		}
	}
	return true;
}

ROSE_INLINE bool sdna_write(SDNA *sdna, void **rest, void *ptr, const void *mem, size_t length, bool fix) {
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
ROSE_INLINE bool sdna_read(const SDNA *sdna, const void **rest, const void *ptr, void *mem, size_t length, bool fix) {
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

ROSE_INLINE bool sdna_write_type_array(SDNA *sdna, void **rest, void *ptr, const RCCTypeArray *array) {
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
ROSE_INLINE bool sdna_read_type_array(const SDNA *sdna, const void **rest, const void *ptr, RCCTypeArray *array) {
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

ROSE_INLINE bool sdna_write_type_basic(SDNA *sdna, void **rest, void *ptr, const RCCTypeBasic *basic) {
	bool status = true;
	status &= DNA_sdna_write_bool(sdna, &ptr, ptr, basic->is_unsigned);
	status &= DNA_sdna_write_i32(sdna, &ptr, ptr, basic->rank);
	if (status) {
		*rest = ptr;
	}
	return status;
}
ROSE_INLINE bool sdna_read_type_basic(const SDNA *sdna, const void **rest, const void *ptr, RCCTypeBasic *basic) {
	bool status = true;
	status &= DNA_sdna_read_bool(sdna, &ptr, ptr, &basic->is_unsigned);
	status &= DNA_sdna_read_i32(sdna, &ptr, ptr, &basic->rank);
	if (status) {
		*rest = ptr;
	}
	return status;
}

ROSE_INLINE bool sdna_write_type_ptr(SDNA *sdna, void **rest, void *ptr, const RCCType *base) {
	return DNA_sdna_write_type(sdna, rest, ptr, base);
}
ROSE_INLINE bool sdna_read_type_ptr(const SDNA *sdna, const void **rest, const void *ptr, const RCCType **base) {
	return DNA_sdna_read_type(sdna, rest, ptr, base);
}

ROSE_INLINE bool sdna_write_type_qual(SDNA *sdna, void **rest, void *ptr, const RCCTypeQualified *qual) {
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
ROSE_INLINE bool sdna_read_type_qual(const SDNA *sdna, const void **rest, const void *ptr, RCCTypeQualified *qual) {
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

ROSE_INLINE bool sdna_write_type_struct(SDNA *sdna, void **rest, void *ptr, const RCCTypeStruct *strct) {
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
ROSE_INLINE bool sdna_read_type_struct(const SDNA *sdna, const void **rest, const void *ptr, RCCTypeStruct *strct) {
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
	DNA_sdna_write_word(sdna, &ptr, ptr, MAKE_ID4('E', 'N', 'D', 'N'));
#else
	DNA_sdna_write_word(sdna, &ptr, ptr, MAKE_ID4('e', 'n', 'd', 'n'));
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

bool DNA_sdna_write_word(SDNA *sdna, void **rest, void *ptr, int word) {
	return sdna_write(sdna, rest, ptr, &word, 4, false);
}
bool DNA_sdna_write_i16(SDNA *sdna, void **rest, void *ptr, int16_t word) {
	return sdna_write(sdna, rest, ptr, &word, sizeof(word), true);
}
bool DNA_sdna_write_i32(SDNA *sdna, void **rest, void *ptr, int32_t word) {
	return sdna_write(sdna, rest, ptr, &word, sizeof(word), true);
}
bool DNA_sdna_write_i64(SDNA *sdna, void **rest, void *ptr, int64_t word) {
	return sdna_write(sdna, rest, ptr, &word, sizeof(word), true);
}
bool DNA_sdna_write_u16(SDNA *sdna, void **rest, void *ptr, uint16_t word) {
	return sdna_write(sdna, rest, ptr, &word, sizeof(word), true);
}
bool DNA_sdna_write_u32(SDNA *sdna, void **rest, void *ptr, uint32_t word) {
	return sdna_write(sdna, rest, ptr, &word, sizeof(word), true);
}
bool DNA_sdna_write_u64(SDNA *sdna, void **rest, void *ptr, uint64_t word) {
	return sdna_write(sdna, rest, ptr, &word, sizeof(word), true);
}

bool DNA_sdna_write_ull(SDNA *sdna, void **rest, void *ptr, unsigned long long word) {
	uint64_t u64 = (uint64_t)word;
	return DNA_sdna_write_u64(sdna, rest, ptr, u64);
}
bool DNA_sdna_write_bool(SDNA *sdna, void **rest, void *ptr, bool word) {
	return DNA_sdna_write_i32(sdna, rest, ptr, (word) ? 1 : 0);
}
bool DNA_sdna_write_str(SDNA *sdna, void **rest, void *ptr, const char *word) {
	size_t length = LIB_strnlen(word, 65);
	if (length <= 64) {
		return sdna_write(sdna, rest, ptr, word, length + 1, false);
	}
	return false;
}

bool DNA_sdna_write_type(SDNA *sdna, void **rest, void *ptr, const RCCType *tp) {
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
bool DNA_sdna_write_token(SDNA *sdna, void **rest, void *ptr, const RCCToken *tok) {
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

bool DNA_sdna_read_word(const SDNA *sdna, const void **rest, const void *ptr, int word) {
	int read;
	if (sdna_read(sdna, rest, ptr, &read, 4, false)) {
		return read == word;
	}
	return false;
}
bool DNA_sdna_read_i16(const SDNA *sdna, const void **rest, const void *ptr, int16_t *word) {
	return sdna_read(sdna, rest, ptr, word, sizeof(*word), true);
}
bool DNA_sdna_read_i32(const SDNA *sdna, const void **rest, const void *ptr, int32_t *word) {
	return sdna_read(sdna, rest, ptr, word, sizeof(*word), true);
}
bool DNA_sdna_read_i64(const SDNA *sdna, const void **rest, const void *ptr, int64_t *word) {
	return sdna_read(sdna, rest, ptr, word, sizeof(*word), true);
}
bool DNA_sdna_read_u16(const SDNA *sdna, const void **rest, const void *ptr, uint16_t *word) {
	return sdna_read(sdna, rest, ptr, word, sizeof(*word), true);
}
bool DNA_sdna_read_u32(const SDNA *sdna, const void **rest, const void *ptr, uint32_t *word) {
	return sdna_read(sdna, rest, ptr, word, sizeof(*word), true);
}
bool DNA_sdna_read_u64(const SDNA *sdna, const void **rest, const void *ptr, uint64_t *word) {
	return sdna_read(sdna, rest, ptr, word, sizeof(*word), true);
}

bool DNA_sdna_read_ull(const SDNA *sdna, const void **rest, const void *ptr, unsigned long long *word) {
	uint64_t u64;
	if (DNA_sdna_read_u64(sdna, rest, ptr, &u64)) {
		*word = (unsigned long long)u64;
		return true;
	}
	return false;
}
bool DNA_sdna_read_bool(const SDNA *sdna, const void **rest, const void *ptr, bool *word) {
	int32_t value = 0;
	if (DNA_sdna_read_i32(sdna, rest, ptr, &value)) {
		*word = (value) ? true : false;
		return true;
	}
	return false;
}
bool DNA_sdna_read_str(const SDNA *sdna, const void **rest, const void *ptr, char *word) {
	size_t length = LIB_strnlen(ptr, 65);
	if (length <= 64) {
		return sdna_read(sdna, rest, ptr, word, length + 1, false);
	}
	return false;
}

bool DNA_sdna_read_type(const SDNA *sdna, const void **rest, const void *ptr, const RCCType **tp) {
	bool status = true;

	uint64_t addr = (uint64_t)NULL;
	status &= DNA_sdna_read_word(sdna, &ptr, ptr, MAKE_ID4('T', 'Y', 'P', 'E'));
	status &= DNA_sdna_read_u64(sdna, &ptr, ptr, &addr);
	if (LIB_ghash_haskey(sdna->visit, (void *)addr)) {
		*tp = LIB_ghash_lookup(sdna->visit, (void *)addr);
	}
	else {
		RCCType *ntp = NULL;

		int kind;
		status &= DNA_sdna_read_i32(sdna, &ptr, ptr, &kind);

		switch (kind) {
			case TP_ARRAY: {
				ntp = RT_type_new_empty_array(sdna->context);
			} break;
			case TP_PTR: {
				ntp = RT_type_new_empty_pointer(sdna->context);
			} break;
			case TP_STRUCT: {
				ntp = RT_type_new_struct(sdna->context, NULL);
			} break;
			default: {
				ntp = RT_type_new_empty_basic(sdna->context);
			} break;
		}

		ntp->kind = kind;

		LIB_ghash_insert(sdna->visit, (void *)addr, ntp);

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
					RT_type_struct_finalize(ntp);
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
bool DNA_sdna_read_token(const SDNA *sdna, const void **rest, const void *ptr, const RCCToken **tok) {
	bool status = true;

	{
		char identifier[65];
		status &= DNA_sdna_read_word(sdna, &ptr, ptr, MAKE_ID4('T', 'O', 'K', 'N'));
		status &= DNA_sdna_read_str(sdna, &ptr, ptr, identifier);
		*tok = RT_token_new_virtual_identifier(sdna->context, identifier);
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

bool DNA_sdna_build_struct_list(const SDNA *sdna) {
	bool status = true;

	const void *ptr = POINTER_OFFSET(sdna->data, 8);
	while (POINTER_OFFSET(ptr, 4) < POINTER_OFFSET(sdna->data, sdna->length)) {
		const RCCToken *token;
		if (!DNA_sdna_read_token(sdna, &ptr, ptr, &token)) {
			status &= false;
			break;
		}
		const RCCType *type;
		if (!DNA_sdna_read_type(sdna, &ptr, ptr, &type)) {
			status &= false;
			break;
		}
		LIB_ghash_insert(sdna->types, (void *)RT_token_as_string(token), (void *)type);
	}

	return status;
}

bool DNA_sdna_needs_endian_swap(const SDNA *sdna) {
	const void *ptr = sdna->data;
	if (!DNA_sdna_read_word(sdna, &ptr, ptr, MAKE_ID4('S', 'D', 'N', 'A'))) {
		ROSE_assert_msg(0, "Invalid SDNA data!");
	}
#ifdef __BIG_ENDIAN__
	if (DNA_sdna_read_word(sdna, &ptr, ptr, MAKE_ID4('E', 'N', 'D', 'N'))) {
		return true;
	}
	ROSE_assert(DNA_sdna_read_word(sdna, &ptr, POINTER_OFFSET(sdna->data, 4), MAKE_ID4('e', 'n', 'd', 'n')));
	return false;
#else
	if (DNA_sdna_read_word(sdna, &ptr, ptr, MAKE_ID4('e', 'n', 'd', 'n'))) {
		return true;
	}
	ROSE_assert(DNA_sdna_read_word(sdna, &ptr, POINTER_OFFSET(sdna->data, 4), MAKE_ID4('E', 'N', 'D', 'N')));
	return false;
#endif
}

ROSE_STATIC void dna_struct_switch_endian_ex(const SDNA *sdna, const RCCType *type, void *data) {
	if (type->is_basic) {
		LIB_endian_switch_rank(data, type->tp_basic.rank);
		return;
	}

	switch (type->kind) {
		case TP_ENUM: {
			const RCCType *tp_enum = LIB_ghash_lookup(sdna->types, "conf::tp_size");
			if (type->tp_enum.underlying_type) {
				tp_enum = type->tp_enum.underlying_type;
			}
			dna_struct_switch_endian_ex(sdna, tp_enum, data);
		} break;
		case TP_PTR: {
			const RCCType *tp_size = LIB_ghash_lookup(sdna->types, "conf::tp_size");
			ROSE_assert(tp_size->is_basic);
			LIB_endian_switch_rank(data, type->tp_basic.rank);
		} break;
		case TP_ARRAY: {
			for (size_t index = 0; index < type->tp_array.length; index++) {
				dna_struct_switch_endian_ex(sdna, type->tp_array.element_type, data);
			}
		} break;
		case TP_STRUCT: {
			RCCParser *parser = RT_parser_new(NULL);
			parser->configuration.tp_size = LIB_ghash_lookup(sdna->types, "conf::tp_size");
			parser->configuration.tp_enum = LIB_ghash_lookup(sdna->types, "conf::tp_enum");

			LISTBASE_FOREACH(RCCField *, field, &type->tp_struct.fields) {
				size_t offset = RT_parser_offsetof(parser, type, field);

				dna_struct_switch_endian_ex(sdna, type, POINTER_OFFSET(data, offset));
			}

			RT_parser_free(parser);
		} break;
		default: {
			ROSE_assert_unreachable();
		} break;
	}
}

void DNA_struct_switch_endian(const SDNA *sdna, uint64_t struct_nr, void *data) {
	const RCCType *type = LIB_ghash_lookup(sdna->visit, (void *)struct_nr);
	if (!type) {
		return;
	}

	dna_struct_switch_endian_ex(sdna, type, data);
}

ROSE_STATIC ptrdiff_t dna_find_member_offset(const SDNA *sdna, const RCCType *type, const RCCField *field) {
	RCCParser *parser = RT_parser_new(NULL);
	parser->configuration.tp_size = LIB_ghash_lookup(sdna->types, "conf::tp_size");
	parser->configuration.tp_enum = LIB_ghash_lookup(sdna->types, "conf::tp_enum");
	ptrdiff_t offset = (ptrdiff_t)RT_parser_offsetof(parser, type, field);
	RT_parser_free(parser);

	return offset;
}

ROSE_STATIC size_t dna_find_type_size(const SDNA *sdna, const RCCType *type) {
	RCCParser *parser = RT_parser_new(NULL);
	parser->configuration.tp_size = LIB_ghash_lookup(sdna->types, "conf::tp_size");
	parser->configuration.tp_enum = LIB_ghash_lookup(sdna->types, "conf::tp_enum");
	size_t size = (size_t)RT_parser_size(parser, type);
	RT_parser_free(parser);

	return size;
}

size_t DNA_sdna_struct_size(const SDNA *sdna, uint64_t struct_nr) {
	const RCCType *type = LIB_ghash_lookup(sdna->visit, (void *)struct_nr);
	if (!type) {
		return 0;
	}

	return dna_find_type_size(sdna, type);
}

ptrdiff_t DNA_sdna_field_offset(const struct SDNA *sdna, uint64_t struct_nr, const char *fieldname) {
	const RCCType *type = LIB_ghash_lookup(sdna->visit, (void *)struct_nr);
	if (!type || type->kind != TP_STRUCT) {
		ROSE_assert_msg(0, "Invalid struct provided for #DNA_sdna_field_offset.");
		return 0;
	}

	LISTBASE_FOREACH(RCCField *, field, &type->tp_struct.fields) {
		if (STREQ(RT_token_as_string(field->identifier), fieldname)) {
			return dna_find_member_offset(sdna, type, field);
		}
	}

	ROSE_assert_msg(0, "Invalid field-name provided for #DNA_sdna_field_offset.");
	return 0;
}

uint64_t DNA_sdna_struct_ex(const SDNA *sdna, const struct RCCType *type) {
	GHashIterator iter;
	GHASH_ITER(iter, sdna->visit) {
		if (LIB_ghashIterator_getValue(&iter) == type) {
			return (uint64_t)LIB_ghashIterator_getKey(&iter);
		}
	}
	return 0;
}

uint64_t DNA_sdna_struct_id(const SDNA *sdna, const char *name) {
	const RCCType *type = LIB_ghash_lookup(sdna->types, name);
	if (type) {
		return DNA_sdna_struct_ex(sdna, type);
	}
	return 0;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Reconstruction Methods
 * \{ */

typedef struct ReconstructStep {
	int type;
	ptrdiff_t offset_old;
	ptrdiff_t offset_new;
	union {
		struct {
			size_t size;
		} memcpy;
		struct {
			size_t length;
			const RCCType *type_old;
			const RCCType *type_new;
		} cast;
		struct {
			size_t size_old;
			size_t size_new;
			size_t length;
			size_t steps;
			struct ReconstructStep *info;
		} reconstruct;
	};
} ReconstructStep;

enum {
	RECONSTRUCT_STEP_INIT_ZERO,
	RECONSTRUCT_STEP_MEMCPY,
	RECONSTRUCT_STEP_CAST_PRIMITIVE,
	RECONSTRUCT_STEP_RECONSTRUCT,
};

ROSE_STATIC const RCCType *dna_find_struct_with_matching_name(const SDNA *sdna, const char *name) {
	const RCCType *match = LIB_ghash_lookup(sdna->types, (void *)name);
	if (match->kind == TP_STRUCT) {
		return match;
	}
	return NULL;
}

ROSE_STATIC const RCCField *dna_find_member_with_matching_name(const SDNA *sdna, const RCCType *type, const char *name) {
	LISTBASE_FOREACH(RCCField *, field, &type->tp_struct.fields) {
		if (STREQ(RT_token_as_string(field->identifier), name)) {
			return field;
		}
	}
	return NULL;
}

ROSE_STATIC void dna_init_reconstruct_step_for_member(const SDNA *dna_old, const SDNA *dna_new, const RCCType *struct_old, const RCCField *field_new, ReconstructStep *r_step) {
	const RCCType *struct_new = dna_find_struct_with_matching_name(dna_new, RT_token_as_string(struct_old->tp_struct.identifier));
	const RCCField *field_old = dna_find_member_with_matching_name(dna_old, struct_old, RT_token_as_string(field_new->identifier));

	fprintf(stdout, "reconstruct info | struct: %s(new), %s(old) | ", RT_token_as_string(struct_new->tp_struct.identifier), RT_token_as_string(struct_old->tp_struct.identifier));
	fprintf(stdout, "field: %s(new), %s(old) | ", RT_token_as_string(field_new->identifier), (field_old) ? RT_token_as_string(field_old->identifier) : "(null)");

	if (!field_old) {
		/** Could not find an old member to copy the data to the new member, init to zero! */
		r_step->type = RECONSTRUCT_STEP_INIT_ZERO;
		fprintf(stdout, "zero\n");
		return;
	}

	const RCCType *type_old = RT_type_unqualified(field_old->type);
	const RCCType *type_new = RT_type_unqualified(field_new->type);

	const size_t narr_len = (type_new->kind == TP_ARRAY) ? RT_type_array_length(type_new) : 1;
	const size_t oarr_len = (type_old->kind == TP_ARRAY) ? RT_type_array_length(type_old) : 1;
	const size_t carr_len = ROSE_MIN(narr_len, oarr_len);

	if (type_new->kind == TP_ARRAY && type_old->kind == TP_ARRAY) {
		type_old = RT_type_unqualified(type_old->tp_array.element_type);
		type_new = RT_type_unqualified(type_new->tp_array.element_type);
	}

	if (type_new->kind == TP_ENUM && type_old->kind == TP_ENUM) {
		type_old = RT_type_unqualified(type_old->tp_enum.underlying_type);
		type_new = RT_type_unqualified(type_new->tp_enum.underlying_type);
	}

	if (type_old->kind == TP_PTR && type_new->kind == TP_PTR) {
		/**
		 * TODO: This allows converting files from x32 to x64 systems but causes issues
		 * when converting files from x64 to x32 since collisions may occur!
		 */
		type_old = LIB_ghash_lookup(dna_old->types, "conf::tp_size");
		type_new = LIB_ghash_lookup(dna_new->types, "conf::tp_size");
	}

	if (RT_type_same(type_new, type_old)) {
		r_step->type = RECONSTRUCT_STEP_MEMCPY;
		r_step->offset_old = dna_find_member_offset(dna_old, struct_old, field_old);
		r_step->offset_new = dna_find_member_offset(dna_old, struct_new, field_new);
		r_step->memcpy.size = dna_find_type_size(dna_new, type_new) * carr_len;
		fprintf(stdout, "memcpy [%zd <- %zd x %zu]\n", r_step->offset_new, r_step->offset_old, carr_len);
		return;
	}

	if (type_new->is_basic && type_old->is_basic) {
		r_step->type = RECONSTRUCT_STEP_CAST_PRIMITIVE;
		r_step->offset_old = dna_find_member_offset(dna_old, struct_old, field_old);
		r_step->offset_new = dna_find_member_offset(dna_old, struct_new, field_new);
		r_step->cast.length = carr_len;
		r_step->cast.type_old = type_old;
		r_step->cast.type_new = type_new;
		fprintf(stdout, "cast [%zd <- %zd x %zu]\n", r_step->offset_new, r_step->offset_old, carr_len);
		return;
	}

	if (type_new->kind == TP_STRUCT && type_old->kind == TP_STRUCT) {
		size_t nfields = LIB_listbase_count(&type_new->tp_struct.fields);

		r_step->type = RECONSTRUCT_STEP_RECONSTRUCT;
		r_step->offset_old = dna_find_member_offset(dna_old, struct_old, field_old);
		r_step->offset_new = dna_find_member_offset(dna_old, struct_new, field_new);
		r_step->reconstruct.size_new = dna_find_type_size(dna_old, type_old);
		r_step->reconstruct.size_old = dna_find_type_size(dna_new, type_new);
		r_step->reconstruct.length = carr_len;
		r_step->reconstruct.steps = nfields;
		r_step->reconstruct.info = MEM_mallocN(sizeof(ReconstructStep) * nfields, "ReconstructStep[]");
		fprintf(stdout, "reconstruct [%zd <- %zd x %zu]\n", r_step->offset_new, r_step->offset_old, carr_len);

		size_t index;
		LISTBASE_FOREACH_INDEX(RCCField *, field, &type_new->tp_struct.fields, index) {
			dna_init_reconstruct_step_for_member(dna_old, dna_new, type_old, field, &r_step->reconstruct.info[index]);
		}
		return;
	}

	fprintf(stdout, "zero\n");
	r_step->type = RECONSTRUCT_STEP_INIT_ZERO;
}

/** TODO: Add a lookup table that stores all the reconstruction steps for each struct so this doesn't run every time! */
ROSE_STATIC ReconstructStep *dna_create_reconstruct_step_for_struct(const SDNA *dna_old, const SDNA *dna_new, const RCCType *struct_old, const RCCType *struct_new) {
	size_t nfields = LIB_listbase_count(&struct_new->tp_struct.fields);
	ReconstructStep *r_step = MEM_mallocN(sizeof(ReconstructStep), "ReconstructStep");

	if (RT_type_same(struct_new, struct_old)) {
		r_step->type = RECONSTRUCT_STEP_MEMCPY;
		r_step->offset_old = 0;
		r_step->offset_new = 0;
		r_step->memcpy.size = dna_find_type_size(dna_old, struct_new);
	}
	else {
		r_step->type = RECONSTRUCT_STEP_RECONSTRUCT;
		r_step->offset_old = 0;
		r_step->offset_new = 0;
		r_step->reconstruct.size_old = dna_find_type_size(dna_old, struct_old);
		r_step->reconstruct.size_new = dna_find_type_size(dna_new, struct_new);
		r_step->reconstruct.length = 1;
		r_step->reconstruct.steps = nfields;
		r_step->reconstruct.info = MEM_mallocN(sizeof(ReconstructStep) * nfields, "ReconstructStep[]");

		size_t index;
		LISTBASE_FOREACH_INDEX(RCCField *, field, &struct_new->tp_struct.fields, index) {
			dna_init_reconstruct_step_for_member(dna_old, dna_new, struct_old, field, &r_step->reconstruct.info[index]);
		}
	}

	return r_step;
}

ROSE_STATIC void memcast(void *ptr_a, const void *ptr_b, size_t length, const RCCType *told, const RCCType *tnew) {
#define CAST(cast_new, cast_old)                                                                  \
	do {                                                                                          \
		if (tnew->tp_basic.rank == sizeof(cast_new) && told->tp_basic.rank == sizeof(cast_old)) { \
			if (!tnew->tp_basic.is_unsigned && !told->tp_basic.is_unsigned) {                     \
				cast_new *value_new = (cast_new *)ptr_a;                                          \
				cast_old *value_old = (cast_old *)ptr_b;                                          \
				value_new[index] = value_old[index];                                              \
				continue;                                                                         \
			}                                                                                     \
			if (tnew->tp_basic.is_unsigned && told->tp_basic.is_unsigned) {                       \
				unsigned cast_new *value_new = (unsigned cast_new *)ptr_a;                        \
				unsigned cast_old *value_old = (unsigned cast_old *)ptr_b;                        \
				value_new[index] = value_old[index];                                              \
				continue;                                                                         \
			}                                                                                     \
		}                                                                                         \
	} while (false)

	for (size_t index = 0; index < length; index++) {
		/**
		 * NOTE: We do not allow casting between bool and integer types
		 * since that would have problematic effects when casting flags.
		 *
		 * The main purpose of this conversion/casting is to tackle the
		 * issue with sizeof(long long) (Windows) != sizeof(long long) (Linux)
		 * This will handle conversions between the same signedness and
		 * different size!
		 */

		const bool new_is_integer = ELEM(tnew->kind, TP_CHAR, TP_SHORT, TP_LONG, TP_LLONG);
		const bool old_is_integer = ELEM(told->kind, TP_CHAR, TP_SHORT, TP_LONG, TP_LLONG);

		if (new_is_integer && old_is_integer) {
			CAST(char, char);
			CAST(char, short);
			CAST(char, int);
			CAST(char, long);
			CAST(char, long long);

			CAST(short, char);
			CAST(short, short);
			CAST(short, int);
			CAST(short, long);
			CAST(short, long long);

			CAST(int, char);
			CAST(int, short);
			CAST(int, int);
			CAST(int, long);
			CAST(int, long long);

			CAST(long, char);
			CAST(long, short);
			CAST(long, int);
			CAST(long, long);
			CAST(long, long long);

			CAST(long long, char);
			CAST(long long, short);
			CAST(long long, int);
			CAST(long long, long);
			CAST(long long, long long);
		}

		/**
		 * NOTE: We do not allow casting between float and integer types
		 * since that would have problemati c effects when casting colors,
		 * e.g. When trying to cast an unsigned char [3] to float [3] we
		 * would get unormalized values that would not be usable when
		 * drawing colors!
		 */

		const bool new_is_floating = ELEM(tnew->kind, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE);
		const bool old_is_floating = ELEM(told->kind, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE);

		if (new_is_floating && old_is_floating) {
			// For now floating point conversions are disabled, no need!
		}

		fprintf(stderr, "[DNA] Warning, no casting for");
		fprintf(stderr, " %s%s[%d bytes] =", tnew->tp_basic.is_unsigned ? "u" : "", new_is_floating ? "flt" : "int", tnew->tp_basic.rank);
		fprintf(stderr, " %s%s[%d bytes]\n", told->tp_basic.is_unsigned ? "u" : "", old_is_floating ? "flt" : "int", told->tp_basic.rank);
	}

#undef CAST_ANY
#undef CAST
#undef CAST_EX
}

ROSE_STATIC void dna_reconstruct_struct(void *data_new, const void *data_old, ReconstructStep *step) {
	switch (step->type) {
		case RECONSTRUCT_STEP_INIT_ZERO: {
		} break;
		case RECONSTRUCT_STEP_MEMCPY: {
			memcpy(POINTER_OFFSET(data_new, step->offset_new), POINTER_OFFSET(data_old, step->offset_old), step->memcpy.size);
		} break;
		case RECONSTRUCT_STEP_CAST_PRIMITIVE: {
			/**
			 * Not really able to used templates in C, this code is responsible for handling
			 * trivial casting between types.
			 *
			 * An equivalent in C++ would be something like this:
			 *
			 * template<typename A, typename B> void cast_single(A *, const B *) {
			 *   *A = *B;
			 * }
			 *
			 * void cast_single_ex(const RCCType *A, const RCCType *B, void *ptr_a, const void *ptr_b) {
			 *   using a = std::tuple_element<A->kind, std::tuple<void, bool, char, short, int, long, long long, float, double, long double>>;
			 *   using b = std::tuple_element<B->kind, std::tuple<void, bool, char, short, int, long, long long, float, double, long double>>;
			 *   cast_single<a, b>(static_cast<a *>(ptr_a), static_cast<b *>(ptr_b));
			 * }
			 *
			 * ...
			 *
			 * void *ptr_a = POINTER_OFFSET(data_new, step->offset_new);
			 * void *ptr_b = POINTER_OFFSET(data_old, step->offset_old);
			 * cast_array_ex(step->cast.type_new, step->cast.type_old, ptr_a, ptr_b, step->cast.length);
			 *
			 * I am not too sure if this is even possible given the fact that #A->kind and #B->kind will not be a compile-time constant,
			 * so I didn't bother remaking this file in C++ as it would probably require a huge amount of effort to make this cleaner...,
			 * I will probably revisit this implementation in order to fix it at some point...!
			 */
			const RCCType *told = step->cast.type_old;
			const RCCType *tnew = step->cast.type_new;
			memcast(POINTER_OFFSET(data_new, step->offset_new), POINTER_OFFSET(data_old, step->offset_old), step->cast.length, told, tnew);
		} break;
		case RECONSTRUCT_STEP_RECONSTRUCT: {
			for (size_t index = 0; index < step->reconstruct.length; index++) {
				for (size_t field = 0; field < step->reconstruct.steps; field++) {
					void *a = POINTER_OFFSET(data_new, step->offset_new + index * step->reconstruct.size_new);
					void *b = POINTER_OFFSET(data_old, step->offset_old + index * step->reconstruct.size_old);
					dna_reconstruct_struct(a, b, &step->reconstruct.info[field]);
				}
			}
		} break;
	}
}

ROSE_STATIC void dna_reconstruct_free_children(ReconstructStep *step) {
	if (step->type == RECONSTRUCT_STEP_RECONSTRUCT) {
		for (size_t field = 0; field < step->reconstruct.steps; field++) {
			dna_reconstruct_free_children(&step->reconstruct.info[field]);
		}
		MEM_freeN(step->reconstruct.info);
	}
}

ROSE_STATIC void dna_reconstruct_free_step(ReconstructStep *step) {
	dna_reconstruct_free_children(step);
	MEM_freeN(step);
}

void *DNA_sdna_struct_reconstruct(const SDNA *dna_old, const SDNA *dna_new, uint64_t struct_nr, const void *data_old, const char *blockname) {
	const RCCType *struct_old = LIB_ghash_lookup(dna_old->visit, (void *)struct_nr);
	if (!struct_old) {
		fprintf(stderr, "Invalid reconstruct for struct %p.\n", (void *)struct_nr);
		return NULL;
	}
	const RCCType *struct_new = dna_find_struct_with_matching_name(dna_new, RT_token_as_string(struct_old->tp_struct.identifier));
	if (!struct_old) {
		fprintf(stderr, "Invalid reconstruct for struct %s, missing equivalent.\n", RT_token_as_string(struct_old->tp_struct.identifier));
		return NULL;
	}

	void *data_new = MEM_callocN(dna_find_type_size(dna_new, struct_new), blockname);

	ReconstructStep *step = dna_create_reconstruct_step_for_struct(dna_old, dna_new, struct_old, struct_new);
	dna_reconstruct_struct(data_new, data_old, step);
	dna_reconstruct_free_step(step);

	return data_new;
}

/** \} */
