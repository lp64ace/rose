#include "MEM_guardedalloc.h"

#include "RNA_access.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"
#include "DEG_depsgraph_query.h"

#include "LIB_math_vector.h"
#include "LIB_math_matrix.h"

#include "ED_object.h"

#include "KER_context.h"
#include "KER_layer.h"
#include "KER_lib_id.h"
#include "KER_main.h"
#include "KER_scene.h"
#include "KER_object.h"

#include "transform_convert.h"
#include "transform_data.h"
#include "transform_orientations.h"

#include <stdio.h>

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

ROSE_STATIC void transform_object_to_transformat_data(TransInfo *t, TransData *td, Object *object) {
	Scene *scene = t->scene;

	td->object = object;

	/* axismtx has the real orientation */
	transform_orientations_create_from_axis(td->axismtx, UNPACK3(object->obmat));

	td->base.loc = object->loc;
	copy_v3_v3(td->base.iloc, object->loc);
	copy_v3_v3(td->base.center, object->obmat[3]);

	td->extension->size = object->scale;
	copy_v3_v3(td->extension->isize, object->scale);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Recalc Data object
 * \{ */

 void ED_transform_object_recalc(TransInfo *t) {
	for (TransDataContainer *tc = t->data_container, *tc_end = t->data_container + t->data_container_length; tc != tc_end; tc++) {
		TransData *td = tc->data;

		for (int i = 0; i < tc->data_length; i++, td++) {
			Object *object = td->object;

			DEG_id_tag_update(&object->id, ID_RECALC_TRANSFORM);
		}
	}
 }

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

ROSE_STATIC void set_transform_object_base_deps_flag_cb(ID *id, eDepsObjectComponentType component, void *user_data) {
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
	DEG_foreach_dependent_ID_component(depsgraph, &object->id, DEG_OB_COMP_TRANSFORM, DEG_FOREACH_COMPONENT_IGNORE_TRANSFORM_SOLVERS, set_transform_object_base_deps_flag_cb, NULL);
}

ROSE_STATIC void set_transform_object_base_flags(TransInfo *t) {
	Main *main = CTX_data_main(t->context);
	ViewLayer *view_layer = t->view_layer;
	View3D *v3d = t->view;
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
		if ((base->flag & BASE_SELECTED) != 0 && KER_base_is_visible(v3d, base)) {
			Object *object = base->object;
			Object *parent = object->parent;

			while (parent != NULL) {
				if ((parent->flag_base & BASE_SELECTED) != 0) {
					Base *baseparent = KER_view_layer_base_find(view_layer, parent);
					if (baseparent != NULL) {
						if ((baseparent->flag & BASE_SELECTED) != 0 && KER_base_is_visible(v3d, baseparent)) {
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
	TransDataExtension *tx = NULL;

	set_transform_object_base_flags(t);

	TransDataContainer *tc = TRANS_DATA_CONTAINER_FIRST_SINGLE(t);

	/* count */
	tc->data_length = CTX_DATA_COUNT(C, selected_bases);

	if (!tc->data_length) {
		/* clear here, main transform function escapes too. */
		clear_transform_object_base_flags(t);
		return;
	}

	td = tc->data = MEM_callocN(tc->data_length * sizeof(TransData), "TransOb");
	tx = tc->data_extension = MEM_callocN(tc->data_length * sizeof(TransDataExtension), "TransObExtension");

	TransDataObject *tdo = MEM_callocN(sizeof(TransDataObject), "TransDataObject");
	t->custom.type.data = tdo;
	t->custom.type.free_cb = transform_object_custom_data_free;

	if (t->options & CTX_OBMODE_XFORM_OBDATA) {
		ROSE_assert_unreachable();
	}

	CTX_DATA_BEGIN(C, Base *, base, selected_bases) {
		Object *object = base->object;

		td->extension = tx;

		transform_object_to_transformat_data(t, td, object);
		td->val = NULL;
		td++;
		tx++;
	}
	CTΧ_DATA_END;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Special After Transform Object
 * \{ */

 void ED_transform_special_after_transform_update_object(rContext *C, TransInfo *t) {
	ROSE_assert((t->options & CTX_OBJECT) != 0);

	TransDataContainer *tc = TRANS_DATA_CONTAINER_FIRST_SINGLE(t);
	for (int i = 0; i < tc->data_length; i++) {
		TransData *td = tc->data + i;

		Object *object = td->object;

		/**
		 * Needed for proper updating of "quick cached" dynamics.
		 * Creates troubles for moving animated objects without
		 * auto-key though, probably needed is an animation-system override?
		 * NOTE(@jahka): Please remove if some other solution is found.
		 * */
		DEG_id_tag_update(&object->id, ID_RECALC_TRANSFORM);

		if (t->state != TRANS_CANCEL) {
			/* No-op */
		}
	}

	clear_transform_object_base_flags(t);
 }

/** \} */
