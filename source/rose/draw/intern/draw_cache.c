#include "GPU_batch.h"

#include "LIB_math_matrix.h"
#include "LIB_math_vector.h"

typedef struct DRWCache {
	GPUBatch *draw_fullscreen_quad;
} DRWCache;

static struct DRWCache GCache; // = NULL;

GPUBatch *DRW_cache_fullscreen_quad_get(void);

GPUBatch *DRW_cache_fullscreen_quad_get(void) {
	if (GCache.draw_fullscreen_quad == NULL) {
		const float pos[3][2] = {
			{-1.0f, -1.0f},
			{ 3.0f, -1.0f},
			{-1.0f,  3.0f},
		};
		const float tex[3][2] = {
			{ 0.0f, 0.0f},
			{ 2.0f, 0.0f},
			{ 0.0f, 2.0f},
		};

		/**
		 * Affine geometry, given w_0, w_1, w_2; w_0 + w_1 + w_2 = 1 and
		 * pos[0] * w_0 + pos[1] * w_1 + pos[2] * w_2 = [1.0f, 1.0f]
		 * 
		 * | pos[0].x, pos[1].x, pos[2].x | | w_0 |   | x |
		 * | pos[0].y, pos[1].y, pos[2].y | | w_1 | = | y |
		 * |    1    ,    1    ,    1     | | w_2 |   | 1 |
		 * 
		 * w = A^-1 * b
		 */
#ifndef NDEBUG
		float m[3][3] = {
			{pos[0][0], pos[0][1], 1},
			{pos[1][0], pos[1][1], 1},
			{pos[2][0], pos[2][1], 1},
		};
		float w[3] = {1, 1, 1};
		invert_m3(m);
		mul_m3_v3(m, w);
		float u = w[0] * tex[0][0] + w[1] * tex[1][0] + w[2] * tex[2][0];
		float v = w[0] * tex[0][1] + w[1] * tex[1][1] + w[2] * tex[2][1];
		ROSE_assert(fabs(u - 1.0f) <= FLT_EPSILON && abs(v - 1.0f) <= FLT_EPSILON);
#endif

		static GPUVertFormat format;
		GPU_vertformat_clear(&format);

		int apos = GPU_vertformat_add(&format, "pos", GPU_COMP_F32, 2, GPU_FETCH_FLOAT);
		int atex = GPU_vertformat_add(&format, "uv", GPU_COMP_F32, 2, GPU_FETCH_FLOAT);
		GPU_vertformat_alias_add(&format, "texCoord");

		do {
			GPUVertBuf *vbo = GPU_vertbuf_create_with_format(&format);
			GPU_vertbuf_data_alloc(vbo, 3);

			for (int index = 0; index < 3; index++) {
				GPU_vertbuf_attr_set(vbo, apos, index, pos[index]);
				GPU_vertbuf_attr_set(vbo, atex, index, tex[index]);
			}

			GCache.draw_fullscreen_quad = GPU_batch_create_ex(GPU_PRIM_TRIS, vbo, NULL, GPU_BATCH_OWNS_VBO);
		} while (false);
	}
	return GCache.draw_fullscreen_quad;
}

void DRW_global_cache_free(void) {
	for (GPUBatch **itr = (GPUBatch **)&GCache; itr != (GPUBatch **)(&GCache + 1); itr++) {
		GPU_BATCH_DISCARD_SAFE(*itr);
	}
	// memset(&GCache, 0, sizeof(GCache));
}
