#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "GPU_framebuffer.h"

#include "LIB_math_vector.h"
#include "LIB_math_matrix.h"

#include "KER_armature.h"
#include "KER_object.h"

#include "overlay_engine.h"
#include "overlay_private.h"

#include "intern/draw_defines.h"
#include "intern/draw_manager.h"

struct DRWArmatureContext;

/* Forward declaration. */
ROSE_STATIC void overlay_armature_draw(struct DRWArmatureContext *ctx);

typedef struct DRWArmatureContext {
	/** Current armature object */
	Object *object;

	union {
		struct {
			DRWCallBuffer *outline;
			DRWCallBuffer *solid;
		};
	};
} DRWArmatureContext;

void overlay_armature_cache_init(DRWOverlayData *vdata) {
	DRWOverlayViewportPassList *psl = vdata->psl;
	DRWOverlayViewportStorageList *stl = vdata->stl;

	DRWOverlayViewportPrivateData *pd = stl->data;

	DRWOverlayInstanceFormats *formats = DRW_overlay_shader_instance_formats();
	DRWOverlayArmatureCallBuffers *cb = &pd->armature_call_buffers;

	GPUShader *armature_shape = DRW_overlay_shader_armature_shape();
	if ((psl->armature_pass = DRW_pass_new("Armature Pass", DRW_STATE_DEFAULT))) {
		GPUVertFormat *format = formats->instance_bone;

		pd->armature_shgroup = DRW_shading_group_new(armature_shape, psl->armature_pass);
		DRW_shading_group_clear_ex(pd->armature_shgroup, GPU_DEPTH_BIT, NULL, 1.0f, 0);
		cb->solid.octahaedron = DRW_shading_group_call_buffer_instance(pd->armature_shgroup, format, DRW_cache_bone_octahedral_get());
	}
}

void overlay_armature_context_init(DRWArmatureContext *ctx, DRWOverlayViewportPrivateData *pd, Object *object) {
	Armature *armature = (Armature *)object->data;

	DRWOverlayArmatureCallBuffers *cb = &pd->armature_call_buffers;

	ctx->outline = NULL;
	ctx->solid = cb->solid.octahaedron;
	ctx->object = object;
}

void overlay_armature_cache_populate(DRWOverlayData *vdata, Object *object) {
	DRWOverlayViewportStorageList *stl = vdata->stl;
	DRWOverlayViewportPrivateData *pd = stl->data;

	ROSE_assert(object->type == OB_ARMATURE);
	DRWArmatureContext context;
	overlay_armature_context_init(&context, pd, object);
	overlay_armature_draw(&context);
}

ROSE_INLINE void draw_bone_update_disp_matrix_default(PoseChannel *posechannel) {
	float bone_scale[3];
	float (*bone_mat)[4];
	float (*disp_mat)[4];
	float (*disp_tail_mat)[4];

	PoseChannel_Runtime *runtime = &posechannel->runtime;

	bone_mat = posechannel->pose_mat;
	disp_mat = runtime->disp_mat;
	disp_tail_mat = runtime->disp_tail_mat;
	copy_v3_fl(bone_scale, posechannel->bone->length);

	copy_m4_m4(disp_mat, bone_mat);
	rescale_m4(disp_mat, bone_scale);
	copy_m4_m4(disp_tail_mat, disp_mat);
	translate_m4(disp_tail_mat, 0.0f, 1.0f, 0.0f);
}

/* Encode 2 units float with byte precision into a float. */
static float encode_2f_to_float(float a, float b) {
	CLAMP(a, 0.0f, 1.0f);
	CLAMP(b, 0.0f, 2.0f); /* Can go up to 2. Needed for wire size. */
	return (float)((int)(a * 255) | ((int)(b * 255) << 8));
}

void overlay_bone_instance_data_set_color_hint(BoneInstanceData *data, const float hint_color[4]) {
	/* Encoded color into 2 floats to be able to use the obmat to color the custom bones. */
	data->color_hint_a = encode_2f_to_float(hint_color[0], hint_color[1]);
	data->color_hint_b = encode_2f_to_float(hint_color[2], hint_color[3]);
}

void overlay_bone_instance_data_set_color(BoneInstanceData *data, const float bone_color[4]) {
	/* Encoded color into 2 floats to be able to use the obmat to color the custom bones. */
	data->color_a = encode_2f_to_float(bone_color[0], bone_color[1]);
	data->color_b = encode_2f_to_float(bone_color[2], bone_color[3]);
}

ROSE_INLINE void draw_shading_group_bone_octahedral(DRWArmatureContext *context, const float (*bone_mat)[4]) {
	BoneInstanceData data;
	mul_m4_m4m4(data.mat, context->object->obmat, bone_mat);

	if (context->solid) {
		overlay_bone_instance_data_set_color(&data, (const float[4]){0.0f, 1.0f, 0.0f, 1.0f});
		overlay_bone_instance_data_set_color_hint(&data, (const float[4]){0.0f, 0.0f, 0.0f, 1.0f});
		DRW_buffer_add_entry_struct(context->solid, &data);
	}
}

ROSE_INLINE void draw_bone_octahedral(DRWArmatureContext *context, PoseChannel *posechannel, Armature *armature) {
	PoseChannel_Runtime *runtime = &posechannel->runtime;

	draw_shading_group_bone_octahedral(context, runtime->disp_mat);
}

ROSE_STATIC void overlay_armature_draw(DRWArmatureContext *context) {
	Object *object = context->object;
	Armature *armature = (Armature *)object->data;

	/** This is not actually invalid we just don't support it yet... */
	if (armature->ebonebase != NULL) {
		return;
	}

	/* We can't safely draw non-updated pose, might contain NULL bone pointers... */
	if (object->pose->flag & POSE_RECALC) {
		return;
	}

	LISTBASE_FOREACH(PoseChannel *, posechannel, &object->pose->channelbase) {
		Bone *bone = posechannel->bone;

		if ((bone->flag & BONE_HIDDEN) != 0) {
			continue;
		}

		draw_bone_update_disp_matrix_default(posechannel);
		draw_bone_octahedral(context, posechannel, armature);
	}
}
