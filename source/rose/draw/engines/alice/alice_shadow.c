#include "MEM_guardedalloc.h"

#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "GPU_batch.h"
#include "GPU_framebuffer.h"
#include "GPU_state.h"
#include "GPU_viewport.h"

#include "KER_armature.h"
#include "KER_object.h"
#include "KER_modifier.h"
#include "KER_scene.h"

#include "LIB_assert.h"
#include "LIB_math_geom.h"
#include "LIB_math_matrix.h"
#include "LIB_math_vector.h"
#include "LIB_math_rotation.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "alice_engine.h"
#include "alice_private.h"

#include "intern/draw_defines.h"
#include "intern/draw_manager.h"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Alice Draw Engine Routines
 * \{ */

ROSE_INLINE void compute_parallel_lines_nor_and_dist(const float v1[2], const float v2[2], const float v3[2], float r_line[4]) {
	sub_v2_v2v2(r_line, v2, v1);
	/* Find orthogonal vector. */
	SWAP(float, r_line[0], r_line[1]);
	r_line[0] = -r_line[0];
	/* Edge distances. */
	r_line[2] = dot_v2v2(r_line, v1);
	r_line[3] = dot_v2v2(r_line, v3);
	/* Make sure r_line[2] is the minimum. */
	if (r_line[2] > r_line[3]) {
		SWAP(float, r_line[2], r_line[3]);
	}
}

ROSE_INLINE void alice_shadow_update(DRWAliceViewportPrivateData *impl) {
	if (!equals_v3_v3(impl->shadow_cached_direction, impl->shadow_direction_ws)) {
		const float up[3] = {0.0f, 0.0f, 1.0f};

		unit_m4(impl->shadow_mat);
		copy_v3_v3(impl->shadow_mat[2], impl->shadow_direction_ws);
		cross_v3_v3v3(impl->shadow_mat[0], impl->shadow_mat[2], up);
		normalize_v3(impl->shadow_mat[0]);
		cross_v3_v3v3(impl->shadow_mat[1], impl->shadow_mat[2], impl->shadow_mat[0]);

		invert_m4_m4(impl->shadow_inv, impl->shadow_mat);

		copy_v3_v3(impl->shadow_cached_direction, impl->shadow_direction_ws);
		impl->shadow_changed |= true;
	}

	float planes[6][4];
	DRW_culling_frustum_planes_get(NULL, planes);
	copy_v4_v4(impl->shadow_far_plane, planes[2]);

	BoundBox corners;
	DRW_culling_frustum_corners_get(NULL, &corners);

	float shadow_near_corners[4][3];
	mul_v3_mat3_m4v3(shadow_near_corners[0], impl->shadow_inv, corners.vec[0]);
	mul_v3_mat3_m4v3(shadow_near_corners[1], impl->shadow_inv, corners.vec[3]);
	mul_v3_mat3_m4v3(shadow_near_corners[2], impl->shadow_inv, corners.vec[7]);
	mul_v3_mat3_m4v3(shadow_near_corners[3], impl->shadow_inv, corners.vec[4]);

	INIT_MINMAX(impl->shadow_near_min, impl->shadow_near_max);
	for (int i = 0; i < 4; i++) {
		minmax_v3v3_v3(impl->shadow_near_min, impl->shadow_near_max, shadow_near_corners[i]);
	}

	compute_parallel_lines_nor_and_dist(shadow_near_corners[0], shadow_near_corners[1], shadow_near_corners[2], impl->shadow_near_sides[0]);
	compute_parallel_lines_nor_and_dist(shadow_near_corners[1], shadow_near_corners[2], shadow_near_corners[0], impl->shadow_near_sides[1]);
}

ROSE_INLINE void alice_shadow_data_update(DRWAliceViewportPrivateData *impl, AliceWorldUBO *data) {
	Scene *scene = GDrawManager.scene;

	float view_matrix[4][4];
	DRW_view_viewmat_get(NULL, view_matrix, false);

	/* Turn the light in a way where it's more user friendly to control. */
	copy_v3_v3(impl->shadow_direction_ws, scene->light_direction);
	SWAP(float, impl->shadow_direction_ws[2], impl->shadow_direction_ws[1]);
	impl->shadow_direction_ws[2] = -impl->shadow_direction_ws[2];
	impl->shadow_direction_ws[0] = -impl->shadow_direction_ws[0];

	/* Shadow direction. */
	mul_v3_m4v3(data->shadow_direction_vs, view_matrix, impl->shadow_direction_ws);

	const float focus = 0.0f;
	const float shift = 0.1f;

	/* Clamp to avoid overshadowing and shading errors. */
	data->shadow_shift = shift;
	data->shadow_focus = 1.0f - focus * (1.0f - shift);

	data->shadow_mul = 0.8f;
	data->shadow_add = 1.0f - data->shadow_mul;
}

void DRW_alice_shadow_cache_init(DRWAliceData *vdata) {
	DRWViewportEmptyList *fbl = (vdata)->fbl;
	DRWViewportEmptyList *txl = (vdata)->txl;
	DRWAliceViewportPassList *psl = (vdata)->psl;
	DRWAliceViewportStorageList *stl = (vdata)->stl;

	DRWAliceViewportPrivateData *impl = stl->data;

	alice_shadow_update(impl);

	DRWState depth_pass_state = DRW_STATE_WRITE_STENCIL_SHADOW_PASS;
	DRWState depth_fail_state = DRW_STATE_WRITE_STENCIL_SHADOW_FAIL;
	DRWState state = DRW_STATE_DEPTH_LESS | DRW_STATE_STENCIL_ALWAYS;

	if (!(psl->shadow_pass[0] = DRW_pass_new("Shadow Pass", state | depth_pass_state))) {
		return;
	}
	if (!(psl->shadow_pass[1] = DRW_pass_new("Shadow Fail", state | depth_fail_state))) {
		return;
	}

	GPUShader *shader;

	/* Stencil Shadow passes. */
	for (int manifold = 0; manifold < 2; manifold++) {
		shader = DRW_alice_shader_shadow_pass_get((bool)manifold);
		impl->shadow_pass_shgroup[manifold] = DRW_shading_group_new(shader, psl->shadow_pass[0]);

		shader = DRW_alice_shader_shadow_fail_get((bool)manifold, false);
		impl->shadow_fail_shgroup[manifold] = DRW_shading_group_new(shader, psl->shadow_pass[1]);

		shader = DRW_alice_shader_shadow_fail_get((bool)manifold, true);
		impl->shadow_caps_shgroup[manifold] = DRW_shading_group_new(shader, psl->shadow_pass[1]);
	}

	/* Needed once to set the stencil state for the pass. */
	DRW_shading_group_clear_ex(impl->shadow_pass_shgroup[0], GPU_STENCIL_BIT, NULL, 1.0f, 0xFF);
}

ROSE_INLINE const BoundBox *alice_shadow_object_shadow_box_get(DRWAliceViewportPrivateData *impl, Object *object, AliceDrawData *add) {
	if ((add->flag & ALICE_SHADOW_BOX_DIRTY) != 0 || impl->shadow_changed) {
		float mat[4][4];
		mul_m4_m4m4(mat, impl->shadow_inv, object->obmat);

		/* Get AABB in shadow space. */
		INIT_MINMAX(add->shadow_min, add->shadow_max);

		/* From object space to shadow space */
		const BoundBox *box = KER_object_boundbox_get(object);
		for (size_t i = 0; i < 8; i++) {
			/**
			 * This does not take into consideration the armature deformation that 
			 * takes place on the GPU!
			 */

			float corner[3];
			mul_v3_m4v3(corner, mat, box->vec[i]);
			minmax_v3v3_v3(add->shadow_min, add->shadow_max, corner);
		}
		add->shadow_depth = add->shadow_max[2] - add->shadow_min[2];
		/* Extend towards infinity. */
		add->shadow_max[2] += 1e4f;

		/* Get extended AABB in world space. */
		KER_boundbox_init_from_minmax(&add->shadow_box, add->shadow_min, add->shadow_max);
		for (size_t i = 0; i < 8; i++) {
			mul_m4_v3(impl->shadow_mat, add->shadow_box.vec[i]);
		}
		add->flag &= ~ALICE_SHADOW_BOX_DIRTY;
	}

	return &add->shadow_box;
}

ROSE_INLINE bool alice_shadow_object_cast_visible_shadow(DRWAliceViewportPrivateData *impl, Object *object, AliceDrawData *add) {
	const BoundBox *shadow_box = alice_shadow_object_shadow_box_get(impl, object, add);

	return DRW_culling_box_test(NULL, shadow_box);
}

ROSE_INLINE float alice_shadow_object_shadow_distance(DRWAliceViewportPrivateData *impl, Object *object, AliceDrawData *add) {
	const BoundBox *shadow_bbox = alice_shadow_object_shadow_box_get(impl, object, add);

	const size_t corners[4] = {0, 3, 4, 7};

	float dist = 1e4f;
	float isect;

	for (size_t i = 0; i < ARRAY_SIZE(corners); i++) {
		if (isect_ray_plane_v3(shadow_bbox->vec[corners[i]], impl->shadow_cached_direction, impl->shadow_far_plane, &isect, true)) {
			if (isect < dist) {
				dist = isect;
			}
		}
		else {
			/* All rays are parallels. If one fails, the other will too. */
			break;
		}
	}

	return ROSE_MAX(dist - add->shadow_depth, 0);
}

ROSE_INLINE bool alice_shadow_camera_in_object_shadow(DRWAliceViewportPrivateData *impl, Object *object, AliceDrawData *add) {
	/* Just to be sure the min, max are updated. */
	alice_shadow_object_shadow_box_get(impl, object, add);
	/* Test if near plane is in front of the shadow. */
	if (add->shadow_min[2] > impl->shadow_near_max[2]) {
		return false;
	}

	/* Separation Axis Theorem test */

	/* Test bbox sides first (faster) */
	if ((add->shadow_min[0] > impl->shadow_near_max[0]) ||
		(add->shadow_max[0] < impl->shadow_near_min[0]) ||
		(add->shadow_min[1] > impl->shadow_near_max[1]) ||
		(add->shadow_max[1] < impl->shadow_near_min[1])) {
		return false;
	}

	/* Test projected near rectangle sides */
	const float pts[4][2] = {
		{add->shadow_min[0], add->shadow_min[1]},
		{add->shadow_min[0], add->shadow_max[1]},
		{add->shadow_max[0], add->shadow_min[1]},
		{add->shadow_max[0], add->shadow_max[1]},
	};

	for (size_t i = 0; i < 2; i++) {
		float min_dst = FLT_MAX, max_dst = -FLT_MAX;
		for (size_t j = 0; j < 4; j++) {
			float dst = dot_v2v2(impl->shadow_near_sides[i], pts[j]);

			/* Do min max */
			if (min_dst > dst) {
				min_dst = dst;
			}
			if (max_dst < dst) {
				max_dst = dst;
			}
		}

		if ((impl->shadow_near_sides[i][2] > max_dst) || (impl->shadow_near_sides[i][3] < min_dst)) {
			return false;
		}
	}

	/* No separation axis found. Both shape intersect. */
	return true;
}

void DRW_alice_shadow_cache_populate(DRWAliceData *vdata, Object *object) {
	DRWAliceViewportPrivateData *impl = vdata->stl->data;

	bool is_manifold;
	struct GPUBatch *shadow_geometry = DRW_cache_object_edge_detection_get(object, &is_manifold);
	if (shadow_geometry == NULL) {
		return;
	}

	AliceDrawData *add = DRW_alice_drawdata(object);

	if (alice_shadow_object_cast_visible_shadow(impl, object, add)) {
		mul_v3_mat3_m4v3(add->shadow_dir, object->invmat, impl->shadow_direction_ws);

		DRWShadingGroup *shgroup;
		bool use_shadow_pass_technique = !alice_shadow_camera_in_object_shadow(impl, object, add);

		/* We cannot use Shadow Pass technique on non-manifold object. */
		if (use_shadow_pass_technique && !is_manifold) {
			use_shadow_pass_technique = false;
		}

		GPUBatch *geometry = DRW_cache_object_surface_get(object);

		if (use_shadow_pass_technique) {
			shgroup = DRW_shading_subgroup_new(impl->shadow_pass_shgroup[is_manifold]);
			/** Ready all the required modifier data blocks for rendering on this group. */
			DRW_alice_modifier_list_build(shgroup, object);
			DRW_shading_group_uniform_v3(shgroup, "lightDirection", add->shadow_dir, 1);
			DRW_shading_group_uniform_float(shgroup, "lightDistance", 1e5f);
			DRW_shading_group_call_ex(shgroup, object, object->obmat, shadow_geometry);
		}
		else {
			float extrude = alice_shadow_object_shadow_distance(impl, object, add);

			const bool need_caps = true;
			if (need_caps) {
				shgroup = DRW_shading_subgroup_new(impl->shadow_caps_shgroup[is_manifold]);
				/** Ready all the required modifier data blocks for rendering on this group. */
				DRW_alice_modifier_list_build(shgroup, object);
				DRW_shading_group_uniform_v3(shgroup, "lightDirection", add->shadow_dir, 1);
				DRW_shading_group_uniform_float(shgroup, "lightDistance", extrude);
				DRW_shading_group_call_ex(shgroup, object, object->obmat, geometry);
			}

			shgroup = DRW_shading_subgroup_new(impl->shadow_fail_shgroup[is_manifold]);
			/** Ready all the required modifier data blocks for rendering on this group. */
			DRW_alice_modifier_list_build(shgroup, object);
			DRW_shading_group_uniform_v3(shgroup, "lightDirection", add->shadow_dir, 1);
			DRW_shading_group_uniform_float(shgroup, "lightDistance", extrude);
			DRW_shading_group_call_ex(shgroup, object, object->obmat, shadow_geometry);
		}
	}
}

void DRW_alice_shadow_cache_finish(DRWAliceData *vdata) {
}

/** \} */
