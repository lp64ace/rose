#ifndef MOD_MODIFIERTYPES_H
#define MOD_MODIFIERTYPES_H

#include "KER_modifier.h"

#ifdef __cplusplus
extern "C" {
#endif

void MOD_modifier_types_init(struct ModifierTypeInfo *modifier_types[NUM_MODIFIER_TYPES]);

extern ModifierTypeInfo MODType_NONE;
extern ModifierTypeInfo MODType_ARMATURE;

#ifdef __cplusplus
}
#endif

#endif	// MOD_MODIFIERTYPES_H