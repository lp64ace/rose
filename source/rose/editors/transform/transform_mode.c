#include "MEM_guardedalloc.h"

#include "RNA_access.h"

#include "LIB_assert.h"
#include "LIB_math_vector.h"
#include "LIB_utildefines.h"

#include "WM_api.h"

#include "transform_mode.h"

void ED_transform_mode_init(TransInfo *t, wmOperator *op, eTransformMode mode) {
	t->mode = mode;

	switch (mode) {
		case TFM_TRANSLATION:
			ED_transform_mode_translation_init(t);
			break;
		case TFM_ROTATION:
			// ED_transform_mode_rotation_init(t);
			break;
		case TFM_RESIZE: {
			float mouse_dir_constraint[3];
			if (op) {
				PropertyRNA *propery = RNA_struct_find_property(op->ptr, "mouse_dir_constraint");
				if (propery) {
					RNA_property_float_get_array(op->ptr, propery, mouse_dir_constraint);
				}
				else {
					/* Resize is expected to have this property. */
					ROSE_assert(!STREQ(op->idname, "TRANSFORM_OT_resize"));
				}
			}
			else {
				zero_v3(mouse_dir_constraint);
			}
			ED_transform_mode_resize_init(t, mouse_dir_constraint);
		} break;
	}

	ROSE_assert(t->mode == mode);
}
