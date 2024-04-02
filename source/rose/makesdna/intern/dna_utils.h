#pragma once

#include "DNA_sdna_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DNAField;
struct DNAStruct;
struct SDNA;

struct SDNA *SDNA_alloc(void);
void SDNA_free(struct SDNA *);

struct DNAField *DNA_field_new(struct DNAStruct *parent, const char *name);
struct DNAStruct *DNA_struct_new(struct SDNA *parent, const char *name);

void DNA_sdna_compile(struct SDNA *sdna);

#ifdef __cplusplus
}
#endif
