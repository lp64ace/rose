#include "dna_utils.h"

#include "MEM_alloc.h"

struct SDNA *SDNA_alloc(void) {
	struct SDNA *sdna = MEM_callocN(sizeof(SDNA), "SDNA");
	
	return sdna;
}

void SDNA_free(struct SDNA *sdna) {
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
	
}

struct DNAStruct *DNA_struct_new(struct SDNA *parent, const char *name) {
}
