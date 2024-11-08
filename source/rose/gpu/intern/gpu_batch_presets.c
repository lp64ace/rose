#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "MEM_guardedalloc.h"

#include "GPU_batch.h"
#include "GPU_batch_presets.h"

/* -------------------------------------------------------------------- */
/** \name Local Structures
 * \{ */

typedef struct GPresets3D {
	GPUVertFormat format;

	struct {
		uint pos;
		uint nor;
	} attr_id;
} GPresets3D;

typedef struct GPresets2D {
	struct {
		GPUBatch *quad;
	} batch;

	GPUVertFormat format;

	struct {
		uint pos;
		uint col;
	} attr_id;
} GPresets2D;

static GPresets3D g_presets_3d;
static GPresets2D g_presets_2d;

static ListBase presets_list = {NULL, NULL};

/* \} */

/* -------------------------------------------------------------------- */
/** \name Batch Formats
 * \{ */

static GPUVertFormat *preset_3d_format() {
	if (g_presets_3d.format.attr_len == 0) {
		GPUVertFormat *format = &g_presets_3d.format;
		g_presets_3d.attr_id.pos = GPU_vertformat_add(format, "pos", GPU_COMP_F32, 3, GPU_FETCH_FLOAT);
		g_presets_3d.attr_id.nor = GPU_vertformat_add(format, "nor", GPU_COMP_F32, 3, GPU_FETCH_FLOAT);
	}
	return &g_presets_3d.format;
}

static GPUVertFormat *preset_2d_format() {
	if (g_presets_2d.format.attr_len == 0) {
		GPUVertFormat *format = &g_presets_2d.format;
		g_presets_2d.attr_id.pos = GPU_vertformat_add(format, "pos", GPU_COMP_F32, 2, GPU_FETCH_FLOAT);
		g_presets_2d.attr_id.col = GPU_vertformat_add(format, "color", GPU_COMP_F32, 4, GPU_FETCH_FLOAT);
	}
	return &g_presets_2d.format;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Preset Batch
 * \{ */

struct GPUBatch *GPU_batch_preset_quad(void) {
	if (!g_presets_2d.batch.quad) {
		GPUVertBuf *vbo = GPU_vertbuf_create_with_format(preset_2d_format());
		GPU_vertbuf_data_alloc(vbo, 4);

		float pos_data[4][2] = {{0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}};
		GPU_vertbuf_attr_fill(vbo, g_presets_2d.attr_id.pos, pos_data);
		/* Don't fill the color. */

		g_presets_2d.batch.quad = GPU_batch_create_ex(GPU_PRIM_TRI_STRIP, vbo, NULL, GPU_BATCH_OWNS_VBO);

		gpu_batch_presets_register(g_presets_2d.batch.quad);
	}
	return g_presets_2d.batch.quad;
}

/* \} */

void gpu_batch_presets_init(void) {
}

void gpu_batch_presets_register(struct GPUBatch *preset_batch) {
	LIB_addtail(&presets_list, LIB_generic_nodeN(preset_batch));
}
void gpu_batch_presets_unregister(struct GPUBatch *preset_batch) {
	LISTBASE_FOREACH_BACKWARD(LinkData *, link, &presets_list) {
		if (preset_batch == link->data) {
			LIB_remlink(&presets_list, link);
			MEM_freeN(link);
			break;
		}
	}
}

void gpu_batch_presets_exit(void) {
	LinkData *link;

	while (link = LIB_pophead(&presets_list)) {
		GPUBatch *preset = link->data;
		GPU_batch_discard(preset);
		MEM_freeN(link);
	}

	/** Reset pointers to NULL for subsequent initializations after tear-down. */
	memset(&g_presets_3d, 0, sizeof(GPresets3D));
	memset(&g_presets_2d, 0, sizeof(GPresets2D));
	LIB_listbase_clear(&presets_list);
}
