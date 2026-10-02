#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_vector_types.h"
#include "DNA_userdef_types.h"
#include "DNA_view3d_types.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_query.h"

#include "DRW_engine.h"

#include "ED_screen.h"
#include "ED_object.h"
#include "ED_view3d.h"

#include "LIB_assert.h"
#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_math_vector.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "KER_global.h"
#include "KER_layer.h"
#include "KER_screen.h"
#include "KER_scene.h"
#include "KER_object.h"

#include "GPU_framebuffer.h"
#include "GPU_matrix.h"
#include "GPU_select.h"

#include "WM_api.h"
#include "WM_draw.h"
#include "WM_window.h"

#include "view3d_intern.h"
#include "view3d_navigate.h"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Assigning Operator Types
 * \{ */

void view3d_operatortypes() {
	WM_operatortype_append(VIEW3D_OT_rotate);
	WM_operatortype_append(VIEW3D_OT_pan);
	WM_operatortype_append(VIEW3D_OT_zoom);
	WM_operatortype_append(VIEW3D_OT_select);
	WM_operatortype_append(VIEW3D_OT_reset);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Operator Key Map
 * \{ */

void view3d_keymap(wmKeyConfig *keyconf) {
	/* 3D View ------------------------------------------------ */
	wmKeyMap *keymap = WM_keymap_ensure(keyconf, "3D View", SPACE_VIEW3D, RGN_TYPE_WINDOW);

	view3d_navigate_keymap(keymap);
	view3d_select_keymap(keymap);
}

/** \} */
