#include "KER_context.h"
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

	FbxImportContext(Main *main, Scene *scene, const ufbx_scene *fbx, const char *filepath) :main(main), fbx(fbx) {

	}

	void import_globals();
	void import_meshes();
	void import_armatures();
	void import_animation(double fps);

};

void FbxImportContext::import_globals() {
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

void importer_main(Main *main, Scene *scene, const char *filepath) {
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
	opts.target_unit_meters = 1.0f;

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

	FbxImportContext ctx(main, scene, fbx, filepath);

	ctx.import_globals();
	ctx.import_armatures();
	ctx.import_meshes();
	ctx.import_animation(fbx->settings.frames_per_second);

	ufbx_free_scene(fbx);
}

void FBX_import(struct rContext *C, const char *filepath) {
	Main *main = CTX_data_main(C);
	Scene *scene = CTX_data_scene(C);

	importer_main(main, scene, filepath);
}
