#include "MEM_guardedalloc.h"

#include "LIB_endian_switch.h"
#include "LIB_ghash.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "RT_context.h"
#include "RT_parser.h"
#include "RT_token.h"

#include "genfile.h"

extern const unsigned char DNAstr[];
extern const int DNAlen;

SDNA *DNA_sdna_new_current(void) {
	SDNA *sdna = DNA_sdna_new_memory(DNAstr, DNAlen);
	if (!DNA_sdna_build_struct_list(sdna)) {
		DNA_sdna_free(sdna);
		return NULL;
	}
	return sdna;
}
