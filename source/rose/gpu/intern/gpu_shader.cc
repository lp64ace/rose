#include "MEM_guardedalloc.h"

#include "LIB_math_matrix.h"
#include "LIB_string.h"

#include "GPU_info.h"
#include "GPU_matrix.h"
#include "GPU_platform.h"

#include "gpu_backend.hh"
#include "gpu_context_private.hh"
#include "gpu_shader_create_info.hh"
#include "gpu_shader_create_info_private.hh"
#include "gpu_shader_dependency_private.h"
#include "gpu_shader_private.hh"

#include <string>

extern "C" char datatoc_gpu_shader_colorspace_lib_glsl[];

namespace rose::gpu {

std::string Shader::defines_declare(const shader::ShaderCreateInfo &info) const {
	std::string defines;
	for (const auto &def : info.defines_) {
		defines += "#define ";
		defines += def[0];
		defines += " ";
		defines += def[1];
		defines += "\n";
	}
	return defines;
}

}  // namespace rose::gpu

using namespace rose;
using namespace rose::gpu;

/* -------------------------------------------------------------------- */
/** \name Creation / Destruction
 * \{ */

Shader::Shader(const char *name) {
	LIB_strcpy(this->name, ARRAY_SIZE(this->name), name);
}

Shader::~Shader() {
	MEM_delete(this->interface);
}

static void standard_defines(Vector<const char *> &sources) {
	ROSE_assert(sources.is_empty());
	/* Version needs to be first, exact value will be added by implementation. */
	sources.append("version");
	/* Define to identify code usage in shading language. */
	sources.append("#define GPU_SHADER\n");

	/* some useful defines to detect GPU type. */
	if (GPU_type_matches(GPU_DEVICE_ATI, GPU_OS_ANY, GPU_DRIVER_ANY)) {
		sources.append("#define GPU_ATI\n");
	}
	else if (GPU_type_matches(GPU_DEVICE_NVIDIA, GPU_OS_ANY, GPU_DRIVER_ANY)) {
		sources.append("#define GPU_NVIDIA\n");
	}
	else if (GPU_type_matches(GPU_DEVICE_INTEL, GPU_OS_ANY, GPU_DRIVER_ANY)) {
		sources.append("#define GPU_INTEL\n");
	}

	/** Some useful defines to detect OS type. */
	if (GPU_type_matches(GPU_DEVICE_ANY, GPU_OS_WIN, GPU_DRIVER_ANY)) {
		sources.append("#define OS_WIN\n");
	}
	else if (GPU_type_matches(GPU_DEVICE_ANY, GPU_OS_MAC, GPU_DRIVER_ANY)) {
		sources.append("#define OS_MAC\n");
	}
	else if (GPU_type_matches(GPU_DEVICE_ANY, GPU_OS_UNIX, GPU_DRIVER_ANY)) {
		sources.append("#define OS_UNIX\n");
	}

	/* API Definition */
	BackendType backend = GPU_backend_get_type();
	switch (backend) {
		case GPU_BACKEND_OPENGL:
			sources.append("#define GPU_OPENGL\n");
			break;
		default:
			ROSE_assert_msg(0, "Invalid GPU Backend Type");
			break;
	}

	if (GPU_get_info_i(GPU_INFO_BROKEN_AMD_DRIVER)) {
		sources.append("#define GPU_DEPRECATED_AMD_DRIVER\n");
	}
}

GPUShader *GPU_shader_create_from_info(const GPUShaderCreateInfo *_info) {
	using namespace rose::gpu::shader;
	const ShaderCreateInfo &info = *reinterpret_cast<const ShaderCreateInfo *>(_info);

	const_cast<ShaderCreateInfo &>(info).finalize();

	const std::string error = info.check_error();
	if (!error.empty()) {
		std::cerr << error.c_str() << "\n";
		ROSE_assert_unreachable();
	}

	Shader *shader = GPUBackend::get()->shader_alloc(info.name_.c_str());

	std::string defines = shader->defines_declare(info);
	std::string resources = shader->resources_declare(info);

	if (info.legacy_resource_location_ == false) {
		defines += "#define USE_GPU_SHADER_CREATE_INFO\n";
	}

	Vector<const char *> typedefs;
	if (!info.typedef_sources_.is_empty() || !info.typedef_source_generated.empty()) {
		typedefs.append(gpu_shader_dependency_get_source("GPU_shader_shared_utils.h").c_str());
	}
	if (!info.typedef_source_generated.empty()) {
		typedefs.append(info.typedef_source_generated.c_str());
	}
	for (auto filename : info.typedef_sources_) {
		typedefs.append(gpu_shader_dependency_get_source(filename).c_str());
	}

	if (!info.vertex_source_.is_empty()) {
		auto code = gpu_shader_dependency_get_resolved_source(info.vertex_source_);
		std::string interface = shader->vertex_interface_declare(info);

		Vector<const char *> sources;
		standard_defines(sources);
		sources.append("#define GPU_VERTEX_SHADER\n");
		if (!info.geometry_source_.is_empty()) {
			sources.append("#define USE_GEOMETRY_SHADER\n");
		}
		sources.append(defines.c_str());
		sources.extend(typedefs);
		sources.append(resources.c_str());
		sources.append(interface.c_str());
		sources.extend(code);
		sources.extend(info.dependencies_generated);
		sources.append(info.vertex_source_generated.c_str());

		shader->vertex_shader_from_glsl(sources);
	}

	if (!info.fragment_source_.is_empty()) {
		auto code = gpu_shader_dependency_get_resolved_source(info.fragment_source_);
		std::string interface = shader->fragment_interface_declare(info);

		Vector<const char *> sources;
		standard_defines(sources);
		sources.append("#define GPU_FRAGMENT_SHADER\n");
		if (!info.geometry_source_.is_empty()) {
			sources.append("#define USE_GEOMETRY_SHADER\n");
		}
		sources.append(defines.c_str());
		sources.extend(typedefs);
		sources.append(resources.c_str());
		sources.append(interface.c_str());
		sources.extend(code);
		sources.extend(info.dependencies_generated);
		sources.append(info.fragment_source_generated.c_str());

		shader->fragment_shader_from_glsl(sources);
	}

	if (!info.geometry_source_.is_empty()) {
		auto code = gpu_shader_dependency_get_resolved_source(info.geometry_source_);
		std::string layout = shader->geometry_layout_declare(info);
		std::string interface = shader->geometry_interface_declare(info);

		Vector<const char *> sources;
		standard_defines(sources);
		sources.append("#define GPU_GEOMETRY_SHADER\n");
		sources.append(defines.c_str());
		sources.extend(typedefs);
		sources.append(resources.c_str());
		sources.append(layout.c_str());
		sources.append(interface.c_str());
		sources.append(info.geometry_source_generated.c_str());
		sources.extend(code);

		shader->geometry_shader_from_glsl(sources);
	}

	if (!info.compute_source_.is_empty()) {
		auto code = gpu_shader_dependency_get_resolved_source(info.compute_source_);
		std::string layout = shader->compute_layout_declare(info);

		Vector<const char *> sources;
		standard_defines(sources);
		sources.append("#define GPU_COMPUTE_SHADER\n");
		sources.append(defines.c_str());
		sources.extend(typedefs);
		sources.append(resources.c_str());
		sources.append(layout.c_str());
		sources.extend(code);
		sources.extend(info.dependencies_generated);
		sources.append(info.compute_source_generated.c_str());

		shader->compute_shader_from_glsl(sources);
	}

	if (!shader->finalize(&info)) {
		MEM_delete<Shader>(shader);
		return nullptr;
	}

	return wrap(shader);
}

GPUShader *GPU_shader_create_from_info_name(const char *info_name) {
	using namespace rose::gpu::shader;
	const GPUShaderCreateInfo *_info = gpu_shader_create_info_get(info_name);
	const ShaderCreateInfo &info = *reinterpret_cast<const ShaderCreateInfo *>(_info);
	if (!info.do_static_compilation_) {
		std::cerr << "Warning: Trying to compile \"" << info.name_.c_str() << "\" which was not marked for static compilation.\n";
	}
	return GPU_shader_create_from_info(_info);
}

const GPUShaderCreateInfo *GPU_shader_create_info_get(const char *info_name) {
	return gpu_shader_create_info_get(info_name);
}

bool GPUshader_create_info_check_error(const GPUShaderCreateInfo *_info, char r_error[128]) {
	using namespace rose::gpu::shader;
	const ShaderCreateInfo &info = *reinterpret_cast<const ShaderCreateInfo *>(_info);
	std::string error = info.check_error();
	if (error.length() == 0) {
		return true;
	}

	LIB_strcpy(r_error, 128, error.c_str());
	return false;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Free
 * \{ */

void GPU_shader_free(GPUShader *shader) {
	MEM_delete<Shader>(unwrap(shader));
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Binding
 * \{ */

void GPU_shader_bind(GPUShader *gpu_shader) {
	Shader *shader = unwrap(gpu_shader);

	Context *ctx = Context::get();

	if (ctx->shader != shader) {
		ctx->shader = shader;
		shader->bind();
		GPU_matrix_bind(gpu_shader);
		Shader::set_srgb_uniform(gpu_shader);
	}
	else {
		if (Shader::srgb_uniform_dirty_get()) {
			Shader::set_srgb_uniform(gpu_shader);
		}
		if (GPU_matrix_dirty_get()) {
			GPU_matrix_bind(gpu_shader);
		}
	}
}

void GPU_shader_unbind(void) {
#ifndef NDEBUG
	Context *ctx = Context::get();
	if (ctx->shader) {
		ctx->shader->unbind();
	}
	ctx->shader = nullptr;
#endif
}

GPUShader *GPU_shader_get_bound(void) {
	Context *ctx = Context::get();
	if (ctx) {
		return wrap(ctx->shader);
	}
	return nullptr;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Uniform API.
 * \{ */

int GPU_shader_get_ubo_binding(GPUShader *shader, const char *name) {
	const ShaderInterface *interface = unwrap(shader)->interface;
	const ShaderInput *ubo = interface->ubo_get(name);
	return ubo ? ubo->binding : -1;
}
int GPU_shader_get_ssbo_binding(GPUShader *shader, const char *name) {
	const ShaderInterface *interface = unwrap(shader)->interface;
	const ShaderInput *ssbo = interface->ssbo_get(name);
	return ssbo ? ssbo->location : -1;
}
int GPU_shader_get_sampler_binding(GPUShader *shader, const char *name) {
	const ShaderInterface *interface = unwrap(shader)->interface;
	const ShaderInput *tex = interface->uniform_get(name);
	return tex ? tex->binding : -1;
}

int GPU_shader_get_uniform(GPUShader *shader, const char *name) {
	const ShaderInterface *interface = unwrap(shader)->interface;
	const ShaderInput *uniform = interface->uniform_get(name);
	return uniform ? uniform->location : -1;
}

void GPU_shader_uniform_float_ex(GPUShader *shader, int location, int length, int array_size, const float *value) {
	unwrap(shader)->uniform_float(location, length, array_size, value);
}
void GPU_shader_uniform_int_ex(GPUShader *shader, int location, int length, int array_size, const int *value) {
	unwrap(shader)->uniform_int(location, length, array_size, value);
}

void GPU_shader_uniform_1i(GPUShader *sh, const char *name, int value) {
	const int loc = GPU_shader_get_uniform(sh, name);
	GPU_shader_uniform_int_ex(sh, loc, 1, 1, &value);
}
void GPU_shader_uniform_1b(GPUShader *sh, const char *name, bool value) {
	GPU_shader_uniform_1i(sh, name, value ? 1 : 0);
}
void GPU_shader_uniform_2f(GPUShader *sh, const char *name, float x, float y) {
	const float data[2] = {x, y};
	GPU_shader_uniform_2fv(sh, name, data);
}
void GPU_shader_uniform_3f(GPUShader *sh, const char *name, float x, float y, float z) {
	const float data[3] = {x, y, z};
	GPU_shader_uniform_3fv(sh, name, data);
}
void GPU_shader_uniform_4f(GPUShader *sh, const char *name, float x, float y, float z, float w) {
	const float data[4] = {x, y, z, w};
	GPU_shader_uniform_4fv(sh, name, data);
}
void GPU_shader_uniform_1f(GPUShader *sh, const char *name, float value) {
	const int loc = GPU_shader_get_uniform(sh, name);
	GPU_shader_uniform_float_ex(sh, loc, 1, 1, &value);
}
void GPU_shader_uniform_2fv(GPUShader *sh, const char *name, const float data[2]) {
	const int loc = GPU_shader_get_uniform(sh, name);
	GPU_shader_uniform_float_ex(sh, loc, 2, 1, data);
}
void GPU_shader_uniform_3fv(GPUShader *sh, const char *name, const float data[3]) {
	const int loc = GPU_shader_get_uniform(sh, name);
	GPU_shader_uniform_float_ex(sh, loc, 3, 1, data);
}
void GPU_shader_uniform_4fv(GPUShader *sh, const char *name, const float data[4]) {
	const int loc = GPU_shader_get_uniform(sh, name);
	GPU_shader_uniform_float_ex(sh, loc, 4, 1, data);
}
void GPU_shader_uniform_2iv(GPUShader *sh, const char *name, const int data[2]) {
	const int loc = GPU_shader_get_uniform(sh, name);
	GPU_shader_uniform_int_ex(sh, loc, 2, 1, data);
}
void GPU_shader_uniform_mat4(GPUShader *sh, const char *name, const float data[4][4]) {
	const int loc = GPU_shader_get_uniform(sh, name);
	GPU_shader_uniform_float_ex(sh, loc, 16, 1, (const float *)data);
}
void GPU_shader_uniform_mat3_as_mat4(GPUShader *sh, const char *name, const float data[3][3]) {
	float matrix[4][4];
	copy_m4_m3(matrix, data);
	GPU_shader_uniform_mat4(sh, name, matrix);
}
void GPU_shader_uniform_1f_array(GPUShader *sh, const char *name, int len, const float *val) {
	const int loc = GPU_shader_get_uniform(sh, name);
	GPU_shader_uniform_float_ex(sh, loc, 1, len, val);
}
void GPU_shader_uniform_2fv_array(GPUShader *sh, const char *name, int len, const float (*val)[2]) {
	const int loc = GPU_shader_get_uniform(sh, name);
	GPU_shader_uniform_float_ex(sh, loc, 2, len, (const float *)val);
}
void GPU_shader_uniform_4fv_array(GPUShader *sh, const char *name, int len, const float (*val)[4]) {
	const int loc = GPU_shader_get_uniform(sh, name);
	GPU_shader_uniform_float_ex(sh, loc, 4, len, (const float *)val);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Uniforms / Resource location
 * \{ */

int GPU_shader_get_builtin_uniform(GPUShader *shader, int builtin) {
	const ShaderInterface *interface = unwrap(shader)->interface;
	return interface->uniform_builtin((UniformBuiltin)builtin);
}

uint GPU_shader_get_attribute_len(const GPUShader *shader) {
	const ShaderInterface *interface = unwrap(shader)->interface;
	return interface->attr_len_;
}

int GPU_shader_get_attribute(const GPUShader *shader, const char *name) {
	const ShaderInterface *interface = unwrap(shader)->interface;
	const ShaderInput *attr = interface->attr_get(name);
	return attr ? attr->location : -1;
}

bool GPU_shader_get_attribute_info(const GPUShader *shader, int attr_location, char r_name[256], int *r_type) {
	const ShaderInterface *interface = unwrap(shader)->interface;

	const ShaderInput *attr = interface->attr_get(attr_location);
	if (!attr) {
		return false;
	}

	LIB_strcpy(r_name, 256, interface->input_name_get(attr));
	*r_type = attr->location != -1 ? interface->attr_types_[attr->location] : -1;
	return true;
}

/* \} */

namespace rose::gpu {

/* -------------------------------------------------------------------- */
/** \name sRGB Rendering Workaround
 *
 * The viewport overlay frame-buffer is sRGB and will expect shaders to output display referred
 * Linear colors. But other frame-buffers (i.e: the area frame-buffers) are not sRGB and require
 * the shader output color to be in sRGB space
 * (assumed display encoded color-space as the time of writing).
 * For this reason we have a uniform to switch the transform on and off depending on the current
 * frame-buffer color-space.
 * \{ */

static int g_shader_builtin_srgb_transform = 0;
static bool g_shader_builtin_srgb_is_dirty = false;

bool Shader::srgb_uniform_dirty_get() {
	return g_shader_builtin_srgb_is_dirty;
}

void Shader::set_srgb_uniform(GPUShader *shader) {
	int32_t loc = GPU_shader_get_builtin_uniform(shader, GPU_UNIFORM_SRGB_TRANSFORM);
	if (loc != -1) {
		GPU_shader_uniform_int_ex(shader, loc, 1, 1, &g_shader_builtin_srgb_transform);
	}
	g_shader_builtin_srgb_is_dirty = false;
}

void Shader::set_framebuffer_srgb_target(int use_srgb_to_linear) {
	if (g_shader_builtin_srgb_transform != use_srgb_to_linear) {
		g_shader_builtin_srgb_transform = use_srgb_to_linear;
		g_shader_builtin_srgb_is_dirty = true;
	}
}

/** \} */

}  // namespace rose::gpu
