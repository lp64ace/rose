#include <stdio.h>

#include "MEM_alloc.h"

#include "DNA_defines.h"
#include "DNA_genfile.h"
#include "DNA_utils.h"

#include "LIB_endian_switch.h"

enum {
	WORD_ENDIANESS_SAME = 0,
	WORD_ENDIANESS_NEEDS_SWAP = 1,
	WORD_ENDIANESS_ERROR = -1,
};

static int endianess_detect(int word, int expected) {
	if(word == expected) {
		return WORD_ENDIANESS_SAME;
	}
	LIB_endian_switch_int32(&word);
	if(word == expected) {
		return WORD_ENDIANESS_NEEDS_SWAP;
	}
	return WORD_ENDIANESS_ERROR;
}

static int _intern_skip_int32_and_fix_endianess(int *store, const char **iter, bool swap_endian) {
	*store = *(const int *)(*iter);
	if(swap_endian) {
		LIB_endian_switch_int32(store);
	}
	*iter += sizeof(*store);
	return *store;
}

static const char *_intern_skip_string_and_fix_endianess(char *store, const char **iter, bool swap_endian) {
	size_t length;
	for(length = 0; (*iter)[length] != '\0'; length++) {
		store[length] = (*iter)[length];
	}
	store[length++] = '\0'; /** Include NULL terminator */
	*iter += sizeof(*store) * length;
	return store;
}

static bool init_structDNA(SDNA *sdna, const char **r_error_message) {
	int data = *(const int *)sdna->data, word = 'SDNA';
	
	sdna->types_len = 0;
	sdna->types = NULL;
	
	int endianess = endianess_detect(data, word);
	if(endianess == WORD_ENDIANESS_ERROR) {
		*r_error_message = "Invalid SDNA data, identifier word is missing";
		return false;
	}
	
	/** Flag this #SDNA that we had to swap the endianess of the data. */
	sdna->flags |= (endianess == WORD_ENDIANESS_NEEDS_SWAP) ? SDNA_ENDIAN_SWAP : 0;
	
	const char *cp = sdna->data + sizeof(word);
	if(_intern_skip_int32_and_fix_endianess(&sdna->types_len, &cp, endianess == WORD_ENDIANESS_NEEDS_SWAP) > 0) {
		sdna->types = MEM_callocN(sizeof(*sdna->types) * sdna->types_len, "SDNA->types");
	} else {
		*r_error_message = "Unkown number of types in SDNA data";
		return false;
	}
	
	for(int ntype = 0; ntype < sdna->types_len; ntype++) {
		DNAStruct *type = &sdna->types[ntype];
		_intern_skip_string_and_fix_endianess(type->name, &cp, endianess == WORD_ENDIANESS_NEEDS_SWAP);
		
		_intern_skip_int32_and_fix_endianess(&type->size, &cp, endianess == WORD_ENDIANESS_NEEDS_SWAP);
		_intern_skip_int32_and_fix_endianess(&type->align, &cp, endianess == WORD_ENDIANESS_NEEDS_SWAP);
		
		if(_intern_skip_int32_and_fix_endianess(&type->fields_len, &cp, endianess == WORD_ENDIANESS_NEEDS_SWAP) < 0) {
			*r_error_message = "Unkown number of fields in struct";
			return false;
		}

		type->fields = MEM_mallocN(sizeof(DNAField) * type->fields_len, "DNAStruct->fields");
		
		for(int nfield = 0; nfield < type->fields_len; nfield++) {
			DNAField *field = &type->fields[nfield];
			
			_intern_skip_string_and_fix_endianess(field->name, &cp, endianess == WORD_ENDIANESS_NEEDS_SWAP);
			_intern_skip_string_and_fix_endianess(field->type, &cp, endianess == WORD_ENDIANESS_NEEDS_SWAP);
			
			_intern_skip_int32_and_fix_endianess(&field->offset, &cp, endianess == WORD_ENDIANESS_NEEDS_SWAP);
			_intern_skip_int32_and_fix_endianess(&field->size, &cp, endianess == WORD_ENDIANESS_NEEDS_SWAP);
			_intern_skip_int32_and_fix_endianess(&field->align, &cp, endianess == WORD_ENDIANESS_NEEDS_SWAP);
			_intern_skip_int32_and_fix_endianess(&field->array, &cp, endianess == WORD_ENDIANESS_NEEDS_SWAP);
			
			_intern_skip_int32_and_fix_endianess(&field->flags, &cp, endianess == WORD_ENDIANESS_NEEDS_SWAP);
		}
	}
	
	return true;
}

struct SDNA *DNA_sdna_from_data(const void *data, int data_len, bool data_alloc, const char **r_error_message) {
	SDNA *sdna = MEM_mallocN(sizeof(*sdna), "SDNA");
	const char *error_message = NULL;

	sdna->data_len = data_len;
	if (data_alloc) {
		char *data_copy = MEM_mallocN(data_len, "SDNA->data");
		memcpy(data_copy, data, data_len);
		sdna->data = data_copy;
	}
	else {
		sdna->data = data;
	}
	sdna->data_alloc = data_alloc;

	if (init_structDNA(sdna, &error_message)) {
		return sdna;
	}

	if (r_error_message == NULL) {
		fprintf(stderr, "Error decoding rose file SDNA: %s\n", error_message);
	}
	else {
		*r_error_message = error_message;
	}
	DNA_sdna_free(sdna);
	return NULL;
}

static SDNA *g_SDNA = NULL;

void DNA_sdna_current_init(void) {
	g_SDNA = DNA_sdna_from_data(DNAstr, DNAlen, false, NULL);
}
void DNA_sdna_current_free(void) {
	DNA_sdna_free(g_SDNA);
	g_SDNA = NULL;
}

const struct SDNA *DNA_sdna_current_get(void) {
	ROSE_assert(g_SDNA != NULL);
	return g_SDNA;
}
