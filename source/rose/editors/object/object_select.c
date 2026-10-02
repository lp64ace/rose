#include "LIB_listbase.h"
#include "LIB_string_utils.h"
#include "LIB_utildefines.h"

#include "KER_action.h"
#include "KER_armature.h"
#include "KER_collection.h"
#include "KER_context.h"
#include "KER_deform.h"
#include "KER_layer.h"
#include "KER_lib_id.h"
#include "KER_main.h"
#include "KER_object.h"
#include "KER_report.h"
#include "KER_scene.h"

#include "ED_object.h"

/* -------------------------------------------------------------------- */
/** \name Public Object Selection API
 * \{ */

void ED_object_base_select(Base *base, eObjectSelect_Mode mode) {
	if (mode == BA_INVERT) {
		mode = (base->flag & BASE_SELECTED) != 0 ? BA_DESELECT : BA_SELECT;
	}

	if (base) {
		switch (mode) {
			case BA_SELECT:
				if ((base->flag & BASE_SELECTABLE) != 0) {
					base->flag |= BASE_SELECTED;
				}
				break;
			case BA_DESELECT:
				base->flag &= ~BASE_SELECTED;
				break;
			case BA_INVERT:
				/* Never happens. */
				break;
		}
		KER_scene_object_base_flag_sync_from_base(base);
	}
}

/** \} */
