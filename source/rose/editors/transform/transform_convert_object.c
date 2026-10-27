#include "MEM_guardedalloc.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"
#include "DEG_depsgraph_query.h"

#include "ED_object.h"

#include "KER_context.h"
#include "KER_layer.h"
#include "KER_lib_id.h"
#include "KER_main.h"
#include "KER_scene.h"
#include "KER_object.h"

#include "transform_convert.h"
#include "transform_data.h"

/* -------------------------------------------------------------------- */
/** \name Object Mode Custom Data
 * \{ */

typedef struct TransDataObject {
	/**
	 * Object to object data transform table.
	 * Don't add these to transform data because we may want to include child objects
	 * which aren't being transformed.
	 */
	struct XFormObjectData_Container *xds;
} TransDataObject;

ROSE_STATIC void transform_object_custom_data_free(TransInfo *t, TransDataContainer *tc, TransCustomData *customdata) {
	TransDataObject *tdo = customdata->data;
	customdata->data = NULL;

	if ((t->options & CTX_OBMODE_XFORM_OBDATA) != 0) {
		ROSE_assert_unreachable();
	}
	if ((t->options & CTX_OBMODE_XFORM_SKIP_CHILDREN) != 0) {
		ROSE_assert_unreachable();
	}
	MEM_freeN(tdo);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Data in Object Mode
 *
 * Use to implement 'Affect Only Origins' feature.
 * We need this to be detached from transform data because,
 * unlike transforming regular objects, we need to transform the children.
 *
 * Nearly all of the logic here is in the 'ED_object_data_xform_container_*' API.
 * \{ */

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Transform Creation
 *
 * Instead of transforming the selection, move the 2D/3D cursor.
 *
 * \{ */

ROSE_STATIC void transform_object_base_deps_flag_prepare(ViewLayer *view_layer) {
	LISTBASE_FOREACH(Base *, base, &view_layer->bases) {
		base->object->id.tag &= ~ID_TAG_DOIT;
	}
}

ROSE_STATIC void set_transform_object_base_deps_flag_cb(ID *id, int component, void *user_data) {
	if (GS(id->name) != ID_OB) {
		return;
	}
	if (ELEM(component, DEG_OB_COMP_TRANSFORM, DEG_OB_COMP_GEOMETRY)) {
		return;
	}
	id->tag |= ID_TAG_DOIT;
}

ROSE_STATIC void flush_transform_object_base_deps_flag(Depsgraph *depsgraph, Object *object) {
	object->id.tag |= ID_TAG_DOIT;
	DEG_foreach_dependent_ID_component(depsgraph, &object->id, DEG_OB_COMP_TRANSFORM, DEG_FOREACH_COMPONENT_IGNORE_TRANSFORM_SOLVERS, transform_object_base_deps_flag_prepare, NULL);
}

ROSE_STATIC void set_transform_object_base_flags(TransInfo *t) {
	Main *main = CTX_data_main(t->context);
	ViewLayer *view_layer = t->view_layer;
	Scene *scene = t->scene;
	Depsgraph *depsgraph = KER_scene_ensure_depsgraph(main, scene, view_layer);

	/* Don't do it if we're not actually going to recalculate anything. */
	if (t->mode == TFM_NONE) {
		return;
	}

	/* Makes sure base flags and object flags are identical. */
	KER_scene_base_flag_to_objects(t->view_layer);
	/* Make sure depsgraph is here. */
	DEG_graph_relations_update(depsgraph);
	/* Clear all flags we need. It will be used to detect dependencies. */
	transform_object_base_deps_flag_prepare(view_layer);
	/* Traverse all bases and set all possible flags. */
	LISTBASE_FOREACH(Base *, base, &view_layer->bases) {
		base->flag_legacy &= ~(BASE_WAS_SELECTED);
		if ((base->flag & BASE_SELECTED) != 0 && KER_base_visible(base)) {
			Object *object = base->object;
			Object *parent = object->parent;

			while (parent != NULL) {
				if ((parent->flag_base & BASE_SELECTED) != 0) {
					Base *baseparent = KER_view_layer_base_find(view_layer, parent);
					if (baseparent != NULL) {
						if ((baseparent->flag & BASE_SELECTED) != 0 && KER_base_visible(baseparent)) {
							break;
						}
					}
				}
				parent = parent->parent;
			}

			if (parent != NULL) {
				base->flag &= ~BASE_SELECTED;
				base->local_collections_bits |= BASE_WAS_SELECTED;
			}
			flush_transform_object_base_deps_flag(depsgraph, object);
		}
	}
}

ROSE_STATIC void clear_transform_object_base_flags(TransInfo *t) {
	ViewLayer *view_layer = t->view_layer;
	Base *base;

	for (base = view_layer->bases.first; base; base = base->next) {
		if (base->flag_legacy & BASE_WAS_SELECTED) {
			ED_object_base_select(base, BA_SELECT);
		}

		base->flag_legacy &= ~(BASE_WAS_SELECTED);
	}
}

void ED_transform_object_create(rContext *C, TransInfo *t) {
	Main *main = CTX_data_main(C);
	TransData *td = NULL;

	set_transform_object_base_flags(t);

	TransDataContainer *tc = TRANS_DATA_CONTAINER_FIRST_SINGLE(t);

	if (!tc->data_length) {
		/* clear here, main transform function escapes too. */
		clear_transform_object_base_flags(t);
		return;
	}

	td = tc->data = MEM_callocN(tc->data_length * sizeof(TransData), "TransOb");

	TransDataObject *tdo = MEM_callocN(sizeof(TransDataObject), "TransDataObject");
	t->custom.type.data = tdo;
	t->custom.type.free_cb = transform_object_custom_data_free;

	if (t->options & CTX_OBMODE_XFORM_OBDATA) {
		ROSE_assert_unreachable();
	}
}

/** \} */
