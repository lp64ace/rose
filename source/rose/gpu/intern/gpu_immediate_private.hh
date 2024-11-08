#pragma once

#include <optional>

#include "GPU_batch.h"
#include "GPU_primitive.h"
#include "GPU_shader.h"
#include "GPU_vertex_format.h"

namespace rose::gpu {

class Immediate {
public:
	/** Pointer to the mapped buffer data for the current vertex. */
	unsigned char *vertex_data = nullptr;
	/** Current vertex index. */
	uint vertex_idx = 0;
	/** Length of the buffer in vertices. */
	uint vertex_len = 0;
	/** Which attributes of current vertex have not been given values? */
	uint16_t unassigned_attr_bits = 0;
	/** Attributes that needs to be set. One bit per attribute. */
	uint16_t enabled_attr_bits = 0;

	PrimType prim_type = GPU_PRIM_NONE;

	GPUVertFormat vertex_format = {};
	GPUShader *shader = nullptr;
	/** Enforce strict vertex count (disabled when using #immBeginAtMost). */
	bool strict_vertex_len = true;

	/** Batch in construction when using #immBeginBatch. */
	GPUBatch *batch = nullptr;

	/** Previously bound shader to restore after drawing. */
	std::optional<BuiltinShader> prev_builtin_shader;
	/** Builtin shader index. Used to test if the line width workaround can be done. */
	std::optional<BuiltinShader> builtin_shader_bound;
	/** Uniform color: Kept here to update the wide-line shader just before #immBegin. */
	float uniform_color[4] = {};

public:
	Immediate() {};
	virtual ~Immediate() {};

	virtual unsigned char *begin() = 0;
	virtual void end() = 0;
};

}  // namespace rose::gpu

void immActivate();
void immDeactivate();
