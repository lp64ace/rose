#include <iomanip>

#include "LIB_string.h"
#include "LIB_vector.hh"

#include "GPU_info.h"
#include "GPU_platform.h"

#include "gl_backend.hh"
#include "gl_context.hh"
#include "gl_vertex_buffer.hh"

#include "gl_shader.hh"
#include "gl_shader_interface.hh"

#include <sstream>

using namespace rose;
using namespace rose::gpu;
using namespace rose::gpu::shader;

extern "C" char datatoc_glsl_shader_defines_glsl[];

/* -------------------------------------------------------------------- */
/** \name Creation / Destruction
 * \{ */

GLShader::GLShader(const char *name) : Shader(name) {
	shader_program_ = glCreateProgram();
}

GLShader::~GLShader() {
	/* Invalid handles are silently ignored. */
	glDeleteShader(vert_shader_);
	glDeleteShader(geom_shader_);
	glDeleteShader(frag_shader_);
	glDeleteShader(compute_shader_);
	glDeleteProgram(shader_program_);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Info
 * \{ */

static const char *to_string(const Interpolation &interp) {
	switch (interp) {
		case Interpolation::SMOOTH:
			return "smooth";
		case Interpolation::FLAT:
			return "flat";
		case Interpolation::NO_PERSPECTIVE:
			return "noperspective";
		default:
			return "unknown";
	}
}

static const char *to_string(const Type &type) {
	switch (type) {
		case Type::FLOAT:
			return "float";
		case Type::VEC2:
			return "vec2";
		case Type::VEC3:
			return "vec3";
		case Type::VEC4:
			return "vec4";
		case Type::MAT3:
			return "mat3";
		case Type::MAT4:
			return "mat4";
		case Type::UINT:
			return "uint";
		case Type::UVEC2:
			return "uvec2";
		case Type::UVEC3:
			return "uvec3";
		case Type::UVEC4:
			return "uvec4";
		case Type::INT:
			return "int";
		case Type::IVEC2:
			return "ivec2";
		case Type::IVEC3:
			return "ivec3";
		case Type::IVEC4:
			return "ivec4";
		case Type::BOOL:
			return "bool";
		/* Alias special types. */
		case Type::UCHAR:
		case Type::USHORT:
			return "uint";
		case Type::UCHAR2:
		case Type::USHORT2:
			return "uvec2";
		case Type::UCHAR3:
		case Type::USHORT3:
			return "uvec3";
		case Type::UCHAR4:
		case Type::USHORT4:
			return "uvec4";
		case Type::CHAR:
		case Type::SHORT:
			return "int";
		case Type::CHAR2:
		case Type::SHORT2:
			return "ivec2";
		case Type::CHAR3:
		case Type::SHORT3:
			return "ivec3";
		case Type::CHAR4:
		case Type::SHORT4:
			return "ivec4";
		case Type::VEC3_101010I2:
			return "vec3";
	}
	ROSE_assert_unreachable();
	return "unknown";
}

static const int to_component_count(const Type &type) {
	switch (type) {
		case Type::FLOAT:
		case Type::UINT:
		case Type::INT:
		case Type::BOOL:
			return 1;
		case Type::VEC2:
		case Type::UVEC2:
		case Type::IVEC2:
			return 2;
		case Type::VEC3:
		case Type::UVEC3:
		case Type::IVEC3:
			return 3;
		case Type::VEC4:
		case Type::UVEC4:
		case Type::IVEC4:
			return 4;
		case Type::MAT3:
			return 9;
		case Type::MAT4:
			return 16;
		/* Alias special types. */
		case Type::UCHAR:
		case Type::USHORT:
			return 1;
		case Type::UCHAR2:
		case Type::USHORT2:
			return 2;
		case Type::UCHAR3:
		case Type::USHORT3:
			return 3;
		case Type::UCHAR4:
		case Type::USHORT4:
			return 4;
		case Type::CHAR:
		case Type::SHORT:
			return 1;
		case Type::CHAR2:
		case Type::SHORT2:
			return 2;
		case Type::CHAR3:
		case Type::SHORT3:
			return 3;
		case Type::CHAR4:
		case Type::SHORT4:
			return 4;
		case Type::VEC3_101010I2:
			return 3;
	}
	ROSE_assert_unreachable();
	return -1;
}

static const Type to_component_type(const Type &type) {
	switch (type) {
		case Type::FLOAT:
		case Type::VEC2:
		case Type::VEC3:
		case Type::VEC4:
		case Type::MAT3:
		case Type::MAT4:
			return Type::FLOAT;
		case Type::UINT:
		case Type::UVEC2:
		case Type::UVEC3:
		case Type::UVEC4:
			return Type::UINT;
		case Type::INT:
		case Type::IVEC2:
		case Type::IVEC3:
		case Type::IVEC4:
		case Type::BOOL:
			return Type::INT;
		/* Alias special types. */
		case Type::UCHAR:
		case Type::UCHAR2:
		case Type::UCHAR3:
		case Type::UCHAR4:
		case Type::USHORT:
		case Type::USHORT2:
		case Type::USHORT3:
		case Type::USHORT4:
			return Type::UINT;
		case Type::CHAR:
		case Type::CHAR2:
		case Type::CHAR3:
		case Type::CHAR4:
		case Type::SHORT:
		case Type::SHORT2:
		case Type::SHORT3:
		case Type::SHORT4:
			return Type::INT;
		case Type::VEC3_101010I2:
			return Type::FLOAT;
	}
	ROSE_assert_unreachable();
	return Type::FLOAT;
}

static const char *to_string(const TextureFormat &type) {
	switch (type) {
		case GPU_RGBA8UI:
			return "rgba8ui";
		case GPU_RGBA8I:
			return "rgba8i";
		case GPU_RGBA8:
			return "rgba8";
		case GPU_RGBA32UI:
			return "rgba32ui";
		case GPU_RGBA32I:
			return "rgba32i";
		case GPU_RGBA32F:
			return "rgba32f";
		case GPU_RGBA16UI:
			return "rgba16ui";
		case GPU_RGBA16I:
			return "rgba16i";
		case GPU_RGBA16F:
			return "rgba16f";
		case GPU_RGBA16:
			return "rgba16";
		case GPU_RG8UI:
			return "rg8ui";
		case GPU_RG8I:
			return "rg8i";
		case GPU_RG8:
			return "rg8";
		case GPU_RG32UI:
			return "rg32ui";
		case GPU_RG32I:
			return "rg32i";
		case GPU_RG32F:
			return "rg32f";
		case GPU_RG16UI:
			return "rg16ui";
		case GPU_RG16I:
			return "rg16i";
		case GPU_RG16F:
			return "rg16f";
		case GPU_RG16:
			return "rg16";
		case GPU_R8UI:
			return "r8ui";
		case GPU_R8I:
			return "r8i";
		case GPU_R8:
			return "r8";
		case GPU_R32UI:
			return "r32ui";
		case GPU_R32I:
			return "r32i";
		case GPU_R32F:
			return "r32f";
		case GPU_R16UI:
			return "r16ui";
		case GPU_R16I:
			return "r16i";
		case GPU_R16F:
			return "r16f";
		case GPU_R16:
			return "r16";
		case GPU_R11F_G11F_B10F:
			return "r11f_g11f_b10f";
		case GPU_RGB10_A2:
			return "rgb10_a2";
		default:
			return "unknown";
	}
}

static const char *to_string(const PrimitiveIn &layout) {
	switch (layout) {
		case PrimitiveIn::POINTS:
			return "points";
		case PrimitiveIn::LINES:
			return "lines";
		case PrimitiveIn::LINES_ADJACENCY:
			return "lines_adjacency";
		case PrimitiveIn::TRIANGLES:
			return "triangles";
		case PrimitiveIn::TRIANGLES_ADJACENCY:
			return "triangles_adjacency";
		default:
			return "unknown";
	}
}

static const char *to_string(const PrimitiveOut &layout) {
	switch (layout) {
		case PrimitiveOut::POINTS:
			return "points";
		case PrimitiveOut::LINE_STRIP:
			return "line_strip";
		case PrimitiveOut::TRIANGLE_STRIP:
			return "triangle_strip";
		default:
			return "unknown";
	}
}

static const char *to_string(const DepthWrite &value) {
	switch (value) {
		case DepthWrite::ANY:
			return "depth_any";
		case DepthWrite::GREATER:
			return "depth_greater";
		case DepthWrite::LESS:
			return "depth_less";
		default:
			return "depth_unchanged";
	}
}

static void print_image_type(std::ostream &os, const ImageType &type, const ShaderCreateInfo::Resource::BindType bind_type) {
	switch (type) {
		case ImageType::INT_BUFFER:
		case ImageType::INT_1D:
		case ImageType::INT_1D_ARRAY:
		case ImageType::INT_2D:
		case ImageType::INT_2D_ARRAY:
		case ImageType::INT_3D:
		case ImageType::INT_CUBE:
		case ImageType::INT_CUBE_ARRAY:
			os << "i";
			break;
		case ImageType::UINT_BUFFER:
		case ImageType::UINT_1D:
		case ImageType::UINT_1D_ARRAY:
		case ImageType::UINT_2D:
		case ImageType::UINT_2D_ARRAY:
		case ImageType::UINT_3D:
		case ImageType::UINT_CUBE:
		case ImageType::UINT_CUBE_ARRAY:
			os << "u";
			break;
		default:
			break;
	}

	if (bind_type == ShaderCreateInfo::Resource::BindType::IMAGE) {
		os << "image";
	}
	else {
		os << "sampler";
	}

	switch (type) {
		case ImageType::FLOAT_BUFFER:
		case ImageType::INT_BUFFER:
		case ImageType::UINT_BUFFER:
			os << "Buffer";
			break;
		case ImageType::FLOAT_1D:
		case ImageType::FLOAT_1D_ARRAY:
		case ImageType::INT_1D:
		case ImageType::INT_1D_ARRAY:
		case ImageType::UINT_1D:
		case ImageType::UINT_1D_ARRAY:
			os << "1D";
			break;
		case ImageType::FLOAT_2D:
		case ImageType::FLOAT_2D_ARRAY:
		case ImageType::INT_2D:
		case ImageType::INT_2D_ARRAY:
		case ImageType::UINT_2D:
		case ImageType::UINT_2D_ARRAY:
		case ImageType::SHADOW_2D:
		case ImageType::SHADOW_2D_ARRAY:
		case ImageType::DEPTH_2D:
		case ImageType::DEPTH_2D_ARRAY:
			os << "2D";
			break;
		case ImageType::FLOAT_3D:
		case ImageType::INT_3D:
		case ImageType::UINT_3D:
			os << "3D";
			break;
		case ImageType::FLOAT_CUBE:
		case ImageType::FLOAT_CUBE_ARRAY:
		case ImageType::INT_CUBE:
		case ImageType::INT_CUBE_ARRAY:
		case ImageType::UINT_CUBE:
		case ImageType::UINT_CUBE_ARRAY:
		case ImageType::SHADOW_CUBE:
		case ImageType::SHADOW_CUBE_ARRAY:
		case ImageType::DEPTH_CUBE:
		case ImageType::DEPTH_CUBE_ARRAY:
			os << "Cube";
			break;
		default:
			break;
	}

	switch (type) {
		case ImageType::FLOAT_1D_ARRAY:
		case ImageType::FLOAT_2D_ARRAY:
		case ImageType::FLOAT_CUBE_ARRAY:
		case ImageType::INT_1D_ARRAY:
		case ImageType::INT_2D_ARRAY:
		case ImageType::INT_CUBE_ARRAY:
		case ImageType::UINT_1D_ARRAY:
		case ImageType::UINT_2D_ARRAY:
		case ImageType::UINT_CUBE_ARRAY:
		case ImageType::SHADOW_2D_ARRAY:
		case ImageType::SHADOW_CUBE_ARRAY:
		case ImageType::DEPTH_2D_ARRAY:
		case ImageType::DEPTH_CUBE_ARRAY:
			os << "Array";
			break;
		default:
			break;
	}

	switch (type) {
		case ImageType::SHADOW_2D:
		case ImageType::SHADOW_2D_ARRAY:
		case ImageType::SHADOW_CUBE:
		case ImageType::SHADOW_CUBE_ARRAY:
			os << "Shadow";
			break;
		default:
			break;
	}
	os << " ";
}

static std::ostream &print_qualifier(std::ostream &os, const Qualifier &qualifiers) {
	if (bool(qualifiers & Qualifier::NO_RESTRICT) == false) {
		os << "restrict ";
	}
	if (bool(qualifiers & Qualifier::READ) == false) {
		os << "writeonly ";
	}
	if (bool(qualifiers & Qualifier::WRITE) == false) {
		os << "readonly ";
	}
	return os;
}

static void print_resource(std::ostream &os, const ShaderCreateInfo::Resource &res, bool auto_resource_location) {
	if (auto_resource_location && res.bind_type == ShaderCreateInfo::Resource::BindType::SAMPLER) {
		/* Skip explicit binding location for samplers when not needed, since drivers can usually
		 * handle more sampler declarations this way (as long as they're not actually used by the
		 * shader). See #105661. */
	}
	else if (GLContext::explicit_location_support) {
		os << "layout(binding = " << res.slot;
		if (res.bind_type == ShaderCreateInfo::Resource::BindType::IMAGE) {
			os << ", " << to_string(res.image.format);
		}
		else if (res.bind_type == ShaderCreateInfo::Resource::BindType::UNIFORM_BUFFER) {
			os << ", std140";
		}
		else if (res.bind_type == ShaderCreateInfo::Resource::BindType::STORAGE_BUFFER) {
			os << ", std430";
		}
		os << ") ";
	}
	else if (res.bind_type == ShaderCreateInfo::Resource::BindType::UNIFORM_BUFFER) {
		os << "layout(std140) ";
	}

	size_t array_offset;
	StringRef name_no_array;

	switch (res.bind_type) {
		case ShaderCreateInfo::Resource::BindType::SAMPLER:
			os << "uniform ";
			print_image_type(os, res.sampler.type, res.bind_type);
			os << res.sampler.name << ";\n";
			break;
		case ShaderCreateInfo::Resource::BindType::IMAGE:
			os << "uniform ";
			print_qualifier(os, res.image.qualifiers);
			print_image_type(os, res.image.type, res.bind_type);
			os << res.image.name << ";\n";
			break;
		case ShaderCreateInfo::Resource::BindType::UNIFORM_BUFFER:
			array_offset = res.uniformbuf.name.find_first_of("[");
			name_no_array = (array_offset == StringRefBase::not_found) ? res.uniformbuf.name : StringRef(res.uniformbuf.name.c_str(), array_offset);
			os << "uniform " << name_no_array << " { " << res.uniformbuf.type_name << " _" << res.uniformbuf.name << "; };\n";
			break;
		case ShaderCreateInfo::Resource::BindType::STORAGE_BUFFER:
			array_offset = res.storagebuf.name.find_first_of("[");
			name_no_array = (array_offset == StringRefBase::not_found) ? res.storagebuf.name : StringRef(res.storagebuf.name.c_str(), array_offset);
			print_qualifier(os, res.storagebuf.qualifiers);
			os << "buffer ";
			os << name_no_array << " { " << res.storagebuf.type_name << " _" << res.storagebuf.name << "; };\n";
			break;
	}
}

static void print_resource_alias(std::ostream &os, const ShaderCreateInfo::Resource &res) {
	size_t array_offset;
	StringRef name_no_array;

	switch (res.bind_type) {
		case ShaderCreateInfo::Resource::BindType::UNIFORM_BUFFER:
			array_offset = res.uniformbuf.name.find_first_of("[");
			name_no_array = (array_offset == StringRefBase::not_found) ? res.uniformbuf.name : StringRef(res.uniformbuf.name.c_str(), array_offset);
			os << "#define " << name_no_array << " (_" << name_no_array << ")\n";
			break;
		case ShaderCreateInfo::Resource::BindType::STORAGE_BUFFER:
			array_offset = res.storagebuf.name.find_first_of("[");
			name_no_array = (array_offset == StringRefBase::not_found) ? res.storagebuf.name : StringRef(res.storagebuf.name.c_str(), array_offset);
			os << "#define " << name_no_array << " (_" << name_no_array << ")\n";
			break;
		default:
			break;
	}
}

static void print_interface(std::ostream &os, const StringRefNull &prefix, const StageInterfaceInfo &iface, const StringRefNull &suffix = "") {
	/* TODO(@fclem): Move that to interface check. */
	// if (iface.instance_name.is_empty()) {
	//   ROSE_assert_msg(0, "Interfaces require an instance name for geometry shader.");
	//   std::cout << iface.name << ": Interfaces require an instance name for geometry shader.\n";
	//   continue;
	// }
	os << prefix << " " << iface.name << "{" << std::endl;
	for (const StageInterfaceInfo::InOut &inout : iface.inouts) {
		os << "  " << to_string(inout.interp) << " " << to_string(inout.type) << " " << inout.name << ";\n";
	}
	os << "}";
	os << (iface.instance_name.is_empty() ? "" : "\n") << iface.instance_name << suffix << ";\n";
}

std::string GLShader::resources_declare(const ShaderCreateInfo &info) const {
	std::stringstream ss;

	/* NOTE: We define macros in GLSL to trigger compilation error if the resource names
	 * are reused for local variables. This is to match other backend behavior which needs accessors
	 * macros. */

	ss << "\n/* Pass Resources. */\n";
	for (const ShaderCreateInfo::Resource &res : info.pass_resources_) {
		print_resource(ss, res, info.auto_resource_location_);
	}
	for (const ShaderCreateInfo::Resource &res : info.pass_resources_) {
		print_resource_alias(ss, res);
	}
	ss << "\n/* Batch Resources. */\n";
	for (const ShaderCreateInfo::Resource &res : info.batch_resources_) {
		print_resource(ss, res, info.auto_resource_location_);
	}
	for (const ShaderCreateInfo::Resource &res : info.batch_resources_) {
		print_resource_alias(ss, res);
	}
	ss << "\n/* Push Constants. */\n";
	for (const ShaderCreateInfo::PushConst &uniform : info.push_constants_) {
		ss << "uniform " << to_string(uniform.type) << " " << uniform.name;
		if (uniform.array_size > 0) {
			ss << "[" << uniform.array_size << "]";
		}
		ss << ";\n";
	}
#if 0 /* #95278: This is not be enough to prevent some compilers think it is recursive. */
  for (const ShaderCreateInfo::PushConst &uniform : info.push_constants_) {
    /* #95278: Double macro to avoid some compilers think it is recursive. */
    ss << "#define " << uniform.name << "_ " << uniform.name << "\n";
    ss << "#define " << uniform.name << " (" << uniform.name << "_)\n";
  }
#endif
	ss << "\n";
	return ss.str();
}

static std::string main_function_wrapper(std::string &pre_main, std::string &post_main) {
	std::stringstream ss;
	/* Prototype for the original main. */
	ss << "\n";
	ss << "void main_function_();\n";
	/* Wrapper to the main function in order to inject code processing on globals. */
	ss << "void main() {\n";
	ss << pre_main;
	ss << "  main_function_();\n";
	ss << post_main;
	ss << "}\n";
	/* Rename the original main. */
	ss << "#define main main_function_\n";
	ss << "\n";
	return ss.str();
}

std::string GLShader::vertex_interface_declare(const ShaderCreateInfo &info) const {
	std::stringstream ss;
	std::string post_main;

	ss << "\n/* Inputs. */\n";
	for (const ShaderCreateInfo::VertIn &attr : info.vertex_inputs_) {
		if (GLContext::explicit_location_support &&
			/* Fix issue with AMDGPU-PRO + workbench_prepass_mesh_vert.glsl being quantized. */
			GPU_type_matches(GPU_DEVICE_ATI, GPU_OS_ANY, GPU_DRIVER_OFFICIAL) == false) {
			ss << "layout(location = " << attr.index << ") ";
		}
		ss << "in " << to_string(attr.type) << " " << attr.name << ";\n";
	}
	ss << "\n/* Interfaces. */\n";
	for (const StageInterfaceInfo *iface : info.vertex_out_interfaces_) {
		print_interface(ss, "out", *iface);
	}
	if (!GLContext::layered_rendering_support && bool(info.builtins_ & BuiltinBits::LAYER)) {
		ss << "out int gpu_Layer;\n";
	}
	if (!GLContext::layered_rendering_support && bool(info.builtins_ & BuiltinBits::VIEWPORT_INDEX)) {
		ss << "out int gpu_ViewportIndex;\n";
	}
	if (bool(info.builtins_ & BuiltinBits::BARYCENTRIC_COORD)) {
		if (!GLContext::native_barycentric_support) {
			/* Disabled or unsupported. */
		}
		else if (has_gl_extension("GL_AMD_shader_explicit_vertex_parameter")) {
			/* Need this for stable barycentric. */
			ss << "flat out vec4 gpu_pos_flat;\n";
			ss << "out vec4 gpu_pos;\n";

			post_main += "  gpu_pos = gpu_pos_flat = gl_Position;\n";
		}
	}
	ss << "\n";

	if (post_main.empty() == false) {
		std::string pre_main;
		ss << main_function_wrapper(pre_main, post_main);
	}
	return ss.str();
}

std::string GLShader::fragment_interface_declare(const ShaderCreateInfo &info) const {
	std::stringstream ss;
	std::string pre_main, post_main;

	ss << "\n/* Interfaces. */\n";
	const Vector<StageInterfaceInfo *> &in_interfaces = info.geometry_source_.is_empty() ? info.vertex_out_interfaces_ : info.geometry_out_interfaces_;
	for (const StageInterfaceInfo *iface : in_interfaces) {
		print_interface(ss, "in", *iface);
	}
	if (!GLContext::layered_rendering_support && bool(info.builtins_ & BuiltinBits::LAYER)) {
		ss << "#define gpu_Layer gl_Layer\n";
	}
	if (!GLContext::layered_rendering_support && bool(info.builtins_ & BuiltinBits::VIEWPORT_INDEX)) {
		ss << "#define gpu_ViewportIndex gl_ViewportIndex\n";
	}
	if (bool(info.builtins_ & BuiltinBits::BARYCENTRIC_COORD)) {
		if (!GLContext::native_barycentric_support) {
			ss << "flat in vec4 gpu_pos[3];\n";
			ss << "smooth in vec3 gpu_BaryCoord;\n";
			ss << "noperspective in vec3 gpu_BaryCoordNoPersp;\n";
			ss << "#define gpu_position_at_vertex(v) gpu_pos[v]\n";
		}
		else if (has_gl_extension("GL_AMD_shader_explicit_vertex_parameter")) {
			/* NOTE(fclem): This won't work with geometry shader. Hopefully, we don't need geometry
			 * shader workaround if this extension/feature is detected. */
			ss << "\n/* Stable Barycentric Coordinates. */\n";
			ss << "flat in vec4 gpu_pos_flat;\n";
			ss << "__explicitInterpAMD in vec4 gpu_pos;\n";
			/* Globals. */
			ss << "vec3 gpu_BaryCoord;\n";
			ss << "vec3 gpu_BaryCoordNoPersp;\n";
			ss << "\n";
			ss << "vec2 stable_bary_(vec2 in_bary) {\n";
			ss << "  vec3 bary = vec3(in_bary, 1.0 - in_bary.x - in_bary.y);\n";
			ss << "  if (interpolateAtVertexAMD(gpu_pos, 0) == gpu_pos_flat) { return bary.zxy; }\n";
			ss << "  if (interpolateAtVertexAMD(gpu_pos, 2) == gpu_pos_flat) { return bary.yzx; }\n";
			ss << "  return bary.xyz;\n";
			ss << "}\n";
			ss << "\n";
			ss << "vec4 gpu_position_at_vertex(int v) {\n";
			ss << "  if (interpolateAtVertexAMD(gpu_pos, 0) == gpu_pos_flat) { v = (v + 2) % 3; }\n";
			ss << "  if (interpolateAtVertexAMD(gpu_pos, 2) == gpu_pos_flat) { v = (v + 1) % 3; }\n";
			ss << "  return interpolateAtVertexAMD(gpu_pos, v);\n";
			ss << "}\n";

			pre_main += "  gpu_BaryCoord = stable_bary_(gl_BaryCoordSmoothAMD);\n";
			pre_main += "  gpu_BaryCoordNoPersp = stable_bary_(gl_BaryCoordNoPerspAMD);\n";
		}
	}
	if (info.early_fragment_test_) {
		ss << "layout(early_fragment_tests) in;\n";
	}
	if (has_gl_extension("GL_ARB_conservative_depth")) {
		ss << "layout(" << to_string(info.depth_write_) << ") out float gl_FragDepth;\n";
	}

	ss << "\n/* Sub-pass Inputs. */\n";
	for (const ShaderCreateInfo::SubpassIn &input : info.subpass_inputs_) {
		if (GLContext::framebuffer_fetch_support) {
			/* Declare as inout but do not write to it. */
			ss << "layout(location = " << std::to_string(input.index) << ") inout " << to_string(input.type) << " " << input.name << ";\n";
		}
		else {
			std::string image_name = "gpu_subpass_img_";
			image_name += std::to_string(input.index);

			/* Declare global for input. */
			ss << to_string(input.type) << " " << input.name << ";\n";

			/* IMPORTANT: We assume that the frame-buffer will be layered or not based on the layer
			 * built-in flag. */
			bool is_layered_fb = bool(info.builtins_ & BuiltinBits::LAYER);

			/* Start with invalid value to detect failure cases. */
			ImageType image_type = ImageType::FLOAT_BUFFER;
			switch (to_component_type(input.type)) {
				case Type::FLOAT:
					image_type = is_layered_fb ? ImageType::FLOAT_2D_ARRAY : ImageType::FLOAT_2D;
					break;
				case Type::INT:
					image_type = is_layered_fb ? ImageType::INT_2D_ARRAY : ImageType::INT_2D;
					break;
				case Type::UINT:
					image_type = is_layered_fb ? ImageType::UINT_2D_ARRAY : ImageType::UINT_2D;
					break;
				default:
					break;
			}
			/* Declare image. */
			using Resource = ShaderCreateInfo::Resource;
			/* NOTE(fclem): Using the attachment index as resource index might be problematic as it might
			 * collide with other resources. */
			Resource res(Resource::BindType::SAMPLER, input.index);
			res.sampler.type = image_type;
			res.sampler.sampler = GPUSamplerState::default_sampler();
			res.sampler.name = image_name;
			print_resource(ss, res, false);

			char swizzle[] = "xyzw";
			swizzle[to_component_count(input.type)] = '\0';

			std::string texel_co = (is_layered_fb) ? "ivec3(gl_FragCoord.xy, gpu_Layer)" : "ivec2(gl_FragCoord.xy)";

			std::stringstream ss_pre;
			/* Populate the global before main using imageLoad. */
			ss_pre << "  " << input.name << " = texelFetch(" << image_name << ", " << texel_co << ", 0)." << swizzle << ";\n";

			pre_main += ss_pre.str();
		}
	}
	ss << "\n/* Outputs. */\n";
	for (const ShaderCreateInfo::FragOut &output : info.fragment_outputs_) {
		ss << "layout(location = " << output.index;
		switch (output.blend) {
			case DualBlend::SRC_0:
				ss << ", index = 0";
				break;
			case DualBlend::SRC_1:
				ss << ", index = 1";
				break;
			default:
				break;
		}
		ss << ") ";
		ss << "out " << to_string(output.type) << " " << output.name << ";\n";
	}
	ss << "\n";

	if (!pre_main.empty() || !post_main.empty()) {
		ss << main_function_wrapper(pre_main, post_main);
	}
	return ss.str();
}

std::string GLShader::geometry_layout_declare(const ShaderCreateInfo &info) const {
	int max_verts = info.geometry_layout_.max_vertices;
	int invocations = info.geometry_layout_.invocations;

	if (GLContext::geometry_shader_invocations == false && invocations != -1) {
		max_verts *= invocations;
		invocations = -1;
	}

	std::stringstream ss;
	ss << "\n/* Geometry Layout. */\n";
	ss << "layout(" << to_string(info.geometry_layout_.primitive_in);
	if (invocations != -1) {
		ss << ", invocations = " << invocations;
	}
	ss << ") in;\n";

	ss << "layout(" << to_string(info.geometry_layout_.primitive_out) << ", max_vertices = " << max_verts << ") out;\n";
	ss << "\n";
	return ss.str();
}

static StageInterfaceInfo *find_interface_by_name(const Vector<StageInterfaceInfo *> &ifaces, const StringRefNull &name) {
	for (auto *iface : ifaces) {
		if (iface->instance_name == name) {
			return iface;
		}
	}
	return nullptr;
}

std::string GLShader::geometry_interface_declare(const ShaderCreateInfo &info) const {
	std::stringstream ss;

	ss << "\n/* Interfaces. */\n";
	for (const StageInterfaceInfo *iface : info.vertex_out_interfaces_) {
		bool has_matching_output_iface = find_interface_by_name(info.geometry_out_interfaces_, iface->instance_name) != nullptr;
		const char *suffix = (has_matching_output_iface) ? "_in[]" : "[]";
		print_interface(ss, "in", *iface, suffix);
	}
	ss << "\n";
	for (const StageInterfaceInfo *iface : info.geometry_out_interfaces_) {
		bool has_matching_input_iface = find_interface_by_name(info.vertex_out_interfaces_, iface->instance_name) != nullptr;
		const char *suffix = (has_matching_input_iface) ? "_out" : "";
		print_interface(ss, "out", *iface, suffix);
	}
	ss << "\n";
	return ss.str();
}

std::string GLShader::compute_layout_declare(const ShaderCreateInfo &info) const {
	std::stringstream ss;
	ss << "\n/* Compute Layout. */\n";
	ss << "layout(local_size_x = " << info.compute_layout_.local_size_x;
	if (info.compute_layout_.local_size_y != -1) {
		ss << ", local_size_y = " << info.compute_layout_.local_size_y;
	}
	if (info.compute_layout_.local_size_z != -1) {
		ss << ", local_size_z = " << info.compute_layout_.local_size_z;
	}
	ss << ") in;\n";
	ss << "\n";
	return ss.str();
}
/** \} */

/* -------------------------------------------------------------------- */
/** \name Passthrough geometry shader emulation
 *
 * \{ */

std::string GLShader::workaround_geometry_shader_source_create(const shader::ShaderCreateInfo &info) {
	std::stringstream ss;

	const bool do_layer_workaround = !GLContext::layered_rendering_support && bool(info.builtins_ & BuiltinBits::LAYER);
	const bool do_viewport_workaround = !GLContext::layered_rendering_support && bool(info.builtins_ & BuiltinBits::VIEWPORT_INDEX);
	const bool do_barycentric_workaround = !GLContext::native_barycentric_support && bool(info.builtins_ & BuiltinBits::BARYCENTRIC_COORD);

	shader::ShaderCreateInfo info_modified = info;
	info_modified.geometry_out_interfaces_ = info_modified.vertex_out_interfaces_;
	/**
	 * NOTE(@fclem): Assuming we will render TRIANGLES. This will not work with other primitive
	 * types. In this case, it might not trigger an error on some implementations.
	 */
	info_modified.geometry_layout(PrimitiveIn::TRIANGLES, PrimitiveOut::TRIANGLE_STRIP, 3);

	ss << geometry_layout_declare(info_modified);
	ss << geometry_interface_declare(info_modified);
	if (do_layer_workaround) {
		ss << "in int gpu_Layer[];\n";
	}
	if (do_viewport_workaround) {
		ss << "in int gpu_ViewportIndex[];\n";
	}
	if (do_barycentric_workaround) {
		ss << "flat out vec4 gpu_pos[3];\n";
		ss << "smooth out vec3 gpu_BaryCoord;\n";
		ss << "noperspective out vec3 gpu_BaryCoordNoPersp;\n";
	}
	ss << "\n";

	ss << "void main()\n";
	ss << "{\n";
	if (do_layer_workaround) {
		ss << "  gl_Layer = gpu_Layer[0];\n";
	}
	if (do_viewport_workaround) {
		ss << "  gl_ViewportIndex = gpu_ViewportIndex[0];\n";
	}
	if (do_barycentric_workaround) {
		ss << "  gpu_pos[0] = gl_in[0].gl_Position;\n";
		ss << "  gpu_pos[1] = gl_in[1].gl_Position;\n";
		ss << "  gpu_pos[2] = gl_in[2].gl_Position;\n";
	}
	for (auto i : IndexRange(3)) {
		for (StageInterfaceInfo *iface : info_modified.vertex_out_interfaces_) {
			for (auto &inout : iface->inouts) {
				ss << "  " << iface->instance_name << "_out." << inout.name;
				ss << " = " << iface->instance_name << "_in[" << i << "]." << inout.name << ";\n";
			}
		}
		if (do_barycentric_workaround) {
			ss << "  gpu_BaryCoordNoPersp = gpu_BaryCoord =";
			ss << " vec3(" << int(i == 0) << ", " << int(i == 1) << ", " << int(i == 2) << ");\n";
		}
		ss << "  gl_Position = gl_in[" << i << "].gl_Position;\n";
		ss << "  EmitVertex();\n";
	}
	ss << "}\n";
	return ss.str();
}

bool GLShader::do_geometry_shader_injection(const shader::ShaderCreateInfo *info) {
	BuiltinBits builtins = info->builtins_;
	if (!GLContext::native_barycentric_support && bool(builtins & BuiltinBits::BARYCENTRIC_COORD)) {
		return true;
	}
	if (!GLContext::layered_rendering_support && bool(builtins & BuiltinBits::LAYER)) {
		return true;
	}
	if (!GLContext::layered_rendering_support && bool(builtins & BuiltinBits::VIEWPORT_INDEX)) {
		return true;
	}
	return false;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Shader stage creation
 * \{ */

static const char *glsl_patch_default_get() {
	/** Used for shader patching. Init once. */
	static std::string patch;
	if (!patch.empty()) {
		return patch.c_str();
	}

	std::stringstream ss;
	/* Version need to go first. */
	if (gl_version() >= 43) {
		ss << "#version 430\n";
	}
	else {
		ss << "#version 330\n";
	}

	/* Enable extensions for features that are not part of our base GLSL version
	 * don't use an extension for something already available! */
	if (GLContext::texture_gather_support) {
		ss << "#extension GL_ARB_texture_gather: enable\n";
		/* Some drivers don't agree on epoxy_has_gl_extension("GL_ARB_texture_gather") and the actual
		 * support in the shader so double check the preprocessor define (see #56544). */
		ss << "#ifdef GL_ARB_texture_gather\n";
		ss << "#  define GPU_ARB_texture_gather\n";
		ss << "#endif\n";
	}
	if (GLContext::shader_draw_parameters_support) {
		ss << "#extension GL_ARB_shader_draw_parameters : enable\n";
		ss << "#define GPU_ARB_shader_draw_parameters\n";
		ss << "#define gpu_BaseInstance gl_BaseInstanceARB\n";
	}
	if (GLContext::geometry_shader_invocations) {
		ss << "#extension GL_ARB_gpu_shader5 : enable\n";
		ss << "#define GPU_ARB_gpu_shader5\n";
	}
	if (GLContext::texture_cube_map_array_support) {
		ss << "#extension GL_ARB_texture_cube_map_array : enable\n";
		ss << "#define GPU_ARB_texture_cube_map_array\n";
	}
	if (has_gl_extension("GL_ARB_conservative_depth")) {
		ss << "#extension GL_ARB_conservative_depth : enable\n";
	}
	if (GPU_get_info_i(GPU_INFO_SHADER_IMAGE_LOAD_STORE_SUPPORT)) {
		ss << "#extension GL_ARB_shader_image_load_store: enable\n";
		ss << "#extension GL_ARB_shading_language_420pack: enable\n";
	}
	if (GLContext::layered_rendering_support) {
		ss << "#extension GL_ARB_shader_viewport_layer_array: enable\n";
		ss << "#define gpu_Layer gl_Layer\n";
		ss << "#define gpu_ViewportIndex gl_ViewportIndex\n";
	}
	if (GLContext::native_barycentric_support) {
		ss << "#extension GL_AMD_shader_explicit_vertex_parameter: enable\n";
	}
	if (GLContext::framebuffer_fetch_support) {
		ss << "#extension GL_EXT_shader_framebuffer_fetch: enable\n";
	}

	/* Fallbacks. */
	if (!GLContext::shader_draw_parameters_support) {
		ss << "uniform int gpu_BaseInstance;\n";
	}

	/* Vulkan GLSL compatibility. */
	ss << "#define gpu_InstanceIndex (gl_InstanceID + gpu_BaseInstance)\n";
	ss << "#define gpu_EmitVertex EmitVertex\n";

	/* Array compatibility. */
	ss << "#define gpu_Array(_type) _type[]\n";

	/* Derivative sign can change depending on implementation. */
	ss << "#define DFDX_SIGN " << std::setprecision(2) << GLContext::derivative_signs[0] << "\n";
	ss << "#define DFDY_SIGN " << std::setprecision(2) << GLContext::derivative_signs[1] << "\n";

	/* GLSL Backend Lib. */
	ss << datatoc_glsl_shader_defines_glsl;

	patch = ss.str();
	return patch.c_str();
}

static const char *glsl_patch_compute_get() {
	/** Used for shader patching. Init once. */
	static std::string patch;
	if (!patch.empty()) {
		return patch.c_str();
	}

	std::stringstream ss;
	/* Version need to go first. */
	ss << "#version 430\n";
	ss << "#extension GL_ARB_compute_shader :enable\n";

	if (GLContext::texture_cube_map_array_support) {
		ss << "#extension GL_ARB_texture_cube_map_array : enable\n";
		ss << "#define GPU_ARB_texture_cube_map_array\n";
	}

	/* Array compatibility. */
	ss << "#define gpu_Array(_type) _type[]\n";

	ss << datatoc_glsl_shader_defines_glsl;

	patch = ss.str();
	return patch.c_str();
}

const char *GLShader::glsl_patch_get(GLenum gl_stage) {
	if (gl_stage == GL_COMPUTE_SHADER) {
		return glsl_patch_compute_get();
	}
	return glsl_patch_default_get();
}

GLuint GLShader::create_shader_stage(GLenum gl_stage, MutableSpan<const char *> sources) {
	GLuint shader = glCreateShader(gl_stage);
	if (shader == 0) {
		fprintf(stderr, "GLShader: Error: Could not create shader object.\n");
		return 0;
	}

	/* Patch the shader code using the first source slot. */
	sources[0] = glsl_patch_get(gl_stage);

	glShaderSource(shader, sources.size(), sources.data(), nullptr);
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status) {
		char log[5000] = "";
		glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
		if (log[0] != '\0') {
			GLLogParser parser;
			switch (gl_stage) {
				case GL_VERTEX_SHADER:
					this->print_log(sources, log, "VertShader", !status, &parser);
					break;
				case GL_GEOMETRY_SHADER:
					this->print_log(sources, log, "GeomShader", !status, &parser);
					break;
				case GL_FRAGMENT_SHADER:
					this->print_log(sources, log, "FragShader", !status, &parser);
					break;
				case GL_COMPUTE_SHADER:
					this->print_log(sources, log, "ComputeShader", !status, &parser);
					break;
			}
		}
	}
	if (!status) {
		glDeleteShader(shader);
		compilation_failed_ = true;
		return 0;
	}

	glAttachShader(shader_program_, shader);
	return shader;
}

void GLShader::vertex_shader_from_glsl(MutableSpan<const char *> sources) {
	vert_shader_ = this->create_shader_stage(GL_VERTEX_SHADER, sources);
}

void GLShader::geometry_shader_from_glsl(MutableSpan<const char *> sources) {
	geom_shader_ = this->create_shader_stage(GL_GEOMETRY_SHADER, sources);
}

void GLShader::fragment_shader_from_glsl(MutableSpan<const char *> sources) {
	frag_shader_ = this->create_shader_stage(GL_FRAGMENT_SHADER, sources);
}

void GLShader::compute_shader_from_glsl(MutableSpan<const char *> sources) {
	compute_shader_ = this->create_shader_stage(GL_COMPUTE_SHADER, sources);
}

bool GLShader::finalize(const shader::ShaderCreateInfo *info) {
	if (compilation_failed_) {
		return false;
	}

	if (info && do_geometry_shader_injection(info)) {
		std::string source = workaround_geometry_shader_source_create(*info);
		Vector<const char *> sources;
		sources.append("version");
		sources.append(source.c_str());
		geometry_shader_from_glsl(sources);
	}

	glLinkProgram(shader_program_);

	GLint status;
	glGetProgramiv(shader_program_, GL_LINK_STATUS, &status);
	if (!status) {
		char log[5000];
		glGetProgramInfoLog(shader_program_, sizeof(log), nullptr, log);
		Span<const char *> sources;
		GLLogParser parser;
		this->print_log(sources, log, "Linking", true, &parser);
		return false;
	}

	if (info != nullptr && info->legacy_resource_location_ == false) {
		interface = MEM_new<GLShaderInterface>("rose::gpu::ShaderInterface", shader_program_, *info);
	}
	else {
		interface = MEM_new<GLShaderInterface>("rose::gpu::ShaderInterface", shader_program_);
	}

	return true;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Binding
 * \{ */

void GLShader::bind() {
	ROSE_assert(shader_program_ != 0);
	glUseProgram(shader_program_);
}

void GLShader::unbind() {
#ifndef NDEBUG
	glUseProgram(0);
#endif
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Uniforms setters
 * \{ */

void GLShader::uniform_float(int location, int comp_len, int array_size, const float *data) {
	switch (comp_len) {
		case 1:
			glUniform1fv(location, array_size, data);
			break;
		case 2:
			glUniform2fv(location, array_size, data);
			break;
		case 3:
			glUniform3fv(location, array_size, data);
			break;
		case 4:
			glUniform4fv(location, array_size, data);
			break;
		case 9:
			glUniformMatrix3fv(location, array_size, 0, data);
			break;
		case 16:
			glUniformMatrix4fv(location, array_size, 0, data);
			break;
		default:
			ROSE_assert(0);
			break;
	}
}

void GLShader::uniform_int(int location, int comp_len, int array_size, const int *data) {
	switch (comp_len) {
		case 1:
			glUniform1iv(location, array_size, data);
			break;
		case 2:
			glUniform2iv(location, array_size, data);
			break;
		case 3:
			glUniform3iv(location, array_size, data);
			break;
		case 4:
			glUniform4iv(location, array_size, data);
			break;
		default:
			ROSE_assert(0);
			break;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name GPUVertFormat from Shader
 * \{ */

int GLShader::program_handle_get() const {
	return int(this->shader_program_);
}

/** \} */
