#include "GPU_immediate.h"
#include "GPU_matrix.h"
#include "GPU_texture.h"

#include "gpu_context_private.hh"
#include "gpu_immediate_private.hh"
#include "gpu_shader_private.hh"
#include "gpu_vertex_format_private.h"

using namespace rose::gpu;

static thread_local Immediate *imm = nullptr;

void immActivate() {
	imm = Context::get()->imm;
}

void immDeactivate() {
	imm = nullptr;
}

struct GPUVertFormat *immVertexFormat(void) {
	GPU_vertformat_clear(&imm->vertex_format);
	return &imm->vertex_format;
}

void immBindShader(struct GPUShader *shader) {
	ROSE_assert(imm->shader == nullptr);

	imm->shader = shader;
	imm->builtin_shader_bound = std::nullopt;

	if (!imm->vertex_format.packed) {
		VertexFormat_pack(&imm->vertex_format);
		imm->enabled_attr_bits = 0xFFFFu & ~(0xFFFFu << imm->vertex_format.attr_len);
	}

	GPU_shader_bind(shader);
	GPU_matrix_bind(shader);
	Shader::set_srgb_uniform(shader);
}

void immUnbindProgram(void) {
	ROSE_assert(imm->shader != nullptr);

	GPU_shader_unbind();
	imm->shader = nullptr;
}

void immBegin(PrimType prim, uint vertex_length) {
	ROSE_assert(imm->prim_type == GPU_PRIM_NONE); /* Make sure we haven't already begun. */

	imm->prim_type = prim;
	imm->vertex_len = vertex_length;
	imm->vertex_idx = 0;
	imm->unassigned_attr_bits = imm->enabled_attr_bits;

	imm->vertex_data = imm->begin();
}

void immBeginAtMost(PrimType prim, uint vertex_length) {
	ROSE_assert(vertex_length > 0);
	imm->strict_vertex_len = false;
	immBegin(prim, vertex_length);
}

void immEnd(void) {
	if (imm->strict_vertex_len) {
		ROSE_assert(imm->vertex_idx == imm->vertex_len); /* With all vertices defined. */
	}
	else {
		ROSE_assert(imm->vertex_idx <= imm->vertex_len);
	}

	if (imm->batch) {
		const uint shrink_threshold = (imm->vertex_len << 1);
		if (imm->vertex_idx <= shrink_threshold) {
			GPU_vertbuf_data_resize(imm->batch->verts[0], imm->vertex_idx);
		}
		GPU_batch_set_shader(imm->batch, imm->shader);
		imm->batch->flag &= ~GPU_BATCH_BUILDING;
		imm->batch = nullptr; /* don't free, batch belongs to caller */
	}
	else {
		imm->end();
	}

	/* Prepare for next immBegin. */
	imm->prim_type = GPU_PRIM_NONE;
	imm->strict_vertex_len = true;
	imm->vertex_data = nullptr;
}

struct GPUBatch *immBeginBatch(PrimType primitive, uint vertex_length) {
	ROSE_assert(imm->prim_type == GPU_PRIM_NONE); /* Make sure we haven't already begun. */

	imm->prim_type = primitive;
	imm->vertex_len = vertex_length;
	imm->vertex_idx = 0;
	imm->unassigned_attr_bits = imm->enabled_attr_bits;

	GPUVertBuf *verts = GPU_vertbuf_create_with_format(&imm->vertex_format);
	GPU_vertbuf_data_alloc(verts, vertex_length);

	imm->vertex_data = (unsigned char *)GPU_vertbuf_get_data(verts);

	imm->batch = GPU_batch_create_ex(primitive, verts, nullptr, GPU_BATCH_OWNS_VBO);
	imm->batch->flag |= GPU_BATCH_BUILDING;

	return imm->batch;
}

struct GPUBatch *immBeginBatchAtMost(PrimType primitive, uint vertex_length) {
	ROSE_assert(vertex_length > 0);
	imm->strict_vertex_len = false;
	return immBeginBatch(primitive, vertex_length);
}

static void setAttrValueBit(uint attr_id) {
	uint16_t mask = 1 << attr_id;
	ROSE_assert(imm->unassigned_attr_bits & mask); /* not already set */
	imm->unassigned_attr_bits &= ~mask;
}

void immAttr1f(uint attr_id, float x) {
	GPUVertAttr *attr = &imm->vertex_format.attrs[attr_id];
	ROSE_assert(attr_id < imm->vertex_format.attr_len);
	ROSE_assert(attr->comp_type == GPU_COMP_F32);
	ROSE_assert(attr->comp_len == 1);
	ROSE_assert(imm->vertex_idx < imm->vertex_len);
	ROSE_assert(imm->prim_type != GPU_PRIM_NONE); /* make sure we're between a Begin/End pair */
	setAttrValueBit(attr_id);

	float *data = (float *)(imm->vertex_data + attr->offset);

	data[0] = x;
}
void immAttr2f(uint attr_id, float x, float y) {
	GPUVertAttr *attr = &imm->vertex_format.attrs[attr_id];
	ROSE_assert(attr_id < imm->vertex_format.attr_len);
	ROSE_assert(attr->comp_type == GPU_COMP_F32);
	ROSE_assert(attr->comp_len == 2);
	ROSE_assert(imm->vertex_idx < imm->vertex_len);
	ROSE_assert(imm->prim_type != GPU_PRIM_NONE); /* make sure we're between a Begin/End pair */
	setAttrValueBit(attr_id);

	float *data = (float *)(imm->vertex_data + attr->offset);

	data[0] = x;
	data[1] = y;
}
void immAttr3f(uint attr_id, float x, float y, float z) {
	GPUVertAttr *attr = &imm->vertex_format.attrs[attr_id];
	ROSE_assert(attr_id < imm->vertex_format.attr_len);
	ROSE_assert(attr->comp_type == GPU_COMP_F32);
	ROSE_assert(attr->comp_len == 3);
	ROSE_assert(imm->vertex_idx < imm->vertex_len);
	ROSE_assert(imm->prim_type != GPU_PRIM_NONE); /* make sure we're between a Begin/End pair */
	setAttrValueBit(attr_id);

	float *data = (float *)(imm->vertex_data + attr->offset);

	data[0] = x;
	data[1] = y;
	data[2] = z;
}
void immAttr4f(uint attr_id, float x, float y, float z, float w) {
	GPUVertAttr *attr = &imm->vertex_format.attrs[attr_id];
	ROSE_assert(attr_id < imm->vertex_format.attr_len);
	ROSE_assert(attr->comp_type == GPU_COMP_F32);
	ROSE_assert(attr->comp_len == 4);
	ROSE_assert(imm->vertex_idx < imm->vertex_len);
	ROSE_assert(imm->prim_type != GPU_PRIM_NONE); /* make sure we're between a Begin/End pair */
	setAttrValueBit(attr_id);

	float *data = (float *)(imm->vertex_data + attr->offset);

	data[0] = x;
	data[1] = y;
	data[2] = z;
	data[3] = w;
}

void immAttr2i(uint attr_id, int x, int y) {
	GPUVertAttr *attr = &imm->vertex_format.attrs[attr_id];
	ROSE_assert(attr_id < imm->vertex_format.attr_len);
	ROSE_assert(attr->comp_type == GPU_COMP_I32);
	ROSE_assert(attr->comp_len == 2);
	ROSE_assert(imm->vertex_idx < imm->vertex_len);
	ROSE_assert(imm->prim_type != GPU_PRIM_NONE); /* make sure we're between a Begin/End pair */
	setAttrValueBit(attr_id);

	int *data = (int *)(imm->vertex_data + attr->offset);

	data[0] = x;
	data[1] = y;
}

void immAttr1u(uint attr_id, uint x) {
	GPUVertAttr *attr = &imm->vertex_format.attrs[attr_id];
	ROSE_assert(attr_id < imm->vertex_format.attr_len);
	ROSE_assert(attr->comp_type == GPU_COMP_U32);
	ROSE_assert(attr->comp_len == 1);
	ROSE_assert(imm->vertex_idx < imm->vertex_len);
	ROSE_assert(imm->prim_type != GPU_PRIM_NONE); /* make sure we're between a Begin/End pair */
	setAttrValueBit(attr_id);

	uint *data = (uint *)(imm->vertex_data + attr->offset);

	data[0] = x;
}

void immAttr2s(uint attr_id, short x, short y) {
	GPUVertAttr *attr = &imm->vertex_format.attrs[attr_id];
	ROSE_assert(attr_id < imm->vertex_format.attr_len);
	ROSE_assert(attr->comp_type == GPU_COMP_I16);
	ROSE_assert(attr->comp_len == 2);
	ROSE_assert(imm->vertex_idx < imm->vertex_len);
	ROSE_assert(imm->prim_type != GPU_PRIM_NONE); /* make sure we're between a Begin/End pair */
	setAttrValueBit(attr_id);

	short *data = (short *)(imm->vertex_data + attr->offset);

	data[0] = x;
	data[1] = y;
}

void immAttr2fv(uint attr_id, const float data[2]) {
	immAttr2f(attr_id, data[0], data[1]);
}
void immAttr3fv(uint attr_id, const float data[3]) {
	immAttr3f(attr_id, data[0], data[1], data[2]);
}
void immAttr4fv(uint attr_id, const float data[4]) {
	immAttr4f(attr_id, data[0], data[1], data[2], data[3]);
}

void immAttr3ub(uint attr_id, unsigned char r, unsigned char g, unsigned char b) {
	GPUVertAttr *attr = &imm->vertex_format.attrs[attr_id];
	ROSE_assert(attr_id < imm->vertex_format.attr_len);
	ROSE_assert(attr->comp_type == GPU_COMP_U8);
	ROSE_assert(attr->comp_len == 3);
	ROSE_assert(imm->vertex_idx < imm->vertex_len);
	ROSE_assert(imm->prim_type != GPU_PRIM_NONE); /* make sure we're between a Begin/End pair */
	setAttrValueBit(attr_id);

	unsigned char *data = imm->vertex_data + attr->offset;

	data[0] = r;
	data[1] = g;
	data[2] = b;
}

void immAttr4ub(uint attr_id, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	GPUVertAttr *attr = &imm->vertex_format.attrs[attr_id];
	ROSE_assert(attr_id < imm->vertex_format.attr_len);
	ROSE_assert(attr->comp_type == GPU_COMP_U8);
	ROSE_assert(attr->comp_len == 4);
	ROSE_assert(imm->vertex_idx < imm->vertex_len);
	ROSE_assert(imm->prim_type != GPU_PRIM_NONE); /* make sure we're between a Begin/End pair */
	setAttrValueBit(attr_id);

	unsigned char *data = imm->vertex_data + attr->offset;

	data[0] = r;
	data[1] = g;
	data[2] = b;
	data[3] = a;
}

void immAttr3ubv(uint attr_id, const unsigned char data[3]) {
	immAttr3ub(attr_id, data[0], data[1], data[2]);
}
void immAttr4ubv(uint attr_id, const unsigned char data[4]) {
	immAttr4ub(attr_id, data[0], data[1], data[2], data[3]);
}

void immAttrSkip(uint attr_id) {
	ROSE_assert(attr_id < imm->vertex_format.attr_len);
	ROSE_assert(imm->vertex_idx < imm->vertex_len);
	ROSE_assert(imm->prim_type != GPU_PRIM_NONE); /* make sure we're between a Begin/End pair */
	setAttrValueBit(attr_id);
}

static void immEndVertex() /* and move on to the next vertex */
{
	ROSE_assert(imm->prim_type != GPU_PRIM_NONE); /* make sure we're between a Begin/End pair */
	ROSE_assert(imm->vertex_idx < imm->vertex_len);

	/* Have all attributes been assigned values?
	 * If not, copy value from previous vertex. */
	if (imm->unassigned_attr_bits) {
		ROSE_assert(imm->vertex_idx > 0); /* first vertex must have all attributes specified */
		for (uint a_idx = 0; a_idx < imm->vertex_format.attr_len; a_idx++) {
			if ((imm->unassigned_attr_bits >> a_idx) & 1) {
				const GPUVertAttr *a = &imm->vertex_format.attrs[a_idx];

				unsigned char *data = imm->vertex_data + a->offset;
				memcpy(data, data - imm->vertex_format.stride, a->size);
				/* TODO: consolidate copy of adjacent attributes */
			}
		}
	}

	imm->vertex_idx++;
	imm->vertex_data += imm->vertex_format.stride;
	imm->unassigned_attr_bits = imm->enabled_attr_bits;
}

void immVertex2f(uint attr_id, float x, float y) {
	immAttr2f(attr_id, x, y);
	immEndVertex();
}

void immVertex3f(uint attr_id, float x, float y, float z) {
	immAttr3f(attr_id, x, y, z);
	immEndVertex();
}

void immVertex4f(uint attr_id, float x, float y, float z, float w) {
	immAttr4f(attr_id, x, y, z, w);
	immEndVertex();
}

void immVertex2i(uint attr_id, int x, int y) {
	immAttr2i(attr_id, x, y);
	immEndVertex();
}

void immVertex2s(uint attr_id, short x, short y) {
	immAttr2s(attr_id, x, y);
	immEndVertex();
}

void immVertex2fv(uint attr_id, const float data[2]) {
	immAttr2f(attr_id, data[0], data[1]);
	immEndVertex();
}

void immVertex3fv(uint attr_id, const float data[3]) {
	immAttr3f(attr_id, data[0], data[1], data[2]);
	immEndVertex();
}

void immVertex2iv(uint attr_id, const int data[2]) {
	immAttr2i(attr_id, data[0], data[1]);
	immEndVertex();
}

void immUniform1f(const char *name, float x) {
	GPU_shader_uniform_1f(imm->shader, name, x);
}

void immUniform2f(const char *name, float x, float y) {
	GPU_shader_uniform_2f(imm->shader, name, x, y);
}

void immUniform2fv(const char *name, const float data[2]) {
	GPU_shader_uniform_2fv(imm->shader, name, data);
}

void immUniform3f(const char *name, float x, float y, float z) {
	GPU_shader_uniform_3f(imm->shader, name, x, y, z);
}

void immUniform3fv(const char *name, const float data[3]) {
	GPU_shader_uniform_3fv(imm->shader, name, data);
}

void immUniform4f(const char *name, float x, float y, float z, float w) {
	GPU_shader_uniform_4f(imm->shader, name, x, y, z, w);
}

void immUniform4fv(const char *name, const float data[4]) {
	GPU_shader_uniform_4fv(imm->shader, name, data);
}

void immUniformArray4fv(const char *name, const float *data, int count) {
	GPU_shader_uniform_4fv_array(imm->shader, name, count, (const float(*)[4])data);
}

void immUniformMatrix4fv(const char *name, const float data[4][4]) {
	GPU_shader_uniform_mat4(imm->shader, name, data);
}

void immUniform1i(const char *name, int x) {
	GPU_shader_uniform_1i(imm->shader, name, x);
}

void immBindTexture(const char *name, GPUTexture *tex) {
	int binding = GPU_shader_get_sampler_binding(imm->shader, name);
	GPU_texture_bind(tex, binding);
}

void immBindTextureSampler(const char *name, GPUTexture *tex, GPUSamplerState state) {
	int binding = GPU_shader_get_sampler_binding(imm->shader, name);
	GPU_texture_bind_ex(tex, state, binding);
}

void immBindUniformBuf(const char *name, GPUUniformBuf *ubo) {
	int binding = GPU_shader_get_ubo_binding(imm->shader, name);
	GPU_uniformbuf_bind(ubo, binding);
}

void immUniformColor4f(float r, float g, float b, float a) {
	int32_t uniform_loc = GPU_shader_get_builtin_uniform(imm->shader, GPU_UNIFORM_COLOR);
	ROSE_assert(uniform_loc != -1);
	float data[4] = {r, g, b, a};
	GPU_shader_uniform_float_ex(imm->shader, uniform_loc, 4, 1, data);
	/* For wide Line workaround. */
	copy_v4_v4(imm->uniform_color, data);
}

void immUniformColor4fv(const float rgba[4]) {
	immUniformColor4f(rgba[0], rgba[1], rgba[2], rgba[3]);
}

void immUniformColor3f(float r, float g, float b) {
	immUniformColor4f(r, g, b, 1.0f);
}

void immUniformColor3fv(const float rgb[3]) {
	immUniformColor4f(rgb[0], rgb[1], rgb[2], 1.0f);
}

void immUniformColor3fvAlpha(const float rgb[3], float a) {
	immUniformColor4f(rgb[0], rgb[1], rgb[2], a);
}

void immUniformColor3ub(unsigned char r, unsigned char g, unsigned char b) {
	const float scale = 1.0f / 255.0f;
	immUniformColor4f(scale * r, scale * g, scale * b, 1.0f);
}

void immUniformColor4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	const float scale = 1.0f / 255.0f;
	immUniformColor4f(scale * r, scale * g, scale * b, scale * a);
}

void immUniformColor3ubv(const unsigned char rgb[3]) {
	immUniformColor3ub(rgb[0], rgb[1], rgb[2]);
}

void immUniformColor3ubvAlpha(const unsigned char rgb[3], unsigned char alpha) {
	immUniformColor4ub(rgb[0], rgb[1], rgb[2], alpha);
}

void immUniformColor4ubv(const unsigned char rgba[4]) {
	immUniformColor4ub(rgba[0], rgba[1], rgba[2], rgba[3]);
}

void immBindBuiltinProgram(BuiltinShader shader_id) {
	GPUShader *shader = GPU_shader_get_builtin_shader(shader_id);
	immBindShader(shader);
	imm->builtin_shader_bound = shader_id;
}
