#ifndef TRANSFORM_MODE_H
#define TRANSFORM_MODE_H

#include "transform.h"

#ifdef __cplusplus
extern "C" {
#endif

void ED_transform_mode_init(struct TransInfo *t, struct wmOperator *op, eTransformMode mode);

/* Modes */

void ED_transform_mode_translation_init(struct TransInfo *t);
// void ED_transform_mode_rotation_init(struct TransInfo *t);
void ED_transform_mode_resize_init(struct TransInfo *t, const float mouse_dir_constraint[3]);

#ifdef __cplusplus
}
#endif

#endif // !TRANSFORM_MODE_H
