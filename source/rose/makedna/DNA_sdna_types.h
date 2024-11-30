#ifndef DNA_SDNA_TYPES_H
#define DNA_SDNA_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RHead {
	int filecode;
	int size;
	int length;
	/** Note that there are 4 padding bytes here! */
	uint64_t address;
	uint64_t dnatype;
} RHead;

#ifdef __cplusplus
}
#endif

#endif	// DNA_SDNA_TYPES_H
