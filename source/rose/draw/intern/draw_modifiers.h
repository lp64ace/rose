#ifndef DRAW_MODIFIERS_H
#define DRAW_MODIFIERS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool draw_modifier_is_device(struct ModifierData *md);
bool draw_modifier_is_device_supported(struct ModifierData *md);

void draw_modifier_cache_populate(struct ModifierData *md, struct Object *object);
void draw_modifier_cache_build(struct ModifierData *md, struct Object *object);

#ifdef __cplusplus
}
#endif

#endif	// DRAW_MODIFIERS_H