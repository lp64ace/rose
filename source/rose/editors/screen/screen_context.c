#include "KER_context.h"
#include "KER_object.h"
#include "KER_lib_id.h"
#include "KER_rose.h"
#include "KER_layer.h"
#include "KER_scene.h"

#include "LIB_ghash.h"
#include "LIB_utildefines.h"

#include "WM_api.h"
#include "WM_window.h"

const char *screen_context_dir[] = {
	"selected_objects",
};

ROSE_INLINE eContextResult screen_ctx_selected_objects(const rContext *C, rContextDataResult *result) {
	wmWindow *window = CTX_wm_window(C);
	View3D *v3d = CTX_wm_space_view3d(C);
	ViewLayer *view_layer = WM_window_get_active_view_layer(window);

	LISTBASE_FOREACH(Base *, base, &view_layer->bases) {
		if ((base->flag & BASE_SELECTED) != 0 && KER_base_is_visible(v3d, base)) {
			CTX_data_id_list_add(result, &base->object->id);
		}
	}

	CTX_data_type_set(result, CTX_DATA_TYPE_COLLECTION);
	return CTX_RESULT_OK;
} 

typedef eContextResult (*fnContextCallback)(const rContext *C, rContextDataResult *result);
static GHash *EDScreenContextFunctions = NULL;

ROSE_STATIC void free_context_function_ghash(void *UNUSED(user_data)) {
	LIB_ghash_free(EDScreenContextFunctions, NULL, NULL);
}
ROSE_INLINE void register_context_function(const char *member, fnContextCallback function) {
	LIB_ghash_insert(EDScreenContextFunctions, (void *)member, function);
}

ROSE_STATIC void ensure_ed_screen_context_functions(void) {
	if (EDScreenContextFunctions != NULL) {
		return;
	}

	EDScreenContextFunctions = LIB_ghash_new(LIB_ghashutil_strhash_p_murmur, LIB_ghashutil_strcmp, __func__);

	KER_rose_atexit_register(free_context_function_ghash, NULL);

	register_context_function("selected_objects", screen_ctx_selected_objects);
}

eContextResult ed_screen_context(const rContext *C, const char *member, rContextDataResult *result) {
	if (CTX_data_dir(member)) {
		CTX_data_dir_set(result, screen_context_dir);
		return CTX_RESULT_OK;
	}

	ensure_ed_screen_context_functions();
	fnContextCallback cb = LIB_ghash_lookup(EDScreenContextFunctions, member);
	if (cb == NULL) {
		return CTX_RESULT_MEMBER_NOT_FOUND;
	}

	return cb(C, result);
}
