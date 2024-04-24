#include "MEM_alloc.h"

#include "dna_utils.h"

#include <string.h>

struct SDNA *DNA_sdna_alloc(void) {
	struct SDNA *sdna = MEM_callocN(sizeof(SDNA), "SDNA");
	
	return sdna;
}

void DNA_sdna_free(struct SDNA *sdna) {
	if(sdna->data_alloc) {
		MEM_SAFE_FREE(sdna->data);
	}
	for(struct DNAStruct *s = sdna->types; s != sdna->types + sdna->types_len; s++) {
		for(struct DNAStruct *f = s->fields; f != s->fields + s->fields_len; f++) {
			/** Nothing to do here. */
		}
		MEM_SAFE_FREE(s->fields);
	}
	MEM_SAFE_FREE(sdna->types);
	MEM_freeN(sdna);
}

struct DNAField *DNA_field_new(struct DNAStruct *parent, const char *name) {
	size_t alloc = sizeof(DNAField) * (size_t)(parent->fields_len + 1);
	parent->fields = MEM_recallocN_id(parent->fields, alloc, "DNAStruct::DNAField");

	DNAField* field = parent->fields + parent->fields_len++;
	memcpy(field->name, name, strlen(name) + 1);
	return field;
}

struct DNAStruct *DNA_struct_new(struct SDNA *parent, const char *name) {
	size_t alloc = sizeof(DNAStruct) * (size_t)(parent->types_len + 1);
	parent->types = MEM_recallocN_id(parent->types, alloc, "DNAStruct");

	DNAStruct* type = parent->types + parent->types_len++;
	memcpy(type->name, name, strlen(name) + 1);
	return type;
}

static void sdna_write(struct SDNA *sdna, const void *buffer, size_t length) {
	while (sdna->data_len + length >= sdna->data_alloc) {
		sdna->data_alloc += 256;
		sdna->data = MEM_recallocN_id(sdna->data, sdna->data_alloc, "SDNA::data");
	}

	memcpy(sdna->data + sdna->data_len, buffer, length);
	sdna->data_len += length;
}

bool DNA_sdna_has_type(struct SDNA* sdna, const char* name) {
	for(struct DNAStruct *type = sdna->types; type != sdna->types + sdna->types_len; type++) {
		if (strcmp(type->name, name) == 0) {
			return true;
		}
	}
	return false;
}

void DNA_sdna_compile(struct SDNA *sdna) {
	if (sdna->data && sdna->data_alloc) {
		MEM_freeN(sdna->data);
	}

	sdna->data_alloc = 256;
	sdna->data_len = 0;
	sdna->data = MEM_mallocN(sdna->data_alloc, "SDNA::data");
	
	sdna->flags = 0;

	int word = 'SDNA';
	sdna_write(sdna, &word, sizeof(word));

	sdna_write(sdna, &sdna->types_len, sizeof(sdna->types_len));
	for (DNAStruct *type = sdna->types; type != sdna->types + sdna->types_len; type++) {
		sdna_write(sdna, &type->name, strlen(type->name) + 1);
		
		sdna_write(sdna, &type->size, sizeof(type->size));
		sdna_write(sdna, &type->align, sizeof(type->align));
		
		sdna_write(sdna, &type->fields_len, sizeof(type->fields_len));
		for (DNAField *field = type->fields; field != type->fields + type->fields_len; field++) {
			sdna_write(sdna, &field->name, strlen(field->name) + 1);
			sdna_write(sdna, &field->type, strlen(field->type) + 1);

			sdna_write(sdna, &field->offset, sizeof(field->offset));
			sdna_write(sdna, &field->size, sizeof(field->size));
			sdna_write(sdna, &field->align, sizeof(field->align));
			sdna_write(sdna, &field->array, sizeof(field->array));

			sdna_write(sdna, &field->flags, sizeof(field->flags));
		}
	}
}
