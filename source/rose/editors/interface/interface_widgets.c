#include "GPU_batch.h"
#include "GPU_batch_presets.h"
#include "GPU_context.h"
#include "GPU_vertex_buffer.h"
#include "GPU_vertex_format.h"

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "interface_intern.h"

#define WIDGET_CURVE_RESOLU 9
#define WIDGET_SIZE_MAX (WIDGET_CURVE_RESOLU * 4)

/* -------------------------------------------------------------------- */
/** \name #GPU_Batch Creation
 *
 * In order to speed up UI drawing we create some batches that are then
 * modified by specialized shaders to draw certain elements really fast.
 * TODO: find a better place. Maybe its own file?
 *
 * \{ */

static struct {
	GPUBatch *roundbox_widget;
	GPUBatch *roundbox_shadow;

	/* TODO: remove. */
	GPUVertFormat format;
	unsigned int vflag_id;
} g_ui_batch_cache;

static GPUVertFormat *vflag_format() {
	if (g_ui_batch_cache.format.attr_len == 0) {
		GPUVertFormat *format = &g_ui_batch_cache.format;
		g_ui_batch_cache.vflag_id = GPU_vertformat_add(format, "vflag", GPU_COMP_U32, 1, GPU_FETCH_INT);
	}
	return &g_ui_batch_cache.format;
}

#define INNER 0
#define OUTLINE 1
#define EMBOSS 2
#define NO_AA 0

static void set_roundbox_vertex_data(GPUVertBufRaw *vflag_step, uint32_t d) {
	uint32_t *data = GPU_vertbuf_raw_step(vflag_step);
	*data = d;
}

static uint32_t set_roundbox_vertex(GPUVertBufRaw *vflag_step, int corner_id, int corner_v, int jit_v, bool inner, bool emboss, int color) {
	uint32_t *data = GPU_vertbuf_raw_step(vflag_step);
	*data = corner_id;
	*data |= corner_v << 2;
	*data |= jit_v << 6;
	*data |= color << 12;
	*data |= (inner) ? (1 << 10) : 0;  /* is inner vert */
	*data |= (emboss) ? (1 << 11) : 0; /* is emboss vert */
	return *data;
}

GPUBatch *ui_batch_roundbox_widget_get() {
	if (g_ui_batch_cache.roundbox_widget == NULL) {
		GPUVertBuf *vbo = GPU_vertbuf_create_with_format(vflag_format());

		GPU_vertbuf_data_alloc(vbo, 12);

		GPUIndexBufBuilder ibuf;
		GPU_indexbuf_init(&ibuf, GPU_PRIM_TRIS, 6, 12);
		/* Widget */
		GPU_indexbuf_add_tri_verts(&ibuf, 0, 1, 2);
		GPU_indexbuf_add_tri_verts(&ibuf, 2, 1, 3);
		/* Trias */
		GPU_indexbuf_add_tri_verts(&ibuf, 4, 5, 6);
		GPU_indexbuf_add_tri_verts(&ibuf, 6, 5, 7);

		GPU_indexbuf_add_tri_verts(&ibuf, 8, 9, 10);
		GPU_indexbuf_add_tri_verts(&ibuf, 10, 9, 11);

		g_ui_batch_cache.roundbox_widget = GPU_batch_create_ex(GPU_PRIM_TRIS, vbo, GPU_indexbuf_build(&ibuf), GPU_BATCH_OWNS_INDEX | GPU_BATCH_OWNS_VBO);
		gpu_batch_presets_register(g_ui_batch_cache.roundbox_widget);
	}
	return g_ui_batch_cache.roundbox_widget;
}

GPUBatch *ui_batch_roundbox_shadow_get() {
	if (g_ui_batch_cache.roundbox_shadow == NULL) {
		uint32_t last_data;
		GPUVertBufRaw vflag_step;
		GPUVertBuf *vbo = GPU_vertbuf_create_with_format(vflag_format());
		const int vcount = (WIDGET_SIZE_MAX + 1) * 2 + 2 + WIDGET_SIZE_MAX;
		GPU_vertbuf_data_alloc(vbo, vcount);
		GPU_vertbuf_attr_get_raw_data(vbo, g_ui_batch_cache.vflag_id, &vflag_step);

		for (int c = 0; c < 4; c++) {
			for (int a = 0; a < WIDGET_CURVE_RESOLU; a++) {
				set_roundbox_vertex(&vflag_step, c, a, NO_AA, true, false, INNER);
				set_roundbox_vertex(&vflag_step, c, a, NO_AA, false, false, INNER);
			}
		}
		/* close loop */
		last_data = set_roundbox_vertex(&vflag_step, 0, 0, NO_AA, true, false, INNER);
		last_data = set_roundbox_vertex(&vflag_step, 0, 0, NO_AA, false, false, INNER);
		/* restart */
		set_roundbox_vertex_data(&vflag_step, last_data);
		set_roundbox_vertex(&vflag_step, 0, 0, NO_AA, true, false, INNER);
		/* filled */
		for (int c1 = 0, c2 = 3; c1 < 2; c1++, c2--) {
			for (int a1 = 0, a2 = WIDGET_CURVE_RESOLU - 1; a2 >= 0; a1++, a2--) {
				set_roundbox_vertex(&vflag_step, c1, a1, NO_AA, true, false, INNER);
				set_roundbox_vertex(&vflag_step, c2, a2, NO_AA, true, false, INNER);
			}
		}
		g_ui_batch_cache.roundbox_shadow = GPU_batch_create_ex(GPU_PRIM_TRI_STRIP, vbo, NULL, GPU_BATCH_OWNS_VBO);
		gpu_batch_presets_register(g_ui_batch_cache.roundbox_shadow);
	}
	return g_ui_batch_cache.roundbox_shadow;
}

#undef INNER
#undef OUTLINE
#undef EMBOSS
#undef NO_AA

/** \} */
