#include "LIB_map.hh"
#include "LIB_set.hh"
#include "LIB_string_ref.hh"

#include "GPU_context.h"
#include "GPU_info.h"
#include "GPU_platform.h"
#include "GPU_shader.h"
#include "GPU_texture.h"

#include "gpu_shader_create_info.hh"
#include "gpu_shader_create_info_private.hh"
#include "gpu_shader_dependency_private.h"
#include "gpu_shader_interface.hh"
#include "gpu_shader_private.hh"

#undef GPU_SHADER_INTERFACE_INFO
#undef GPU_SHADER_CREATE_INFO

namespace rose::gpu::shader {

using CreateInfoDictionnary = Map<StringRef, ShaderCreateInfo *>;
using InterfaceDictionnary = Map<StringRef, StageInterfaceInfo *>;

static CreateInfoDictionnary *g_create_infos = nullptr;
static InterfaceDictionnary *g_interfaces = nullptr;

/* -------------------------------------------------------------------- */
/** \name Check Backend Support
 *
 * \{ */

static bool is_vulkan_compatible_interface(const StageInterfaceInfo &iface) {
	if (iface.instance_name.is_empty()) {
		return true;
	}

	bool use_flat = false;
	bool use_smooth = false;
	bool use_noperspective = false;
	for (const StageInterfaceInfo::InOut &attr : iface.inouts) {
		switch (attr.interp) {
			case Interpolation::FLAT:
				use_flat = true;
				break;
			case Interpolation::SMOOTH:
				use_smooth = true;
				break;
			case Interpolation::NO_PERSPECTIVE:
				use_noperspective = true;
				break;
		}
	}
	int num_used_interpolation_types = (use_flat ? 1 : 0) + (use_smooth ? 1 : 0) + (use_noperspective ? 1 : 0);

#if 0
  if (num_used_interpolation_types > 1) {
    std::cout << "'" << iface.name << "' uses multiple interpolation types\n";
  }
#endif

	return num_used_interpolation_types <= 1;
}

bool ShaderCreateInfo::is_vulkan_compatible() const {
	/* Vulkan doesn't support setting an interpolation mode per attribute in a struct. */
	for (const StageInterfaceInfo *iface : vertex_out_interfaces_) {
		if (!is_vulkan_compatible_interface(*iface)) {
			return false;
		}
	}
	for (const StageInterfaceInfo *iface : geometry_out_interfaces_) {
		if (!is_vulkan_compatible_interface(*iface)) {
			return false;
		}
	}

	return true;
}

/* \} */

void ShaderCreateInfo::finalize() {
	if (finalized_) {
		return;
	}
	finalized_ = true;

	Set<StringRefNull> deps_merged;

	validate_vertex_attributes();

	for (auto &info_name : additional_infos_) {

		/* Fetch create info. */
		const ShaderCreateInfo &info = *reinterpret_cast<const ShaderCreateInfo *>(gpu_shader_create_info_get(info_name.c_str()));

		/* Recursive. */
		const_cast<ShaderCreateInfo &>(info).finalize();

		interface_names_size_ += info.interface_names_size_;

		vertex_inputs_.extend_non_duplicates(info.vertex_inputs_);
		fragment_outputs_.extend_non_duplicates(info.fragment_outputs_);
		vertex_out_interfaces_.extend_non_duplicates(info.vertex_out_interfaces_);
		geometry_out_interfaces_.extend_non_duplicates(info.geometry_out_interfaces_);
		subpass_inputs_.extend_non_duplicates(info.subpass_inputs_);

		validate_vertex_attributes(&info);

		/* Insert with duplicate check. */
		push_constants_.extend_non_duplicates(info.push_constants_);
		defines_.extend_non_duplicates(info.defines_);
		batch_resources_.extend_non_duplicates(info.batch_resources_);
		pass_resources_.extend_non_duplicates(info.pass_resources_);
		typedef_sources_.extend_non_duplicates(info.typedef_sources_);

		if (info.early_fragment_test_) {
			early_fragment_test_ = true;
		}
		/* Modify depth write if has been changed from default. `UNCHANGED` implies gl_FragDepth is not used at all. */
		if (info.depth_write_ != DepthWrite::UNCHANGED) {
			depth_write_ = info.depth_write_;
		}

		/* Inherit builtin bits from additional info. */
		builtins_ |= info.builtins_;

		validate_merge(info);

		auto assert_no_overlap = [&](const bool test, const StringRefNull error) {
			if (!test) {
				std::cout << name_ << ": Validation failed while merging " << info.name_ << " : ";
				std::cout << error << std::endl;
				ROSE_assert_unreachable();
			}
		};

		if (!deps_merged.add(info.name_)) {
			assert_no_overlap(false, "additional info already merged via another info");
		}

		if (info.compute_layout_.local_size_x != -1) {
			assert_no_overlap(compute_layout_.local_size_x == -1, "Compute layout already defined");
			compute_layout_ = info.compute_layout_;
		}

		if (!info.vertex_source_.is_empty()) {
			assert_no_overlap(vertex_source_.is_empty(), "Vertex source already existing");
			vertex_source_ = info.vertex_source_;
		}
		if (!info.geometry_source_.is_empty()) {
			assert_no_overlap(geometry_source_.is_empty(), "Geometry source already existing");
			geometry_source_ = info.geometry_source_;
			geometry_layout_ = info.geometry_layout_;
		}
		if (!info.fragment_source_.is_empty()) {
			assert_no_overlap(fragment_source_.is_empty(), "Fragment source already existing");
			fragment_source_ = info.fragment_source_;
		}
		if (!info.compute_source_.is_empty()) {
			assert_no_overlap(compute_source_.is_empty(), "Compute source already existing");
			compute_source_ = info.compute_source_;
		}
	}

	if (!geometry_source_.is_empty() && bool(builtins_ & BuiltinBits::LAYER)) {
		std::cout << name_ << ": Validation failed. BuiltinBits::LAYER shouldn't be used with geometry shaders." << std::endl;
		ROSE_assert_unreachable();
	}

	if (auto_resource_location_) {
		int images = 0, samplers = 0, ubos = 0, ssbos = 0;

		auto set_resource_slot = [&](Resource &res) {
			switch (res.bind_type) {
				case Resource::BindType::UNIFORM_BUFFER:
					res.slot = ubos++;
					break;
				case Resource::BindType::STORAGE_BUFFER:
					res.slot = ssbos++;
					break;
				case Resource::BindType::SAMPLER:
					res.slot = samplers++;
					break;
				case Resource::BindType::IMAGE:
					res.slot = images++;
					break;
			}
		};

		for (auto &res : batch_resources_) {
			set_resource_slot(res);
		}
		for (auto &res : pass_resources_) {
			set_resource_slot(res);
		}
	}
}

std::string ShaderCreateInfo::check_error() const {
	std::string error;

	/* At least a vertex shader and a fragment shader are required, or only a compute shader. */
	if (this->compute_source_.is_empty()) {
		if (this->vertex_source_.is_empty()) {
			error += "Missing vertex shader in " + this->name_ + ".\n";
		}
		if (this->fragment_source_.is_empty()) {
			error += "Missing fragment shader in " + this->name_ + ".\n";
		}
	}
	else {
		if (!this->vertex_source_.is_empty()) {
			error += "Compute shader has vertex_source_ shader attached in " + this->name_ + ".\n";
		}
		if (!this->geometry_source_.is_empty()) {
			error += "Compute shader has geometry_source_ shader attached in " + this->name_ + ".\n";
		}
		if (!this->fragment_source_.is_empty()) {
			error += "Compute shader has fragment_source_ shader attached in " + this->name_ + ".\n";
		}
	}

	if (!this->geometry_source_.is_empty()) {
		if (bool(this->builtins_ & BuiltinBits::BARYCENTRIC_COORD)) {
			error += "Shader " + this->name_ +
					 " has geometry stage and uses barycentric coordinates. This is not allowed as fallback injects a geometry "
					 "stage.\n";
		}
		if (bool(this->builtins_ & BuiltinBits::VIEWPORT_INDEX)) {
			error += "Shader " + this->name_ + " has geometry stage and uses multi-viewport. This is not allowed as fallback injects a geometry stage.\n";
		}
		if (bool(this->builtins_ & BuiltinBits::LAYER)) {
			error += "Shader " + this->name_ + " has geometry stage and uses layer output. This is not allowed as fallback injects a geometry stage.\n";
		}
	}

#ifndef NDEBUG
	if (bool(this->builtins_ & (BuiltinBits::BARYCENTRIC_COORD | BuiltinBits::VIEWPORT_INDEX | BuiltinBits::LAYER))) {
		for (const StageInterfaceInfo *interface : this->vertex_out_interfaces_) {
			if (interface->instance_name.is_empty()) {
				error += "Shader " + this->name_ + " uses interface " + interface->name + " that doesn't contain an instance name, but is required for the fallback geometry shader.\n";
			}
		}
	}

	if (!this->is_vulkan_compatible()) {
		error += this->name_ +
				 " contains a stage interface using an instance name and mixed interpolation modes. This is not compatible "
				 "with Vulkan and need to be adjusted.\n";
	}
#endif

	return error;
}

void ShaderCreateInfo::validate_merge(const ShaderCreateInfo &other_info) {
	if (!auto_resource_location_) {
		/* Check same bind-points usage in OGL. */
		Set<int> images, samplers, ubos, ssbos;

		auto register_resource = [&](const Resource &res) -> bool {
			switch (res.bind_type) {
				case Resource::BindType::UNIFORM_BUFFER:
					return images.add(res.slot);
				case Resource::BindType::STORAGE_BUFFER:
					return samplers.add(res.slot);
				case Resource::BindType::SAMPLER:
					return ubos.add(res.slot);
				case Resource::BindType::IMAGE:
					return ssbos.add(res.slot);
				default:
					return false;
			}
		};

		auto print_error_msg = [&](const Resource &res, Vector<Resource> &resources) {
			auto print_resource_name = [&](const Resource &res) {
				switch (res.bind_type) {
					case Resource::BindType::UNIFORM_BUFFER:
						std::cout << "Uniform Buffer " << res.uniformbuf.name;
						break;
					case Resource::BindType::STORAGE_BUFFER:
						std::cout << "Storage Buffer " << res.storagebuf.name;
						break;
					case Resource::BindType::SAMPLER:
						std::cout << "Sampler " << res.sampler.name;
						break;
					case Resource::BindType::IMAGE:
						std::cout << "Image " << res.image.name;
						break;
					default:
						std::cout << "Unknown Type";
						break;
				}
			};

			for (const Resource &_res : resources) {
				if (&res != &_res && res.bind_type == _res.bind_type && res.slot == _res.slot) {
					std::cout << name_ << ": Validation failed : Overlapping ";
					print_resource_name(res);
					std::cout << " and ";
					print_resource_name(_res);
					std::cout << " at (" << res.slot << ") while merging " << other_info.name_ << std::endl;
				}
			}
		};

		for (auto &res : batch_resources_) {
			if (register_resource(res) == false) {
				print_error_msg(res, batch_resources_);
				print_error_msg(res, pass_resources_);
			}
		}

		for (auto &res : pass_resources_) {
			if (register_resource(res) == false) {
				print_error_msg(res, batch_resources_);
				print_error_msg(res, pass_resources_);
			}
		}
	}
}

void ShaderCreateInfo::validate_vertex_attributes(const ShaderCreateInfo *other_info) {
	uint32_t attr_bits = 0;
	for (auto &attr : vertex_inputs_) {
		if (attr.index >= 16 || attr.index < 0) {
			std::cout << name_ << ": \"" << attr.name << "\" : Type::MAT3 unsupported as vertex attribute." << std::endl;
			ROSE_assert_unreachable();
		}
		if (attr.index >= 16 || attr.index < 0) {
			std::cout << name_ << ": Invalid index for attribute \"" << attr.name << "\"" << std::endl;
			ROSE_assert_unreachable();
		}
		uint32_t attr_new = 0;
		if (attr.type == Type::MAT4) {
			for (int i = 0; i < 4; i++) {
				attr_new |= 1 << (attr.index + i);
			}
		}
		else {
			attr_new |= 1 << attr.index;
		}

		if ((attr_bits & attr_new) != 0) {
			std::cout << name_ << ": Attribute \"" << attr.name << "\" overlap one or more index from another attribute. Note that mat4 takes up 4 indices.";
			if (other_info) {
				std::cout << " While merging " << other_info->name_ << std::endl;
			}
			std::cout << std::endl;
			ROSE_assert_unreachable();
		}
		attr_bits |= attr_new;
	}
}

}  // namespace rose::gpu::shader

using namespace rose::gpu::shader;

void gpu_shader_create_info_init() {
	g_create_infos = MEM_new<CreateInfoDictionnary>("rose::gpu::GlobalCreateInfoDictionary");
	g_interfaces = MEM_new<InterfaceDictionnary>("rose::gpu::GlobalInterfaceDictionary");

#define GPU_SHADER_INTERFACE_INFO(_interface, _inst_name)                                                              \
	StageInterfaceInfo *ptr_##_interface = MEM_new<StageInterfaceInfo>("StageInterfaceInfo", #_interface, _inst_name); \
	StageInterfaceInfo &_interface = *ptr_##_interface;                                                                \
	g_interfaces->add_new(#_interface, ptr_##_interface);                                                              \
	_interface

#define GPU_SHADER_CREATE_INFO(_info)                                                      \
	ShaderCreateInfo *ptr_##_info = MEM_new<ShaderCreateInfo>("ShaderCreateInfo", #_info); \
	ShaderCreateInfo &_info = *ptr_##_info;                                                \
	g_create_infos->add_new(#_info, ptr_##_info);                                          \
	_info

/* Declare, register and construct the infos. */
#include "gpu_shader_create_info_list.hh"

/* Baked shader data appended to create infos. */
/* TODO(jbakker): should call a function with a callback. so we could switch implementations.
 * We cannot compile bf_gpu twice. */
#ifdef GPU_RUNTIME
#	include "gpu_shader_baked.hh"
#endif

	for (ShaderCreateInfo *info : g_create_infos->values()) {
		info->builtins_ |= gpu_shader_dependency_get_builtins(info->vertex_source_);
		info->builtins_ |= gpu_shader_dependency_get_builtins(info->fragment_source_);
		info->builtins_ |= gpu_shader_dependency_get_builtins(info->geometry_source_);
		info->builtins_ |= gpu_shader_dependency_get_builtins(info->compute_source_);

#ifndef NDEBUG
		/* Automatically amend the create info for ease of use of the debug feature. */
		if ((info->builtins_ & BuiltinBits::USE_DEBUG_DRAW) == BuiltinBits::USE_DEBUG_DRAW) {
			info->additional_info("draw_debug_draw");
		}
		if ((info->builtins_ & BuiltinBits::USE_DEBUG_PRINT) == BuiltinBits::USE_DEBUG_PRINT) {
			info->additional_info("draw_debug_print");
		}
#endif
	}

#ifndef NDEBUG
	/** TEST */
	gpu_shader_create_info_compile(nullptr);
#endif
}

void gpu_shader_create_info_exit() {
	for (auto *value : g_create_infos->values()) {
		MEM_delete(value);
	}
	MEM_delete(g_create_infos);

	for (auto *value : g_interfaces->values()) {
		MEM_delete(value);
	}
	MEM_delete(g_interfaces);
}

bool gpu_shader_create_info_compile(const char *name_starts_with_filter) {
	using namespace rose::gpu;
	int success = 0;
	int skipped_filter = 0;
	int skipped = 0;
	int total = 0;

	for (ShaderCreateInfo *info : g_create_infos->values()) {
		info->finalize();
		if (info->do_static_compilation_) {
			if (name_starts_with_filter && !info->name_.startswith(rose::StringRefNull(name_starts_with_filter))) {
				skipped_filter++;
				continue;
			}
			if ((GPU_get_info_i(GPU_INFO_GEOMETRY_SHADER_SUPPORT) == false && info->compute_source_ != nullptr) || (GPU_get_info_i(GPU_INFO_GEOMETRY_SHADER_SUPPORT) == false && info->geometry_source_ != nullptr) || (GPU_get_info_i(GPU_INFO_GEOMETRY_SHADER_SUPPORT) == false && info->has_resource_image())) {
				skipped++;
				continue;
			}

			total++;
			GPUShader *shader = GPU_shader_create_from_info(reinterpret_cast<const GPUShaderCreateInfo *>(info));

			if (shader == nullptr) {
				std::cerr << "Compilation " << info->name_.c_str() << " Failed\n";
			}
			else {
				success++;

#ifndef NDEBUG
				const ShaderInterface *interface = unwrap(shader)->interface;

				rose::Vector<ShaderCreateInfo::Resource> all_resources;
				all_resources.extend(info->pass_resources_);
				all_resources.extend(info->batch_resources_);

				for (ShaderCreateInfo::Resource &res : all_resources) {
					rose::StringRefNull name = "";
					const ShaderInput *input = nullptr;

					switch (res.bind_type) {
						case ShaderCreateInfo::Resource::BindType::UNIFORM_BUFFER:
							input = interface->ubo_get(res.slot);
							name = res.uniformbuf.name;
							break;
						case ShaderCreateInfo::Resource::BindType::STORAGE_BUFFER:
							input = interface->ssbo_get(res.slot);
							name = res.storagebuf.name;
							break;
						case ShaderCreateInfo::Resource::BindType::SAMPLER:
							input = interface->texture_get(res.slot);
							name = res.sampler.name;
							break;
						case ShaderCreateInfo::Resource::BindType::IMAGE:
							input = interface->texture_get(res.slot);
							name = res.image.name;
							break;
					}

					if (input == nullptr) {
						std::cerr << "Error: " << info->name_;
						std::cerr << ": Resource « " << name << " » not found in the shader interface\n";
					}
					else if (input->location == -1) {
						std::cerr << "Warning: " << info->name_;
						std::cerr << ": Resource « " << name << " » is optimized out\n";
					}
				}
#endif
			}
			GPU_shader_free(shader);
		}
	}

	printf("[GPU] Shader test compilation result: %d/%d, %d%% passed", success, total, success * 100 / total);
	if (skipped_filter > 0) {
		printf(" (skipped %d when filtering)", skipped_filter);
	}
	if (skipped > 0) {
		printf(" (skipped %d for compatibility reasons)", skipped);
	}
	printf("\n");
	return success == total;
}

const GPUShaderCreateInfo *gpu_shader_create_info_get(const char *info_name) {
	if (g_create_infos->contains(info_name) == false) {
		printf("Error: Cannot find shader create info named \"%s\"\n", info_name);
		return nullptr;
	}
	ShaderCreateInfo *info = g_create_infos->lookup(info_name);
	return reinterpret_cast<const GPUShaderCreateInfo *>(info);
}
