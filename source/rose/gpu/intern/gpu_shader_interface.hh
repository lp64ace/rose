#pragma once

#include <cstring> /* required for STREQ later on. */

#include "LIB_hash.h"
#include "LIB_utildefines.h"

#include "GPU_shader.h"
#include "GPU_vertex_format.h" /* GPU_VERT_ATTR_MAX_LEN */

#include "gpu_shader_create_info.hh"

namespace rose::gpu {

struct ShaderInput {
	uint32_t name_offset;
	uint32_t name_hash;
	/**
	 * Vulkan backend use location to encode the descriptor set binding. This binding is different
	 * than the binding stored in the binding attribute. In Vulkan the binding inside a descriptor
	 * set must be unique. In future the location will also be used to select the right descriptor
	 * set.
	 */
	int32_t location;
	/** Defined at interface creation or in shader. Only for Samplers, UBOs and Vertex Attributes. */
	int32_t binding;
};

/**
 * Implementation of Shader interface.
 * Base class which is then specialized for each implementation (GL, VK, ...).
 */
class ShaderInterface {
	friend shader::ShaderCreateInfo;
	/* TODO(fclem): should be protected. */
public:
	/** Flat array. In this order: Attributes, Ubos, Uniforms. */
	ShaderInput *inputs_ = nullptr;
	/** Buffer containing all inputs names separated by '\0'. */
	char *name_buffer_ = nullptr;
	/** Input counts inside input array. */
	uint attr_len_ = 0;
	uint ubo_len_ = 0;
	uint uniform_len_ = 0;
	uint ssbo_len_ = 0;
	/** Enabled bind-points that needs to be fed with data. */
	uint16_t enabled_attr_mask_ = 0;
	uint16_t enabled_ubo_mask_ = 0;
	uint8_t enabled_ima_mask_ = 0;
	uint64_t enabled_tex_mask_ = 0;
	uint16_t enabled_ssbo_mask_ = 0;
	/** Location of builtin uniforms. Fast access, no lookup needed. */
	int32_t builtins_[GPU_NUM_UNIFORMS] = {};

	/**
	 * Currently only used for `GPU_shader_get_attribute_info`.
	 * This utility is useful for automatic creation of `GPUVertFormat` in Python.
	 * Use `ShaderInput::location` to identify the `Type`.
	 */
	uint8_t attr_types_[GPU_VERT_ATTR_MAX_LEN] = {};

public:
	ShaderInterface();
	virtual ~ShaderInterface();

	void debug_print() const;

	inline const ShaderInput *attr_get(const char *name) const {
		return input_lookup(inputs_, attr_len_, name);
	}
	inline const ShaderInput *attr_get(const int binding) const {
		return input_lookup(inputs_, attr_len_, binding);
	}

	inline const ShaderInput *ubo_get(const char *name) const {
		return input_lookup(inputs_ + attr_len_, ubo_len_, name);
	}
	inline const ShaderInput *ubo_get(const int binding) const {
		return input_lookup(inputs_ + attr_len_, ubo_len_, binding);
	}

	inline const ShaderInput *uniform_get(const char *name) const {
		return input_lookup(inputs_ + attr_len_ + ubo_len_, uniform_len_, name);
	}

	inline const ShaderInput *texture_get(const int binding) const {
		return input_lookup(inputs_ + attr_len_ + ubo_len_, uniform_len_, binding);
	}

	inline const ShaderInput *ssbo_get(const char *name) const {
		return input_lookup(inputs_ + attr_len_ + ubo_len_ + uniform_len_, ssbo_len_, name);
	}
	inline const ShaderInput *ssbo_get(const int binding) const {
		return input_lookup(inputs_ + attr_len_ + ubo_len_ + uniform_len_, ssbo_len_, binding);
	}

	inline const char *input_name_get(const ShaderInput *input) const {
		return name_buffer_ + input->name_offset;
	}

	/* Returns uniform location. */
	inline int32_t uniform_builtin(const UniformBuiltin builtin) const {
		ROSE_assert(0 <= builtin && builtin < ARRAY_SIZE(builtins_));
		return builtins_[builtin];
	}

protected:
	static inline const char *builtin_uniform_name(UniformBuiltin u);

	inline uint32_t set_input_name(ShaderInput *input, char *name, uint32_t name_len) const;
	inline void copy_input_name(ShaderInput *input, const StringRefNull &name, char *name_buffer,
								uint32_t &name_buffer_offset) const;

	/**
	 * Finalize interface construction by sorting the #ShaderInputs for faster lookups.
	 */
	void sort_inputs();

private:
	inline const ShaderInput *input_lookup(const ShaderInput *const inputs, uint inputs_len, const char *name) const;

	inline const ShaderInput *input_lookup(const ShaderInput *const inputs, uint inputs_len, int binding) const;
};

inline const char *ShaderInterface::builtin_uniform_name(UniformBuiltin u) {
	switch (u) {
		case GPU_UNIFORM_MODEL:
			return "ModelMatrix";
		case GPU_UNIFORM_VIEW:
			return "ViewMatrix";
		case GPU_UNIFORM_MODELVIEW:
			return "ModelViewMatrix";
		case GPU_UNIFORM_PROJECTION:
			return "ProjectionMatrix";
		case GPU_UNIFORM_VIEWPROJECTION:
			return "ViewProjectionMatrix";
		case GPU_UNIFORM_MVP:
			return "ModelViewProjectionMatrix";

		case GPU_UNIFORM_MODEL_INV:
			return "ModelMatrixInverse";
		case GPU_UNIFORM_VIEW_INV:
			return "ViewMatrixInverse";
		case GPU_UNIFORM_MODELVIEW_INV:
			return "ModelViewMatrixInverse";
		case GPU_UNIFORM_PROJECTION_INV:
			return "ProjectionMatrixInverse";
		case GPU_UNIFORM_VIEWPROJECTION_INV:
			return "ViewProjectionMatrixInverse";

		case GPU_UNIFORM_NORMAL:
			return "NormalMatrix";
		case GPU_UNIFORM_ORCO:
			return "OrcoTexCoFactors";
		case GPU_UNIFORM_CLIPPLANES:
			return "WorldClipPlanes";

		case GPU_UNIFORM_COLOR:
			return "color";
		case GPU_UNIFORM_BASE_INSTANCE:
			return "gpu_BaseInstance";
		case GPU_UNIFORM_RESOURCE_CHUNK:
			return "drw_resourceChunk";
		case GPU_UNIFORM_RESOURCE_ID:
			return "drw_ResourceID";
		case GPU_UNIFORM_SRGB_TRANSFORM:
			return "srgbTarget";

		default:
			return nullptr;
	}
}

/* Returns string length including '\0' terminator. */
inline uint32_t ShaderInterface::set_input_name(ShaderInput *input, char *name, uint32_t name_len) const {
	/* remove "[0]" from array name */
	if (name[name_len - 1] == ']') {
		for (; name_len > 1; name_len--) {
			if (name[name_len] == '[') {
				name[name_len] = '\0';
				break;
			}
		}
	}

	input->name_offset = (uint32_t)(name - name_buffer_);
	input->name_hash = LIB_hash_string(name);
	return name_len + 1; /* include NULL terminator */
}

inline void ShaderInterface::copy_input_name(ShaderInput *input, const StringRefNull &name, char *name_buffer,
											 uint32_t &name_buffer_offset) const {
	uint32_t name_len = name.size();
	/* Copy include NULL terminator. */
	memcpy(name_buffer + name_buffer_offset, name.c_str(), name_len + 1);
	name_buffer_offset += set_input_name(input, name_buffer + name_buffer_offset, name_len);
}

inline const ShaderInput *ShaderInterface::input_lookup(const ShaderInput *const inputs, const uint inputs_len,
														const char *name) const {
	const uint name_hash = LIB_hash_string(name);
	/* Simple linear search for now. */
	for (int i = inputs_len - 1; i >= 0; i--) {
		if (inputs[i].name_hash == name_hash) {
			if ((i > 0) && inputs[i - 1].name_hash == name_hash) {
				/* Hash collision resolve. */
				for (; i >= 0 && inputs[i].name_hash == name_hash; i--) {
					if (STREQ(name, name_buffer_ + inputs[i].name_offset)) {
						return inputs + i; /* not found */
					}
				}
				return nullptr; /* not found */
			}

			/* This is a bit dangerous since we could have a hash collision.
			 * where the asked uniform that does not exist has the same hash
			 * as a real uniform. */
			ROSE_assert(STREQ(name, name_buffer_ + inputs[i].name_offset));
			return inputs + i;
		}
	}
	return nullptr; /* not found */
}

inline const ShaderInput *ShaderInterface::input_lookup(const ShaderInput *const inputs, const uint inputs_len,
														const int binding) const {
	/* Simple linear search for now. */
	for (int i = inputs_len - 1; i >= 0; i--) {
		if (inputs[i].binding == binding) {
			return inputs + i;
		}
	}
	return nullptr; /* not found */
}

}  // namespace rose::gpu
