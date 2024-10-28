#include "gl_backend.hh"

#include "intern/gpu_framebuffer_private.hh"
#include "intern/gpu_info_private.hh"
#include "intern/gpu_platform_private.hh"

#include "LIB_math_vector.h"
#include "LIB_vector.hh"

#include <string>

namespace rose::gpu {

/* -------------------------------------------------------------------- */
/** \name Platform
 * \{ */

void GLBackend::platform_init() {
	ROSE_assert(!GPG.initialized);

	const char *vendor = (const char *)glGetString(GL_VENDOR);
	const char *renderer = (const char *)glGetString(GL_RENDERER);
	const char *version = (const char *)glGetString(GL_VERSION);

	DeviceType device = GPU_DEVICE_ANY;
	OperatingSystemType system = GPU_OS_ANY;
	DriverType driver = GPU_DRIVER_ANY;
	SupportLevel support_level = GPU_SUPPORT_LEVEL_SUPPORTED;

#ifdef WIN32
	system = GPU_OS_WIN;
#else
	system = GPU_OS_UNIX;
#endif

	if (!vendor) {
		printf("Warning: No OpenGL vendor detected.\n");
		device = GPU_DEVICE_UNKNOWN;
		driver = GPU_DRIVER_OFFICIAL;
	}
	else if (strstr(vendor, "ATI") || strstr(vendor, "AMD")) {
		device = GPU_DEVICE_ATI;
		driver = GPU_DRIVER_OFFICIAL;
	}
	else if (strstr(vendor, "NVIDIA")) {
		device = GPU_DEVICE_NVIDIA;
		driver = GPU_DRIVER_OFFICIAL;
	}
	else if (strstr(vendor, "Intel") ||
			 /* src/mesa/drivers/dri/intel/intel_context.c */
			 strstr(renderer, "Mesa DRI Intel") || strstr(renderer, "Mesa DRI Mobile Intel")) {
		device = GPU_DEVICE_INTEL;
		driver = GPU_DRIVER_OFFICIAL;

		if (strstr(renderer, "UHD Graphics") ||
			/* Not UHD but affected by the same bugs. */
			strstr(renderer, "HD Graphics 530") || strstr(renderer, "Kaby Lake GT2") || strstr(renderer, "Whiskey Lake")) {
			device |= GPU_DEVICE_INTEL_UHD;
		}
	}
	else if (strstr(renderer, "Mesa DRI R") || (strstr(renderer, "Radeon") && strstr(vendor, "X.Org")) || (strstr(renderer, "AMD") && strstr(vendor, "X.Org")) || (strstr(renderer, "Gallium ") && strstr(renderer, " on ATI ")) || (strstr(renderer, "Gallium ") && strstr(renderer, " on AMD "))) {
		device = GPU_DEVICE_ATI;
		driver = GPU_DRIVER_OPENSOURCE;
	}
	else if (strstr(renderer, "Nouveau") || strstr(vendor, "nouveau")) {
		device = GPU_DEVICE_NVIDIA;
		driver = GPU_DRIVER_OPENSOURCE;
	}
	else if (strstr(vendor, "Mesa")) {
		device = GPU_DEVICE_SOFTWARE;
		driver = GPU_DRIVER_SOFTWARE;
	}
	else if (strstr(vendor, "Microsoft")) {
		/* Qualcomm devices use Mesa's GLOn12, which claims to be vended by Microsoft */
		if (strstr(renderer, "Qualcomm")) {
			device = GPU_DEVICE_QUALCOMM;
			driver = GPU_DRIVER_OFFICIAL;
		}
		else {
			device = GPU_DEVICE_SOFTWARE;
			driver = GPU_DRIVER_SOFTWARE;
		}
	}
	else if (strstr(vendor, "Apple")) {
		/* Apple Silicon. */
		device = GPU_DEVICE_APPLE;
		driver = GPU_DRIVER_OFFICIAL;
	}
	else if (strstr(renderer, "Apple Software Renderer")) {
		device = GPU_DEVICE_SOFTWARE;
		driver = GPU_DRIVER_SOFTWARE;
	}
	else if (strstr(renderer, "llvmpipe") || strstr(renderer, "softpipe")) {
		device = GPU_DEVICE_SOFTWARE;
		driver = GPU_DRIVER_SOFTWARE;
	}
	else {
		printf("Warning: Could not find a matching GPU name. Things may not behave as expected.\n");
		printf("Detected OpenGL configuration:\n");
		printf("Vendor: %s\n", vendor);
		printf("Renderer: %s\n", renderer);
	}

	if (!(gl_version() >= 43)) {
		support_level = GPU_SUPPORT_LEVEL_UNSUPPORTED;
	}
	else {
		if ((device & GPU_DEVICE_INTEL) && (system & GPU_OS_WIN)) {
			/**
			 * Old Intel drivers with known bugs that cause material properties to crash.
			 * Version Build 10.18.14.5067 is the latest available and appears to be working
			 * ok with our workarounds, so excluded from this list.
			 */
			if (strstr(version, "Build 7.14") || strstr(version, "Build 7.15") || strstr(version, "Build 8.15") || strstr(version, "Build 9.17") || strstr(version, "Build 9.18") || strstr(version, "Build 10.18.10.3") || strstr(version, "Build 10.18.10.4") || strstr(version, "Build 10.18.10.5") || strstr(version, "Build 10.18.14.4")) {
				support_level = GPU_SUPPORT_LEVEL_LIMITED;
			}
			/**
			 * Latest Intel driver have bugs that won't allow Rose to start.
			 * Users must install different version of the driver.
			 * See #113124 for more information.
			 */
			if (strstr(version, "Build 20.19.15.51")) {
				support_level = GPU_SUPPORT_LEVEL_UNSUPPORTED;
			}
		}
		if ((device & GPU_DEVICE_ATI) && (system & GPU_OS_UNIX)) {
			/**
			 * Platform seems to work when SB backend is disabled. This can be done
			 * by adding the environment variable `R600_DEBUG=nosb`.
			 */
			if (strstr(renderer, "AMD CEDAR")) {
				support_level = GPU_SUPPORT_LEVEL_LIMITED;
			}
		}
	}

	GPG.init(device, system, support_level, GPU_BACKEND_OPENGL, vendor, renderer, version, GPU_ARCHITECTURE_IMR);
}

void GLBackend::platform_exit() {
	ROSE_assert(GPG.initialized);

	GPG.clear();
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Capabilities
 * \{ */

static bool match_renderer(std::string renderer, const rose::Vector<std::string> &items) {
	for (const std::string &item : items) {
		const std::string wrapped = " " + item + " ";
		if (renderer.compare(renderer.size() - item.size(), item.size(), item) == 0 || renderer.find(wrapped) != std::string::npos) {
			return true;
		}
	}
	return false;
}

static bool detect_mip_render_workaround() {
	int cube_size = 2;
	float clear_color[4] = {1.0f, 0.5f, 0.0f, 0.0f};
	float *source_pix = (float *)MEM_callocN(sizeof(float[4]) * cube_size * cube_size * 6, __func__);

	/* Not using GPU API since it is not yet fully initialized. */
	GLuint tex, fb;
	/* Create cubemap with 2 mip level. */
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
	for (int mip = 0; mip < 2; mip++) {
		for (int i = 0; i < 6; i++) {
			const int width = cube_size / (1 << mip);
			GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
			glTexImage2D(target, mip, GL_RGBA16F, width, width, 0, GL_RGBA, GL_FLOAT, source_pix);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
	/* Attach and clear mip 1. */
	glGenFramebuffers(1, &fb);
	glBindFramebuffer(GL_FRAMEBUFFER, fb);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 1);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClearColor(UNPACK4(clear_color));
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	/* Read mip 1. If color is not the same as the clear_color, the rendering failed. */
	glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 1, GL_RGBA, GL_FLOAT, source_pix);
	bool enable_workaround = !equals_v4_v4(clear_color, source_pix);
	MEM_freeN(source_pix);

	glDeleteFramebuffers(1, &fb);
	glDeleteTextures(1, &tex);

	return enable_workaround;
}

static void detect_workarounds() {
	const char *vendor = (const char *)glGetString(GL_VENDOR);
	const char *renderer = (const char *)glGetString(GL_RENDERER);
	const char *version = (const char *)glGetString(GL_VERSION);

	if (0) {
		printf("\n");
		printf("GL: Forcing workaround usage and disabling extensions.\n");
		printf("    OpenGL identification strings\n");
		printf("    vendor: %s\n", vendor);
		printf("    renderer: %s\n", renderer);
		printf("    version: %s\n\n", version);
		gpu_set_info_i(GPU_INFO_DEPTH_BLITTING_WORKAROUND, true);
		gpu_set_info_i(GPU_INFO_MIP_RENDER_WORKAROUND, true);
		GLContext::debug_layer_workaround = true;
		GLContext::unused_fb_slot_workaround = true;
		/* Turn off extensions. */
		gpu_set_info_i(GPU_INFO_SHADER_IMAGE_LOAD_STORE_SUPPORT, false);
		gpu_set_info_i(GPU_INFO_SHADER_DRAW_PARAMETERS_SUPPORT, false);
		gpu_set_info_i(GPU_INFO_SHADER_STORAGE_BUFFER_OBJECTS_SUPPORT, false);
		GLContext::base_instance_support = false;
		GLContext::clear_texture_support = false;
		GLContext::copy_image_support = false;
		GLContext::debug_layer_support = false;
		GLContext::direct_state_access_support = false;
		GLContext::fixed_restart_index_support = false;
		GLContext::geometry_shader_invocations = false;
		GLContext::layered_rendering_support = false;
		GLContext::native_barycentric_support = false;
		GLContext::multi_bind_support = false;
		GLContext::multi_draw_indirect_support = false;
		GLContext::shader_draw_parameters_support = false;
		GLContext::texture_cube_map_array_support = false;
		GLContext::texture_filter_anisotropic_support = false;
		GLContext::texture_gather_support = false;
		GLContext::texture_storage_support = false;
		GLContext::vertex_attrib_binding_support = false;
		return;
	}

	/* Limit support for GL_ARB_base_instance to OpenGL 4.0 and higher. NVIDIA Quadro FX 4800
	 * (TeraScale) report that they support GL_ARB_base_instance, but the driver does not support
	 * GLEW_ARB_draw_indirect as it has an OpenGL3 context what also matches the minimum needed
	 * requirements.
	 *
	 * We use it as a target for glMapBuffer(Range) what is part of the OpenGL 4 API. So better
	 * disable it when we don't have an OpenGL4 context (See #77657) */
	if (!(gl_version() >= 40)) {
		GLContext::base_instance_support = false;
	}
	if (GPU_type_matches(GPU_DEVICE_ATI, GPU_OS_WIN, GPU_DRIVER_OFFICIAL) && (strstr(version, "4.5.13399") || strstr(version, "4.5.13417") || strstr(version, "4.5.13422") || strstr(version, "4.5.13467"))) {
		/* The renderers include:
		 *   Radeon HD 5000;
		 *   Radeon HD 7500M;
		 *   Radeon HD 7570M;
		 *   Radeon HD 7600M;
		 *   Radeon R5 Graphics;
		 * And others... */
		GLContext::unused_fb_slot_workaround = true;
		gpu_set_info_i(GPU_INFO_MIP_RENDER_WORKAROUND, true);
		gpu_set_info_i(GPU_INFO_SHADER_IMAGE_LOAD_STORE_SUPPORT, false);
		gpu_set_info_i(GPU_INFO_SHADER_DRAW_PARAMETERS_SUPPORT, false);
		gpu_set_info_i(GPU_INFO_BROKEN_AMD_DRIVER, true);
	}
	/* Compute shaders have some issues with those versions (see #94936). */
	if (GPU_type_matches(GPU_DEVICE_ATI, GPU_OS_ANY, GPU_DRIVER_OFFICIAL) && (strstr(version, "4.5.14831") || strstr(version, "4.5.14760"))) {
		gpu_set_info_i(GPU_INFO_COMPUTE_SHADER_SUPPORT, false);
	}
	/* We have issues with this specific renderer. (see #74024) */
	if (GPU_type_matches(GPU_DEVICE_ATI, GPU_OS_UNIX, GPU_DRIVER_OPENSOURCE) && (strstr(renderer, "AMD VERDE") || strstr(renderer, "AMD KAVERI") || strstr(renderer, "AMD TAHITI"))) {
		GLContext::unused_fb_slot_workaround = true;
		gpu_set_info_i(GPU_INFO_SHADER_IMAGE_LOAD_STORE_SUPPORT, false);
		gpu_set_info_i(GPU_INFO_SHADER_DRAW_PARAMETERS_SUPPORT, false);
		gpu_set_info_i(GPU_INFO_BROKEN_AMD_DRIVER, true);
	}
	/* Fix slowdown on this particular driver. (see #77641) */
	if (GPU_type_matches(GPU_DEVICE_ATI, GPU_OS_UNIX, GPU_DRIVER_OPENSOURCE) && strstr(version, "Mesa 19.3.4")) {
		gpu_set_info_i(GPU_INFO_SHADER_IMAGE_LOAD_STORE_SUPPORT, false);
		gpu_set_info_i(GPU_INFO_SHADER_DRAW_PARAMETERS_SUPPORT, false);
		gpu_set_info_i(GPU_INFO_BROKEN_AMD_DRIVER, true);
	}
	/* See #82856: AMD drivers since 20.11 running on a polaris architecture doesn't support the
	 * `GL_INT_2_10_10_10_REV` data type correctly. This data type is used to pack normals and
	 * flags. The work around uses `GPU_RGBA16I`. In 22.?.? drivers this has been fixed for polaris
	 * platform. Keeping legacy platforms around just in case.
	 */
	if (GPU_type_matches(GPU_DEVICE_ATI, GPU_OS_ANY, GPU_DRIVER_OFFICIAL)) {
		const Vector<std::string> matches = {"RX550/550", "(TM) 520", "(TM) 530", "(TM) 535", "R5", "R7", "R9"};

		if (match_renderer(renderer, matches)) {
			gpu_set_info_i(GPU_INFO_USE_HQ_NORMALS_WORKAROUND, true);
		}
	}
	/* There is an issue with the #glBlitFramebuffer on MacOS with radeon pro graphics.
	 * Blitting depth with#GL_DEPTH24_STENCIL8 is buggy so the workaround is to use
	 * #GPU_DEPTH32F_STENCIL8. Then Blitting depth will work but blitting stencil will
	 * still be broken. */
	if (GPU_type_matches(GPU_DEVICE_ATI, GPU_OS_MAC, GPU_DRIVER_OFFICIAL)) {
		if (strstr(renderer, "AMD Radeon Pro") || strstr(renderer, "AMD Radeon R9") || strstr(renderer, "AMD Radeon RX")) {
			gpu_set_info_i(GPU_INFO_DEPTH_BLITTING_WORKAROUND, true);
		}
	}
	/* Limit this fix to older hardware with GL < 4.5. This means Broadwell GPUs are
	 * covered since they only support GL 4.4 on windows.
	 * This fixes some issues with workbench anti-aliasing on Win + Intel GPU. (see #76273) */
	if (GPU_type_matches(GPU_DEVICE_INTEL, GPU_OS_WIN, GPU_DRIVER_OFFICIAL) && !(gl_version() >= 45)) {
		GLContext::copy_image_support = false;
	}
	/* Special fix for these specific GPUs.
	 * Without this workaround, rose crashes on startup. (see #72098) */
	if (GPU_type_matches(GPU_DEVICE_INTEL, GPU_OS_WIN, GPU_DRIVER_OFFICIAL) && (strstr(renderer, "HD Graphics 620") || strstr(renderer, "HD Graphics 630"))) {
		gpu_set_info_i(GPU_INFO_MIP_RENDER_WORKAROUND, true);
	}
	/* Intel Ivy Bridge GPU's seems to have buggy cube-map array support. (see #75943) */
	if (GPU_type_matches(GPU_DEVICE_INTEL, GPU_OS_WIN, GPU_DRIVER_OFFICIAL) && (strstr(renderer, "HD Graphics 4000") || strstr(renderer, "HD Graphics 4400") || strstr(renderer, "HD Graphics 2500"))) {
		GLContext::texture_cube_map_array_support = false;
	}
	/* Maybe not all of these drivers have problems with `GL_ARB_base_instance`.
	 * But it's hard to test each case.
	 * We get crashes from some crappy Intel drivers don't work well with shaders created in
	 * different rendering contexts. */
	if (GPU_type_matches(GPU_DEVICE_INTEL, GPU_OS_WIN, GPU_DRIVER_ANY) && (strstr(version, "Build 10.18.10.3") || strstr(version, "Build 10.18.10.4") || strstr(version, "Build 10.18.10.5") || strstr(version, "Build 10.18.14.4") || strstr(version, "Build 10.18.14.5"))) {
		GLContext::base_instance_support = false;
		gpu_set_info_i(GPU_INFO_USE_MAIN_CONTEXT_WORKAROUND, true);
	}
	/* Somehow fixes armature display issues (see #69743). */
	if (GPU_type_matches(GPU_DEVICE_INTEL, GPU_OS_WIN, GPU_DRIVER_ANY) && strstr(version, "Build 20.19.15.4285")) {
		gpu_set_info_i(GPU_INFO_USE_MAIN_CONTEXT_WORKAROUND, true);
	}
	/* See #70187: merging vertices fail. This has been tested from `18.2.2` till `19.3.0~dev`
	 * of the Mesa driver */
	if (GPU_type_matches(GPU_DEVICE_ATI, GPU_OS_UNIX, GPU_DRIVER_OPENSOURCE) && (strstr(version, "Mesa 18.") || strstr(version, "Mesa 19.0") || strstr(version, "Mesa 19.1") || strstr(version, "Mesa 19.2"))) {
		GLContext::unused_fb_slot_workaround = true;
	}
	/* There is a bug on older Nvidia GPU where GL_ARB_texture_gather
	 * is reported to be supported but yield a compile error (see #55802). */
	if (GPU_type_matches(GPU_DEVICE_NVIDIA, GPU_OS_ANY, GPU_DRIVER_ANY) && !(gl_version() >= 40)) {
		GLContext::texture_gather_support = false;
	}

	/* dFdx/dFdy calculation factors, those are dependent on driver. */
	if (GPU_type_matches(GPU_DEVICE_ATI, GPU_OS_ANY, GPU_DRIVER_ANY) && strstr(version, "3.3.10750")) {
		GLContext::derivative_signs[0] = 1.0;
		GLContext::derivative_signs[1] = -1.0;
	}
	else if (GPU_type_matches(GPU_DEVICE_INTEL, GPU_OS_WIN, GPU_DRIVER_ANY)) {
		if (strstr(version, "4.0.0 - Build 10.18.10.3308") || strstr(version, "4.0.0 - Build 9.18.10.3186") || strstr(version, "4.0.0 - Build 9.18.10.3165") || strstr(version, "3.1.0 - Build 9.17.10.3347") || strstr(version, "3.1.0 - Build 9.17.10.4101") || strstr(version, "3.3.0 - Build 8.15.10.2618")) {
			GLContext::derivative_signs[0] = -1.0;
			GLContext::derivative_signs[1] = 1.0;
		}
	}

	/* Disable TF on macOS. */
	if (GPU_type_matches(GPU_DEVICE_ANY, GPU_OS_MAC, GPU_DRIVER_ANY)) {
		gpu_set_info_i(GPU_INFO_TRANSFORM_FEEDBACK_SUPPORT, false);
	}

	/* Some Intel drivers have issues with using mips as frame-buffer targets if
	 * GL_TEXTURE_MAX_LEVEL is higher than the target MIP.
	 * Only check at the end after all other workarounds because this uses the drawing code.
	 * Also after device/driver flags to avoid the check that causes pre GCN Radeon to crash. */
	if (GPU_get_info_i(GPU_INFO_MIP_RENDER_WORKAROUND) == false) {
		gpu_set_info_i(GPU_INFO_MIP_RENDER_WORKAROUND, detect_mip_render_workaround());
	}
	/* Disable multi-draw if the base instance cannot be read. */
	if (GLContext::shader_draw_parameters_support == false) {
		GLContext::multi_draw_indirect_support = false;
	}
	/* Enable our own incomplete debug layer if no other is available. */
	if (GLContext::debug_layer_support == false) {
		GLContext::debug_layer_workaround = true;
	}

	/* Broken glGenerateMipmap on macOS 10.15.7 security update. */
	if (GPU_type_matches(GPU_DEVICE_INTEL, GPU_OS_MAC, GPU_DRIVER_ANY) && strstr(renderer, "HD Graphics 4000")) {
		GLContext::generate_mipmap_workaround = true;
	}

	/* Buggy interface query functions cause crashes when handling SSBOs (#93680) */
	if (GPU_type_matches(GPU_DEVICE_INTEL, GPU_OS_ANY, GPU_DRIVER_ANY) && (strstr(renderer, "HD Graphics 4400") || strstr(renderer, "HD Graphics 4600"))) {
		gpu_set_info_i(GPU_INFO_SHADER_STORAGE_BUFFER_OBJECTS_SUPPORT, false);
	}

	/* Certain Intel/AMD based platforms don't clear the viewport textures. Always clearing leads to
	 * noticeable performance regressions on other platforms as well. */
	if (GPU_type_matches(GPU_DEVICE_ANY, GPU_OS_MAC, GPU_DRIVER_ANY) || GPU_type_matches(GPU_DEVICE_INTEL, GPU_OS_ANY, GPU_DRIVER_ANY)) {
		gpu_set_info_i(GPU_INFO_CLEAR_VIEWPORT_WORKAROUND, true);
	}

	/* Metal-related Workarounds. */

	/* Minimum Per-Vertex stride is 1 byte for OpenGL. */
	gpu_set_info_i(GPU_INFO_MINIMUM_PER_VERTEX_STRIDE, 1);
}

GLint GLContext::max_cubemap_size = 0;
GLint GLContext::max_ubo_binds = 0;
GLint GLContext::max_ubo_size = 0;
GLint GLContext::max_ssbo_binds = 0;
GLint GLContext::max_ssbo_size = 0;

bool GLContext::base_instance_support = false;
bool GLContext::clear_texture_support = false;
bool GLContext::copy_image_support = false;
bool GLContext::debug_layer_support = false;
bool GLContext::direct_state_access_support = false;
bool GLContext::explicit_location_support = false;
bool GLContext::geometry_shader_invocations = false;
bool GLContext::fixed_restart_index_support = false;
bool GLContext::layered_rendering_support = false;
bool GLContext::native_barycentric_support = false;
bool GLContext::multi_bind_support = false;
bool GLContext::multi_bind_image_support = false;
bool GLContext::multi_draw_indirect_support = false;
bool GLContext::shader_draw_parameters_support = false;
bool GLContext::stencil_texturing_support = false;
bool GLContext::framebuffer_fetch_support = false;
bool GLContext::texture_barrier_support = false;
bool GLContext::texture_cube_map_array_support = false;
bool GLContext::texture_filter_anisotropic_support = false;
bool GLContext::texture_gather_support = false;
bool GLContext::texture_storage_support = false;
bool GLContext::vertex_attrib_binding_support = false;

bool GLContext::debug_layer_workaround = false;
bool GLContext::unused_fb_slot_workaround = false;
bool GLContext::generate_mipmap_workaround = false;
float GLContext::derivative_signs[2] = {1.0f, 1.0f};

void GLBackend::capabilities_init() {
	ROSE_assert(gl_version() >= 33);
	/* Common Capabilities. */
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gpu_get_info_i(GPU_INFO_MAX_TEXTURE_SIZE));
	glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &gpu_get_info_i(GPU_INFO_MAX_TEXTURE_LAYERS));
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &gpu_get_info_i(GPU_INFO_MAX_TEXTURES_FRAG));
	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &gpu_get_info_i(GPU_INFO_MAX_TEXTURES_VERT));
	glGetIntegerv(GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, &gpu_get_info_i(GPU_INFO_MAX_TEXTURES_GEOM));
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &gpu_get_info_i(GPU_INFO_MAX_TEXTURES));
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &gpu_get_info_i(GPU_INFO_MAX_UNIFORMS_VERT));
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &gpu_get_info_i(GPU_INFO_MAX_UNIFORMS_FRAG));
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &gpu_get_info_i(GPU_INFO_MAX_BATCH_INDICES));
	glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &gpu_get_info_i(GPU_INFO_MAX_BATCH_VERTICES));
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &gpu_get_info_i(GPU_INFO_MAX_VERTEX_ATTRIBS));
	if (GPU_type_matches(GPU_DEVICE_APPLE, GPU_OS_MAC, GPU_DRIVER_OFFICIAL)) {
		/* Due to a bug, querying GL_MAX_VARYING_FLOATS is emitting GL_INVALID_ENUM.
		 * Force use minimum required value. */
		gpu_set_info_i(GPU_INFO_MAX_VARYING_FLOATS, 32);
	}
	else {
		glGetIntegerv(GL_MAX_VARYING_FLOATS, &gpu_get_info_i(GPU_INFO_MAX_VARYING_FLOATS));
	}

	glGetIntegerv(GL_NUM_EXTENSIONS, &gpu_get_info_i(GPU_INFO_EXTENSIONS_LEN));

	gpu_set_info_i(GPU_INFO_MAX_SAMPLERS, gpu_get_info_i(GPU_INFO_MAX_TEXTURES));
	gpu_set_info_i(GPU_INFO_MEM_STATS_SUPPORT, has_gl_extension("GL_NVX_gpu_memory_info") || has_gl_extension("GL_ATI_meminfo"));
	gpu_set_info_i(GPU_INFO_SHADER_IMAGE_LOAD_STORE_SUPPORT, has_gl_extension("GL_ARB_shader_image_load_store"));
	gpu_set_info_i(GPU_INFO_SHADER_DRAW_PARAMETERS_SUPPORT, has_gl_extension("GL_ARB_shader_draw_parameters"));
	gpu_set_info_i(GPU_INFO_COMPUTE_SHADER_SUPPORT, has_gl_extension("GL_ARB_compute_shader") && gl_version() >= 43);
	gpu_set_info_i(GPU_INFO_GEOMETRY_SHADER_SUPPORT, true);
	gpu_set_info_i(GPU_INFO_MAX_SAMPLERS, gpu_get_info_i(GPU_INFO_MAX_TEXTURES));

	if (gpu_get_info_i(GPU_INFO_COMPUTE_SHADER_SUPPORT)) {
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &gpu_get_info_i(GPU_INFO_MAX_WORK_GROUP_COUNT_X));
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &gpu_get_info_i(GPU_INFO_MAX_WORK_GROUP_COUNT_Y));
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &gpu_get_info_i(GPU_INFO_MAX_WORK_GROUP_COUNT_Z));
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &gpu_get_info_i(GPU_INFO_MAX_WORK_GROUP_SIZE_X));
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &gpu_get_info_i(GPU_INFO_MAX_WORK_GROUP_SIZE_Y));
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &gpu_get_info_i(GPU_INFO_MAX_WORK_GROUP_SIZE_Z));
		glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &gpu_get_info_i(GPU_INFO_MAX_SHADER_STORAGE_BUFFER_BINDINGS));
		glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &gpu_get_info_i(GPU_INFO_MAX_COMPUTE_SHADER_STORAGE_BLOCKS));
	}
	gpu_set_info_i(GPU_INFO_SHADER_STORAGE_BUFFER_OBJECTS_SUPPORT, has_gl_extension("GL_ARB_shader_storage_buffer_object"));
	gpu_set_info_i(GPU_INFO_TRANSFORM_FEEDBACK_SUPPORT, true);

	/* GL specific capabilities. */
	glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &gpu_get_info_i(GPU_INFO_MAX_TEXTURE_3D_SIZE));
	glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &GLContext::max_cubemap_size);
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &GLContext::max_ubo_binds);
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &GLContext::max_ubo_size);
	glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &gpu_get_info_i(GPU_INFO_MAX_STORAGE_BUFFER_SIZE));
	if (gpu_get_info_i(GPU_INFO_SHADER_STORAGE_BUFFER_OBJECTS_SUPPORT)) {
		GLint max_ssbo_binds;
		GLContext::max_ssbo_binds = 999999;
		glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &max_ssbo_binds);
		GLContext::max_ssbo_binds = ROSE_MIN(GLContext::max_ssbo_binds, max_ssbo_binds);
		glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &max_ssbo_binds);
		GLContext::max_ssbo_binds = ROSE_MIN(GLContext::max_ssbo_binds, max_ssbo_binds);
		glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &max_ssbo_binds);
		GLContext::max_ssbo_binds = ROSE_MIN(GLContext::max_ssbo_binds, max_ssbo_binds);
		if (GLContext::max_ssbo_binds < 8) {
			/* Does not meet our minimum requirements. */
			gpu_set_info_i(GPU_INFO_SHADER_STORAGE_BUFFER_OBJECTS_SUPPORT, false);
		}
		glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &GLContext::max_ssbo_size);
	}
	GLContext::base_instance_support = has_gl_extension("GL_ARB_base_instance");
	GLContext::clear_texture_support = has_gl_extension("GL_ARB_clear_texture");
	GLContext::copy_image_support = has_gl_extension("GL_ARB_copy_image");
	GLContext::debug_layer_support = gl_version() >= 43 || has_gl_extension("GL_KHR_debug") || has_gl_extension("GL_ARB_debug_output");
	GLContext::direct_state_access_support = has_gl_extension("GL_ARB_direct_state_access");
	GLContext::explicit_location_support = gl_version() >= 43;
	GLContext::geometry_shader_invocations = has_gl_extension("GL_ARB_gpu_shader5");
	GLContext::fixed_restart_index_support = has_gl_extension("GL_ARB_ES3_compatibility");
	GLContext::framebuffer_fetch_support = has_gl_extension("GL_EXT_shader_framebuffer_fetch");
	GLContext::texture_barrier_support = has_gl_extension("GL_ARB_texture_barrier");
	GLContext::layered_rendering_support = has_gl_extension("GL_AMD_vertex_shader_layer");
	GLContext::native_barycentric_support = has_gl_extension("GL_AMD_shader_explicit_vertex_parameter");
	GLContext::multi_bind_support = has_gl_extension("GL_ARB_multi_bind");
	GLContext::multi_bind_image_support = has_gl_extension("GL_ARB_multi_bind");
	GLContext::multi_draw_indirect_support = has_gl_extension("GL_ARB_multi_draw_indirect");
	GLContext::shader_draw_parameters_support = has_gl_extension("GL_ARB_shader_draw_parameters");
	GLContext::stencil_texturing_support = gl_version() >= 43;
	GLContext::texture_cube_map_array_support = has_gl_extension("GL_ARB_texture_cube_map_array");
	GLContext::texture_filter_anisotropic_support = has_gl_extension("GL_EXT_texture_filter_anisotropic");
	GLContext::texture_gather_support = has_gl_extension("GL_ARB_texture_gather");
	GLContext::texture_storage_support = gl_version() >= 43;
	GLContext::vertex_attrib_binding_support = has_gl_extension("GL_ARB_vertex_attrib_binding");

	detect_workarounds();
}

/* \} */

bool has_gl_extension(const char *name) {
	return glewGetExtension(name);
}

int gl_version(void) {
	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	return static_cast<int>(major) * 10 + static_cast<int>(minor);
}

}  // namespace rose::gpu
