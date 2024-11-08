#include "LIB_assert.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "GPU_info.h"
#include "GPU_vertex_format.h"

#include "gpu_vertex_format_private.h"

/* -------------------------------------------------------------------- */
/** \name Initialization
 * \{ */

void GPU_vertformat_clear(GPUVertFormat *format) {
	memset(format, 0, sizeof(GPUVertFormat));
}

void GPU_vertformat_copy(GPUVertFormat *destination, const GPUVertFormat *source) {
	memcpy(destination, source, sizeof(GPUVertFormat));
}

/* \} */

static unsigned int comp_size(VertCompType type) {
	const unsigned int sizes[] = {1, 1, 2, 2, 4, 4, 4, 4};
	return sizes[type];
}

static unsigned int attr_size(const GPUVertAttr *attr) {
	if (attr->comp_type == GPU_COMP_I10) {
		return 4;
	}
	return attr->comp_len * comp_size(static_cast<VertCompType>(attr->comp_type));
}

static unsigned int attr_align(const GPUVertAttr *attr, const unsigned int minimum_stride) {
	if (attr->comp_type == GPU_COMP_I10) {
		return 4;
	}
	unsigned int c = comp_size(static_cast<VertCompType>(attr->comp_type));
	if (attr->comp_len == 3 && c <= 2) {
		return 4 * c;
	}
	return ROSE_MAX(minimum_stride, c);
}

/* -------------------------------------------------------------------- */
/** \name Attribute Updates
 * \{ */

static unsigned char copy_attr_name(GPUVertFormat *format, const char *name) {
	unsigned char name_offset = format->name_offset;
	char *name_copy = format->names + name_offset;
	unsigned int available = GPU_VERT_ATTR_NAMES_BUF_LEN - name_offset;
	bool terminated = false;

	for (unsigned int i = 0; i < available; i++) {
		const char c = name[i];
		name_copy[i] = c;
		if (c == '\0') {
			terminated = true;
			format->name_offset += (i + 1);
			break;
		}
	}

	ROSE_assert(terminated);
	ROSE_assert(format->name_offset <= GPU_VERT_ATTR_NAMES_BUF_LEN);
	/** Silence the unused variable warning on Release mode. */
	UNUSED_VARS_NDEBUG(terminated);
	return name_offset;
}

unsigned int GPU_vertformat_add(GPUVertFormat *format, const char *name, VertCompType comp_type, unsigned int comp_len,
								VertFetchMode fetch_mode) {
#ifndef NDEBUG
	ROSE_assert(format->name_len < GPU_VERT_FORMAT_MAX_NAMES);
	ROSE_assert(format->attr_len < GPU_VERT_ATTR_MAX_LEN);
	ROSE_assert(!format->packed);
	ROSE_assert((comp_len >= 1 && comp_len <= 4) || comp_len == 8 || comp_len == 12 || comp_len == 16);

	switch (comp_type) {
		case GPU_COMP_F32: {
			ROSE_assert(fetch_mode == GPU_FETCH_FLOAT);
		} break;
		case GPU_COMP_I10: {
			ROSE_assert(ELEM(comp_len, 3, 4));
			ROSE_assert(fetch_mode == GPU_FETCH_INT_TO_FLOAT_UNIT);
		} break;
		default: {
			ROSE_assert(fetch_mode != GPU_FETCH_FLOAT);
			ROSE_assert(!ELEM(comp_len, 8, 12, 16));
		} break;
	}
#endif
	format->name_len++;

	const unsigned int attr_id = format->attr_len++;
	GPUVertAttr *attr = &format->attrs[attr_id];

	attr->names[attr->name_len++] = copy_attr_name(format, name);
	attr->comp_type = comp_type;
	attr->comp_len = (comp_type == GPU_COMP_I10) ? 4 : comp_len;
	attr->size = attr_size(attr);
	attr->offset = 0;
	attr->fetch_mode = fetch_mode;

	return attr_id;
}

void GPU_vertformat_alias_add(GPUVertFormat *format, const char *alias) {
	GPUVertAttr *attr = &format->attrs[format->attr_len - 1];

	ROSE_assert(format->name_len < GPU_VERT_FORMAT_MAX_NAMES);
	ROSE_assert(attr->name_len < GPU_VERT_ATTR_MAX_NAMES);

	format->name_len++;
	attr->names[attr->name_len++] = copy_attr_name(format, alias);
}

void GPU_vertformat_deinterleave(GPUVertFormat *format) {
	format->deinterleaved = true;
}

int GPU_vertformat_attr_id_get(const GPUVertFormat *format, const char *name) {
	for (unsigned int i = 0; i < format->attr_len; i++) {
		const GPUVertAttr *attr = &format->attrs[i];
		for (unsigned int j = 0; j < attr->name_len; j++) {
			const char *attr_name = GPU_vertformat_attr_name_get(format, attr, j);
			if (STREQ(name, attr_name)) {
				return static_cast<int>(i);
			}
		}
	}
	return -1;
}

void GPU_vertformat_attr_rename(GPUVertFormat *format, unsigned int attr_id, const char *new_name) {
	ROSE_assert(attr_id >= 0 && attr_id < format->attr_len);
	GPUVertAttr *attr = &format->attrs[attr_id];
	char *attr_name = (char *)GPU_vertformat_attr_name_get(format, attr, 0);
	ROSE_assert(LIB_strlen(attr_name) == LIB_strlen(new_name));
	int i = 0;
	while (attr_name[i] != '\0') {
		attr_name[i] = new_name[i];
		i++;
	}
	attr->name_len = 1;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Private
 * \{ */

static void VertexFormat_pack_impl(GPUVertFormat *format, unsigned int minimum_stride) {
	GPUVertAttr *a0 = &format->attrs[0];
	a0->offset = 0;
	unsigned int offset = a0->size;

	for (unsigned int a_idx = 1; a_idx < format->attr_len; a_idx++) {
		GPUVertAttr *a = &format->attrs[a_idx];
		unsigned int mid_padding = padding(offset, attr_align(a, minimum_stride));
		offset += mid_padding;
		a->offset = offset;
		offset += a->size;
	}

	unsigned int end_padding = padding(offset, attr_align(a0, minimum_stride));

	format->stride = offset + end_padding;
	format->packed = true;
}

void VertexFormat_pack(struct GPUVertFormat *format) {
	VertexFormat_pack_impl(format, GPU_get_info_i(GPU_INFO_MINIMUM_PER_VERTEX_STRIDE));
}
void VertexFormat_texture_buffer_pack(struct GPUVertFormat *format) {
	for (unsigned int i = 0; i < format->attr_len; i++) {
		ROSE_assert_msg(format->attrs[i].size == format->attrs[0].size,
						"Texture buffer mode should only use a attributes with the same size.");
	}

	VertexFormat_pack_impl(format, 1);
}

unsigned int padding(unsigned int offset, unsigned int alignment) {
	const unsigned int mod = offset % alignment;
	return (mod == 0) ? 0 : (alignment - mod);
}
unsigned int vertex_buffer_size(const struct GPUVertFormat *format, unsigned int vertex_length) {
	ROSE_assert(format->packed && format->stride > 0);
	return format->stride * vertex_length;
}

/* \} */
