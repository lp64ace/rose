#pragma once

#include "LIB_assert.h"
#include "LIB_utildefines.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define GPU_VERT_ATTR_MAX_LEN 16
#define GPU_VERT_ATTR_MAX_NAMES 6
#define GPU_VERT_ATTR_NAMES_BUF_LEN 256
#define GPU_VERT_FORMAT_MAX_NAMES 63
#define GPU_VERT_SAFE_ATTR_NAME 12

typedef enum VertCompType {
	GPU_COMP_I8 = 0,
	GPU_COMP_U8,
	GPU_COMP_I16,
	GPU_COMP_U16,
	GPU_COMP_I32,
	GPU_COMP_U32,
	GPU_COMP_F32,
	GPU_COMP_I10,
	GPU_COMP_MAX,
} VertCompType;

typedef enum VertFetchMode {
	GPU_FETCH_FLOAT = 0,
	GPU_FETCH_INT,
	GPU_FETCH_INT_TO_FLOAT_UNIT,
	GPU_FETCH_INT_TO_FLOAT,
} VertFetchMode;

typedef struct GPUVertAttr {
	unsigned int fetch_mode : 2;
	unsigned int comp_type : 3;
	unsigned int comp_len : 5;
	unsigned int size : 7;
	unsigned int offset : 11;
	unsigned int name_len : 3;
	unsigned char names[GPU_VERT_ATTR_MAX_NAMES];
} GPUVertAttr;

typedef struct GPUVertFormat {
	unsigned int attr_len : 5;
	unsigned int name_len : 6;
	unsigned int stride : 11;
	unsigned int packed : 1;
	unsigned int name_offset : 8;
	unsigned int deinterleaved : 1;

	GPUVertAttr attrs[GPU_VERT_ATTR_MAX_LEN];
	char names[GPU_VERT_ATTR_NAMES_BUF_LEN];
} GPUVertFormat;

struct GPUShader;

void GPU_vertformat_clear(struct GPUVertFormat *format);
void GPU_vertformat_copy(struct GPUVertFormat *destination, const struct GPUVertFormat *source);

unsigned int GPU_vertformat_add(struct GPUVertFormat *, const char *name, VertCompType, unsigned int comp_len, VertFetchMode);
void GPU_vertformat_alias_add(struct GPUVertFormat *, const char *alias);

/**
 * Make attribute layours non-interleaved.
 * Warning this does not change data layout! Use indirect buffer access to fill the data!
 * This is for advanced usage.
 *
 * De-interleaved data means all attribute data for each attribute is stored continuously like this:
 * 000011112222
 * instead of:
 * 012012012012
 */
void GPU_vertformat_deinterleave(struct GPUVertFormat *format);

int GPU_vertformat_attr_id_get(const struct GPUVertFormat *format, const char *name);

ROSE_INLINE const char *GPU_vertformat_attr_name_get(const struct GPUVertFormat *format, const struct GPUVertAttr *attr, unsigned int n_idx) {
	return format->names + attr->names[n_idx];
}

/** Can only rename using a string with same charachter count, this removes all other aliases of this attribute. */
void GPU_vertformat_attr_rename(struct GPUVertFormat *format, unsigned int attr, const char *new_name);

typedef struct GPUPackedNormal {
	int x : 10;
	int y : 10;
	int z : 10;
	int w : 2;
} GPUPackedNormal;

typedef struct GPUNormal {
	union {
		GPUPackedNormal low;
		short high[3];
	};
} GPUNormal;

#define SIGNED_INT_10_MAX 511
#define SIGNED_INT_10_MIN -512

ROSE_INLINE int clampi(int x, int min_allowed, int max_allowed) {
	if (x < min_allowed) {
		return min_allowed;
	}
	if (x > max_allowed) {
		return max_allowed;
	}
	return x;
}

ROSE_INLINE int gpu_convert_normalized_f32_to_i10(float x) {
	int qx = (int)(x * 511.0f);
	return clampi(qx, SIGNED_INT_10_MIN, SIGNED_INT_10_MAX);
}

ROSE_INLINE int gpu_convert_i16_to_i10(short x) {
	return x >> 16;
}

ROSE_INLINE GPUPackedNormal GPU_normal_convert_i10_v3(const float data[3]) {
	GPUPackedNormal n = {
		gpu_convert_normalized_f32_to_i10(data[0]),
		gpu_convert_normalized_f32_to_i10(data[1]),
		gpu_convert_normalized_f32_to_i10(data[2]),
	};
	return n;
}

ROSE_INLINE GPUPackedNormal GPU_normal_convert_i10_s3(const short data[3]) {
	GPUPackedNormal n = {
		gpu_convert_i16_to_i10(data[0]),
		gpu_convert_i16_to_i10(data[1]),
		gpu_convert_i16_to_i10(data[2]),
	};
	return n;
}

ROSE_INLINE void GPU_normal_convert_v3(struct GPUNormal *normal, const float data[3], const bool hq) {
	if (hq) {
		normal->high[0] = (short)(data[0] * 32767.0f);
		normal->high[1] = (short)(data[1] * 32767.0f);
		normal->high[2] = (short)(data[2] * 32767.0f);
	}
	else {
		normal->low = GPU_normal_convert_i10_v3(data);
	}
}

#if defined(__cplusplus)
}
#endif
