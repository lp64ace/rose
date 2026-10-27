#include "MEM_guardedalloc.h"

#include "KER_context.h"
#include "KER_global.h"
#include "KER_layer.h"
#include "KER_scene.h"

#include "transform_convert.h"

ROSE_STATIC void transform_data_containers_init(TransInfo *t, Object *active, Object **objects, unsigned int length) {
	switch (t->type) {
		case TC_OBJECT:
		case TC_NONE:
		default:
			/* Does not support Multi object editing. */
			return;
	}

	const int object_type = active ? active->type : -1;

	/* No-op; TODO! */

	ROSE_assert_unreachable();
}

ROSE_STATIC int transform_data_containers_count_and_clean(TransInfo *t) {
	ROSE_assert(ELEM(t->data_length_all, 0, -1));
	t->data_length_all = 0;

	int original_data_length = t->data_container_length;
	for (TransDataContainer *th_end = t->data_container - 1, *tc = &t->data_container[t->data_container_length - 1]; tc != th_end; tc--) {
		if (tc->data_length == 0) {
			unsigned int index = tc - t->data_container;
			if (index + 1 != t->data_container_length) {
				SWAP(TransDataContainer, t->data_container[index], t->data_container[t->data_container_length - 1]);
			}
			t->data_container_length -= 1;
		}
		else {
			t->data_length_all += tc->data_length;
		}
	}

	if (original_data_length != t->data_container_length) {
		t->data_container = MEM_reallocN(t->data_container, sizeof(*t->data_container) * t->data_container_length);
	}

	return t->data_length_all;
}

ROSE_STATIC eTConvertType convert_type_get(const TransInfo *t) {
	ViewLayer *view_layer = t->view_layer;
	Object *object = OBJACT(view_layer);
	eTConvertType type = TC_NONE;

	if (object != NULL) {
		type = TC_OBJECT;
	}

	return type;
}

void ED_transform_data_create(rContext *C, TransInfo *t) {
	t->data_length_all = -1;

	t->type = convert_type_get(t);

	ViewLayer *view_layer = t->view_layer;
	Object *object = OBJACT(view_layer);
	transform_data_containers_init(t, object, NULL, 0);

	switch (t->type) {
		case TC_OBJECT: {
			t->options |= CTX_OBJECT;

			KER_scene_graph_evaluated_ensure(t->depsgraph, CTX_data_main(t->context));

			// if ((t->settings->transform_flag & SCE_XFORM_DATA_ORIGIN) != 0) {
			// 	t->options |= CTX_OBMODE_XFORM_OBDATA;
			// }
			// if ((t->settings->transform_flag & SCE_XFORM_SKIP_CHILDREN) != 0) {
			// 	t->options |= CTX_OBMODE_XFORM_SKIP_CHILDREN;
			// }

			ED_transform_object_create(C, t);
		} break;
		default: {
			/** TransInfo->type is not implemented! */
			ROSE_assert(t->data_length_all == -1);
			t->data_length_all = 0;
		} break;
	}

	transform_data_containers_count_and_clean(t);
}
