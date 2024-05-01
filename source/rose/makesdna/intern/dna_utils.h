#pragma once

#include "DNA_sdna_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DNAField;
struct DNAStruct;
struct SDNA;

struct SDNA *DNA_sdna_alloc(void);
void DNA_sdna_free(struct SDNA *);

struct DNAField *DNA_field_new(struct DNAStruct *parent, const char *name);
struct DNAStruct *DNA_struct_new(struct SDNA *parent, const char *name);

bool DNA_sdna_has_type(struct SDNA* sdna, const char* name);

void DNA_sdna_compile(struct SDNA *sdna);

/** Find the `DNAStruct *` information of a field, if it exists. */
struct DNAStruct* DNA_sdna_find_field_as_struct(struct SDNA* sdna, struct DNAField* field);
/** To be called like `DNA_sdna_find_field_type_as_struct(sdna, field->type);` */
struct DNAStruct* DNA_sdna_find_field_type_as_struct(struct SDNA* sdna, const char *field);

#ifdef __cplusplus
}
#endif
