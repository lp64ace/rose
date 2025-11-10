#include "MOD_modifiertypes.h"

void MOD_modifier_types_init(struct ModifierTypeInfo *modifier_types[NUM_MODIFIER_TYPES]) {
#define INIT_TYPE(modifier) (modifier_types[MODIFIER_TYPE_##modifier] = &MODType_##modifier)
	INIT_TYPE(NONE);
	INIT_TYPE(ARMATURE);
#undef INIT_TYPE
}
