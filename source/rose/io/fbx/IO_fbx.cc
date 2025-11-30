#include "KER_collection.h"
#include "KER_context.h"
#include "KER_layer.h"
#include "KER_main.h"
#include "KER_mesh.h"
#include "KER_scene.h"
#include "KER_object.h"

#include "LIB_fileops.h"
#include "LIB_task.hh"

#include "IO_fbx.h"

#include "importer/fbx_import_util.hh"
#include "importer/fbx_import_anim.hh"
#include "importer/fbx_import_armature.hh"
#include "importer/fbx_import_mesh.hh"

namespace rose::io::fbx {

struct FbxImportContext {
	const ufbx_scene *fbx;

	Main *main;
	Scene *scene;
	FbxElementMapping mapping;

	float fps;

	FbxImportContext(Main *main, Scene *scene, const ufbx_scene *fbx, const char *filepath) : main(main), scene(scene), fbx(fbx) {
		fps = (float)fbx->settings.frames_per_second;
	}

	void import_globals();
	void import_meshes();
	void import_armatures();
	void import_animation(double fps);

};

void FbxImportContext::import_globals() {
	RenderData *r = &this->scene->r;

	if (r->fps == 0) {
		r->fps = fps;
	}
	else {
		fps = r->fps;
	}
}

void FbxImportContext::import_meshes() {
	rose::io::fbx::import_meshes(this->main, this->scene, this->fbx, &this->mapping);
}

void FbxImportContext::import_armatures() {
	rose::io::fbx::import_armatures(this->main, this->scene, this->fbx, &this->mapping);
}

void FbxImportContext::import_animation(double fps) {
	rose::io::fbx::import_animations(this->main, this->scene, this->fbx, &this->mapping, fps);
}

}  // namespace rose::io::fbx

using namespace rose::io::fbx;

static void fbx_task_run_fn(void * /* user */, ufbx_thread_pool_context ctx, uint32_t /* group */, uint32_t start_index, uint32_t count) {
	rose::threading::parallel_for_each(rose::IndexRange(start_index, count), [&](const int64_t index) {
		ufbx_thread_pool_run_task(ctx, index);
	});
}

static void fbx_task_wait_fn(void * /* user */, ufbx_thread_pool_context /* ctx */, uint32_t /* group */, uint32_t /* max_index */) {
	/* Empty implementation; #fbx_task_run_fn already waits for the tasks. This means that only one fbx "task group" is effectively scheduled at once. */
}

void importer_scene(Main *main, Scene *scene, ViewLayer *view_layer, ufbx_scene *fbx, const char *filepath) {
	FbxImportContext ctx(main, scene, fbx, filepath);

	ctx.import_globals();
	ctx.import_armatures();
	ctx.import_meshes();
	ctx.import_animation(ctx.fps);

	LayerCollection *lc = KER_layer_collection_get_active(view_layer);

	/* Add objects to collection. */
	for (Object *obj : ctx.mapping.imported_objects) {
		KER_collection_object_add(main, lc->collection, obj);
	}

	KER_view_layer_base_deselect_all(view_layer);
	for (Object *obj : ctx.mapping.imported_objects) {
		Base *base = KER_view_layer_base_find(view_layer, obj);
		KER_view_layer_base_select_and_set_active(view_layer, base);
	}
}

void importer_file(Main *main, Scene *scene, ViewLayer *view_layer, const char *filepath) {
	FILE *file = fopen(filepath, "rb");
	if (!file) {
		fprintf(stderr, "[FBX] Cannot open resource file '%s'", filepath);
		return;
	}

	ufbx_load_opts opts = {};
	opts.filename.data = filepath;
	opts.filename.length = LIB_strlen(filepath);
	opts.evaluate_skinning = false;
	opts.evaluate_caches = false;
	opts.load_external_files = false;
	opts.clean_skin_weights = true;
	opts.use_blender_pbr_material = true;

	opts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY;
	opts.pivot_handling = UFBX_PIVOT_HANDLING_ADJUST_TO_ROTATION_PIVOT;

	opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
	opts.target_axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
	opts.target_axes.up = UFBX_COORDINATE_AXIS_POSITIVE_Z;
	opts.target_axes.front = UFBX_COORDINATE_AXIS_NEGATIVE_Y;
	opts.target_unit_meters = 128.0f;

	opts.target_camera_axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
	opts.target_camera_axes.up = UFBX_COORDINATE_AXIS_POSITIVE_Y;
	opts.target_camera_axes.front = UFBX_COORDINATE_AXIS_POSITIVE_Z;
	opts.target_light_axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
	opts.target_light_axes.up = UFBX_COORDINATE_AXIS_POSITIVE_Y;
	opts.target_light_axes.front = UFBX_COORDINATE_AXIS_POSITIVE_Z;

	/* Setup ufbx threading to go through our own task system. */
	opts.thread_opts.pool.run_fn = fbx_task_run_fn;
	opts.thread_opts.pool.wait_fn = fbx_task_wait_fn;

	ufbx_error fbx_error;
	ufbx_scene *fbx = ufbx_load_stdio(file, &opts, &fbx_error);
	fclose(file);

	if (!fbx) {
		fprintf(stderr, "[FBX] Cannot import resource file '%s': %s", filepath, fbx_error.description.data);
		return;
	}

	importer_scene(main, scene, view_layer, fbx, filepath);

	ufbx_free_scene(fbx);
}

void importer_memory(Main *main, Scene *scene, ViewLayer *view_layer, const void *memory, size_t size) {
	ufbx_load_opts opts = {};
	opts.evaluate_skinning = false;
	opts.evaluate_caches = false;
	opts.load_external_files = false;
	opts.clean_skin_weights = true;
	opts.use_blender_pbr_material = true;

	opts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY;
	opts.pivot_handling = UFBX_PIVOT_HANDLING_ADJUST_TO_ROTATION_PIVOT;

	opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;
	opts.target_axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
	opts.target_axes.up = UFBX_COORDINATE_AXIS_POSITIVE_Z;
	opts.target_axes.front = UFBX_COORDINATE_AXIS_NEGATIVE_Y;
	opts.target_unit_meters = 128.0f;

	opts.target_camera_axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
	opts.target_camera_axes.up = UFBX_COORDINATE_AXIS_POSITIVE_Y;
	opts.target_camera_axes.front = UFBX_COORDINATE_AXIS_POSITIVE_Z;
	opts.target_light_axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
	opts.target_light_axes.up = UFBX_COORDINATE_AXIS_POSITIVE_Y;
	opts.target_light_axes.front = UFBX_COORDINATE_AXIS_POSITIVE_Z;

	/* Setup ufbx threading to go through our own task system. */
	opts.thread_opts.pool.run_fn = fbx_task_run_fn;
	opts.thread_opts.pool.wait_fn = fbx_task_wait_fn;

	ufbx_error fbx_error;
	ufbx_scene *fbx = ufbx_load_memory(memory, size, &opts, &fbx_error);

	if (!fbx) {
		fprintf(stderr, "[FBX] Cannot import resource file : %s", fbx_error.description.data);
		return;
	}

	importer_scene(main, scene, view_layer, fbx, "");

	ufbx_free_scene(fbx);
}

void FBX_import(struct rContext *C, const char *filepath) {
	Main *main = CTX_data_main(C);
	Scene *scene = CTX_data_scene(C);
	ViewLayer *view_layer = CTX_data_view_layer(C);

	importer_file(main, scene, view_layer, filepath);
}

void FBX_import_memory(struct rContext *C, const void *memory, size_t size) {
	Main *main = CTX_data_main(C);
	Scene *scene = CTX_data_scene(C);
	ViewLayer *view_layer = CTX_data_view_layer(C);

	importer_memory(main, scene, view_layer, memory, size);
}
