#include "MEM_guardedalloc.h"

#include "KER_collection.h"
#include "KER_idtype.h"
#include "KER_layer.h"
#include "KER_lib_id.h"
#include "KER_main.h"
#include "KER_scene.h"

#include <limits.h>

/* -------------------------------------------------------------------- */
/** \name Scene Creation
 * \{ */

Scene *KER_scene_new(Main *main, const char *name) {
	return KER_id_new(main, ID_SCE, name);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Scene Render Data
 * \{ */

void KER_scene_time_step(struct Scene *scene, float dt) {
	RenderData *r = &scene->r;

	r->subframe += dt * r->fps;

	int f = (int)r->subframe;

	if (f > 0) {
		r->subframe = r->subframe - (float)f;
		r->cframe += f;

		/**
		 * This should only happen once and is faster than running 
		 * a module here!
		 */
		const int l = r->eframe - r->sframe;
		while (r->cframe >= r->eframe) {
			r->cframe -= l;
		} 
	}
}

float KER_scene_frame(const Scene *scene) {
	const RenderData *r = &scene->r;

	return (float)r->cframe + (float)r->subframe;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Scene Data-block Definition
 * \{ */

ROSE_STATIC void scene_init_data(ID *id) {
	Scene *scene = (Scene *)id;

	scene->r.sframe = 0;
	scene->r.eframe = INT_MAX;
	scene->r.fps = 24;

	/* Master Collection */
	scene->master_collection = KER_collection_master_add();

	KER_view_layer_add(scene, "ViewLayer", NULL, VIEWLAYER_ADD_NEW);
}

ROSE_STATIC void scene_free_data(ID *id) {
	Scene *scene = (Scene *)id;

	LISTBASE_FOREACH_MUTABLE(ViewLayer *, view_layer, &scene->view_layers) {
		LIB_remlink(&scene->view_layers, view_layer);
		KER_view_layer_free_ex(view_layer, false);
	}

	/* Master Collection */
	/* TODO: what to do with do_id_user? it's also true when just
	 * closing the file which seems wrong? should decrement users
	 * for objects directly in the master collection? then other
	 * collections in the scene need to do it too? */
	if (scene->master_collection) {
		KER_collection_free_data(scene->master_collection);
		MEM_freeN(scene->master_collection);
		scene->master_collection = NULL;
	}
}

 IDTypeInfo IDType_ID_SCE = {
	.idcode = ID_SCE,

	.filter = FILTER_ID_SCE,
	.depends = 0,
	.index = INDEX_ID_SCE,
	.size = sizeof(Scene),

	.name = "Scene",
	.name_plural = "Scenes",

	.flag = IDTYPE_FLAGS_NO_COPY,

	.init_data = scene_init_data,
	.copy_data = NULL,
	.free_data = scene_free_data,

	.foreach_id = NULL,

	.write = NULL,
	.read_data = NULL,
};

/** \} */

