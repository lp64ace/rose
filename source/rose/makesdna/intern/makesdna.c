#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "MEM_alloc.h"

#include "LIB_assert.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "DNA_sdna_types.h"

static const char *includefiles[] = {
#include "dna_includes_as_strings.h"
};

int main(void) {
	return 0;
}
