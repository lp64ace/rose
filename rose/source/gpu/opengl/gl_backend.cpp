#include "gl_backend.h"

#include "gpu/intern/gpu_info_private.h"

#include "lib/lib_vector.h"

#include <gl/glew.h>
#include <stdio.h>
#include <sstream>
#include <string>
#include <cmath>

int get_gl_version ( ) {
	GLint major , minor;
	glGetIntegerv ( GL_MAJOR_VERSION , &major );
	glGetIntegerv ( GL_MINOR_VERSION , &minor );
	return major * 10 + minor;
}

bool has_gl_extension ( const char *name ) {
	GLint n = 0;
	glGetIntegerv ( GL_NUM_EXTENSIONS , &n );
	for ( GLint i = 0; i < n; i++ ) {
		const char *extension = ( const char * ) glGetStringi ( GL_EXTENSIONS , i );
		if ( strcmp ( name , extension ) == 0 ) {
			return true;
		}
	}
	return false;
}

namespace rose {
namespace gpu {

GLBackend::GLBackend ( ) {
	InitPlatform ( );
	InitCapabilities ( );
}

GLBackend::~GLBackend ( ) {
	ExitPlatform ( );
}

void GLBackend::DeleteResources ( ) {

}

void GLBackend::InitPlatform ( ) {
	const char *vendor = ( const char * ) glGetString ( GL_VENDOR );
	const char *renderer = ( const char * ) glGetString ( GL_RENDERER );
	const char *version = ( const char * ) glGetString ( GL_VERSION );
	int device = GPU_DEVICE_ANY;
	int os = GPU_OS_ANY;
	int driver = GPU_DRIVER_ANY;
	int support_level = GPU_SUPPORT_LEVEL_SUPPORTED;

#ifdef _WIN32
	os = GPU_OS_WIN;
#elif defined(__APPLE__)
	os = GPU_OS_MAC;
#else
	os = GPU_OS_UNIX;
#endif

	if ( !vendor ) {
		printf ( "Warning: No OpenGL vendor detected.\n" );
		device = GPU_DEVICE_UNKNOWN;
		driver = GPU_DRIVER_ANY;
	} else if ( strstr ( vendor , "ATI" ) || strstr ( vendor , "AMD" ) ) {
		device = GPU_DEVICE_ATI;
		driver = GPU_DRIVER_OFFICIAL;
	} else if ( strstr ( vendor , "NVIDIA" ) ) {
		device = GPU_DEVICE_NVIDIA;
		driver = GPU_DRIVER_OFFICIAL;
	} else if ( strstr ( vendor , "Intel" ) || strstr ( renderer , "Mesa DRI Intel" ) || strstr ( renderer , "Mesa DRI Mobile Intel" ) ) {
		device = GPU_DEVICE_INTEL;
		driver = GPU_DRIVER_OFFICIAL;

		if ( strstr ( renderer , "UHD Graphics" ) ||
		     // Not UHD but affected by the same bugs.
		     strstr ( renderer , "HD Graphics 530" ) || strstr ( renderer , "Kaby Lake GT2" ) ||
		     strstr ( renderer , "Whiskey Lake" ) ) {
			device |= GPU_DEVICE_INTEL_UHD;
		}
	} else if ( strstr ( renderer , "Mesa DRI R" ) ||
		   ( strstr ( renderer , "Radeon" ) && strstr ( vendor , "X.Org" ) ) ||
		   ( strstr ( renderer , "AMD" ) && strstr ( vendor , "X.Org" ) ) ||
		   ( strstr ( renderer , "Gallium " ) && strstr ( renderer , " on ATI " ) ) ||
		   ( strstr ( renderer , "Gallium " ) && strstr ( renderer , " on AMD " ) ) ) {
		device = GPU_DEVICE_ATI;
		driver = GPU_DRIVER_OPENSOURCE;
	} else if ( strstr ( renderer , "Nouveau" ) || strstr ( vendor , "nouveau" ) ) {
		device = GPU_DEVICE_NVIDIA;
		driver = GPU_DRIVER_OPENSOURCE;
	} else if ( strstr ( vendor , "Mesa" ) ) {
		device = GPU_DEVICE_SOFTWARE;
		driver = GPU_DRIVER_SOFTWARE;
	} else if ( strstr ( vendor , "Microsoft" ) ) {
		device = GPU_DEVICE_SOFTWARE;
		driver = GPU_DRIVER_SOFTWARE;
	} else if ( strstr ( vendor , "Apple" ) ) {
		/* Apple Silicon. */
		device = GPU_DEVICE_APPLE;
		driver = GPU_DRIVER_OFFICIAL;
	} else if ( strstr ( renderer , "Apple Software Renderer" ) ) {
		device = GPU_DEVICE_SOFTWARE;
		driver = GPU_DRIVER_SOFTWARE;
	} else if ( strstr ( renderer , "llvmpipe" ) || strstr ( renderer , "softpipe" ) ) {
		device = GPU_DEVICE_SOFTWARE;
		driver = GPU_DRIVER_SOFTWARE;
	} else {
		printf ( "Warning: Could not find a matching GPU name. Things may not behave as expected.\n" );
		printf ( "Detected OpenGL configuration:\n" );
		printf ( "Vendor: %s\n" , vendor );
		printf ( "Renderer: %s\n" , renderer );
	}

	if ( get_gl_version ( ) < 33 ) {
		support_level = GPU_SUPPORT_LEVEL_UNSUPPORTED;
	} else {
		if ( ( device & GPU_DEVICE_INTEL ) && ( os & GPU_OS_WIN ) ) {
			/* Old Intel drivers with known bugs that cause material properties to crash.
			 * Version Build 10.18.14.5067 is the latest available and appears to be working
			 * ok with our workarounds, so excluded from this list. */
			if ( strstr ( version , "Build 7.14" ) || strstr ( version , "Build 7.15" ) ||
			     strstr ( version , "Build 8.15" ) || strstr ( version , "Build 9.17" ) ||
			     strstr ( version , "Build 9.18" ) || strstr ( version , "Build 10.18.10.3" ) ||
			     strstr ( version , "Build 10.18.10.4" ) || strstr ( version , "Build 10.18.10.5" ) ||
			     strstr ( version , "Build 10.18.14.4" ) ) {
				support_level = GPU_SUPPORT_LEVEL_LIMITED;
			}
		}
		if ( ( device & GPU_DEVICE_ATI ) && ( os & GPU_OS_UNIX ) ) {
			/* Platform seems to work when SB backend is disabled. This can be done
			 * by adding the environment variable `R600_DEBUG=nosb`. */
			if ( strstr ( renderer , "AMD CEDAR" ) ) {
				support_level = GPU_SUPPORT_LEVEL_LIMITED;
			}
		}
	}

	gpu_platform_init ( device , os , driver , support_level , GPU_BACKEND_OPENGL , vendor , renderer , version );
}

void GLBackend::ExitPlatform ( ) {
	gpu_platform_clear ( );
}

static bool detect_mip_render_workaround ( ) {
	int cube_size = 2;
	float clear_color [ 4 ] = { 1.0f, 0.5f, 0.0f, 0.0f };
	float *source_pix = ( float * ) MEM_mallocN ( sizeof ( float [ 4 ] ) * cube_size * cube_size * 6 , __FUNCTION__ );

	/* Not using GPU API since it is not yet fully initialized. */
	GLuint tex , fb;
	/* Create cubemap with 2 mip level. */
	glGenTextures ( 1 , &tex );
	glBindTexture ( GL_TEXTURE_CUBE_MAP , tex );
	for ( int mip = 0; mip < 2; mip++ ) {
		for ( int i = 0; i < 6; i++ ) {
			const int width = cube_size / ( 1 << mip );
			GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
			glTexImage2D ( target , mip , GL_RGBA16F , width , width , 0 , GL_RGBA , GL_FLOAT , source_pix );
		}
	}
	glTexParameteri ( GL_TEXTURE_CUBE_MAP , GL_TEXTURE_BASE_LEVEL , 0 );
	glTexParameteri ( GL_TEXTURE_CUBE_MAP , GL_TEXTURE_MAX_LEVEL , 0 );
	/* Attach and clear mip 1. */
	glGenFramebuffers ( 1 , &fb );
	glBindFramebuffer ( GL_FRAMEBUFFER , fb );
	glFramebufferTexture ( GL_FRAMEBUFFER , GL_COLOR_ATTACHMENT0 , tex , 1 );
	glDrawBuffer ( GL_COLOR_ATTACHMENT0 );
	glClearColor ( clear_color [ 0 ] , clear_color [ 1 ] , clear_color [ 2 ] , clear_color [ 3 ] );
	glColorMask ( GL_TRUE , GL_TRUE , GL_TRUE , GL_TRUE );
	glClear ( GL_COLOR_BUFFER_BIT );
	glBindFramebuffer ( GL_FRAMEBUFFER , 0 );

	/* Read mip 1. If color is not the same as the clear_color, the rendering failed. */
	glGetTexImage ( GL_TEXTURE_CUBE_MAP_POSITIVE_X , 1 , GL_RGBA , GL_FLOAT , source_pix );
	bool enable_workaround = ( memcmp ( clear_color , source_pix , sizeof ( float ) * 4 ) != 0 );
	MEM_freeN ( source_pix );

	glDeleteFramebuffers ( 1 , &fb );
	glDeleteTextures ( 1 , &tex );

	return enable_workaround;
}

static bool match_renderer ( const std::string& renderer , const Vector<std::string> &items ) {
	for ( size_t index = 0; index < items.Size ( ); index++ ) {
		const std::string wrapped = " " + items [ index ] + " ";
		if ( renderer.find ( wrapped ) != std::string::npos ) {
			return true;
		}
	}
	return false;
}

static void detect_workarounds ( ) {
	const char *vendor = ( const char * ) glGetString ( GL_VENDOR );
	const char *renderer = ( const char * ) glGetString ( GL_RENDERER );
	const char *version = ( const char * ) glGetString ( GL_VERSION );

	/* Limit support for GL_ARB_base_instance to OpenGL 4.0 and higher. NVIDIA Quadro FX 4800
	* (TeraScale) report that they support GL_ARB_base_instance, but the driver does not support
	* GLEW_ARB_draw_indirect as it has an OpenGL3 context what also matches the minimum needed
	* requirements.
	*
	* We use it as a target for glMapBuffer(Range) what is part of the OpenGL 4 API. So better
	* disable it when we don't have an OpenGL4 context (See T77657) */
	if ( !( get_gl_version ( ) >= 40 ) ) {
		GLContext::BaseInstanceSupport = false;
	}
	if ( GPU_type_matches ( GPU_DEVICE_ATI , GPU_OS_WIN , GPU_DRIVER_OFFICIAL ) &&
	     ( strstr ( version , "4.5.13399" ) || strstr ( version , "4.5.13417" ) ||
	       strstr ( version , "4.5.13422" ) || strstr ( version , "4.5.13467" ) ) ) {
		/* The renderers include:
		*   Radeon HD 5000;
		*   Radeon HD 7500M;
		*   Radeon HD 7570M;
		*   Radeon HD 7600M;
		*   Radeon R5 Graphics;
		* And others... */
		GLContext::UnusedFbSlotWorkaround = true;
		gpu_set_info_i ( GPU_INFO_MIP_RENDER_WORKAROUND , 1 );
		gpu_set_info_i ( GPU_INFO_SHADER_IMAGE_LOAD_STORE_SUPPORT , 0 );
		gpu_set_info_i ( GPU_INFO_SHADER_DRAW_PARAMETERS_SUPPORT , 0 );
		gpu_set_info_i ( GPU_INFO_BROKEN_AMD_DRIVER , 1 );
	}
	/* Compute shaders have some issues with those versions (see T94936). */
	if ( GPU_type_matches ( GPU_DEVICE_ATI , GPU_OS_ANY , GPU_DRIVER_OFFICIAL ) &&
	     ( strstr ( version , "4.5.14831" ) || strstr ( version , "4.5.14760" ) ) ) {
		gpu_set_info_i ( GPU_INFO_COMPUTE_SHADER_SUPPORT , 0 );
	}
	/* We have issues with this specific renderer. (see T74024) */
	if ( GPU_type_matches ( GPU_DEVICE_ATI , GPU_OS_UNIX , GPU_DRIVER_OPENSOURCE ) &&
	     ( strstr ( renderer , "AMD VERDE" ) || strstr ( renderer , "AMD KAVERI" ) ||
	       strstr ( renderer , "AMD TAHITI" ) ) ) {
		GLContext::UnusedFbSlotWorkaround = true;
		gpu_set_info_i ( GPU_INFO_SHADER_IMAGE_LOAD_STORE_SUPPORT , 0 );
		gpu_set_info_i ( GPU_INFO_SHADER_DRAW_PARAMETERS_SUPPORT , 0 );
		gpu_set_info_i ( GPU_INFO_BROKEN_AMD_DRIVER , 1 );
	}
	/* Fix slowdown on this particular driver. (see T77641) */
	if ( GPU_type_matches ( GPU_DEVICE_ATI , GPU_OS_UNIX , GPU_DRIVER_OPENSOURCE ) &&
	     strstr ( version , "Mesa 19.3.4" ) ) {
		gpu_set_info_i ( GPU_INFO_SHADER_IMAGE_LOAD_STORE_SUPPORT , 0 );
		gpu_set_info_i ( GPU_INFO_SHADER_DRAW_PARAMETERS_SUPPORT , 0 );
		gpu_set_info_i ( GPU_INFO_BROKEN_AMD_DRIVER , 1 );
	}
	/* See T82856: AMD drivers since 20.11 running on a polaris architecture doesn't support the
	 * `GL_INT_2_10_10_10_REV` data type correctly. This data type is used to pack normals and flags.
	 * The work around uses `GPU_RGBA16I`. In 22.?.? drivers this has been fixed for
	 * polaris platform. Keeping legacy platforms around just in case.
	 */
	if ( GPU_type_matches ( GPU_DEVICE_ATI , GPU_OS_ANY , GPU_DRIVER_OFFICIAL ) ) {
		const Vector<std::string> matches = { "RX550/550", "(TM) 520", "(TM) 530", "(TM) 535", "R5", "R7", "R9" };

		if ( match_renderer ( renderer , matches ) ) {
			gpu_set_info_i ( GPU_INFO_USE_HQ_NORMALS_WORKAROUND , 1 );
		}
	}
	/* There is an issue with the #glBlitFramebuffer on MacOS with radeon pro graphics.
	 * Blitting depth with#GL_DEPTH24_STENCIL8 is buggy so the workaround is to use
	 * #GPU_DEPTH32F_STENCIL8. Then Blitting depth will work but blitting stencil will
	 * still be broken. */
	if ( GPU_type_matches ( GPU_DEVICE_ATI , GPU_OS_MAC , GPU_DRIVER_OFFICIAL ) ) {
		if ( strstr ( renderer , "AMD Radeon Pro" ) || strstr ( renderer , "AMD Radeon R9" ) ||
		     strstr ( renderer , "AMD Radeon RX" ) ) {
			gpu_set_info_i ( GPU_INFO_DEPTH_BLITTING_WORKAROUND , 1 );
		}
	}
	/* Limit this fix to older hardware with GL < 4.5. This means Broadwell GPUs are
	 * covered since they only support GL 4.4 on windows.
	 * This fixes some issues with workbench anti-aliasing on Win + Intel GPU. (see T76273) */
	if ( GPU_type_matches ( GPU_DEVICE_INTEL , GPU_OS_WIN , GPU_DRIVER_OFFICIAL ) &&
	     !( get_gl_version ( ) >= 45 ) ) {
		GLContext::CopyImageSupport = false;
	}
	/* Special fix for these specific GPUs.
	 * Without this workaround, blender crashes on startup. (see T72098) */
	if ( GPU_type_matches ( GPU_DEVICE_INTEL , GPU_OS_WIN , GPU_DRIVER_OFFICIAL ) &&
	     ( strstr ( renderer , "HD Graphics 620" ) || strstr ( renderer , "HD Graphics 630" ) ) ) {
		gpu_set_info_i ( GPU_INFO_MIP_RENDER_WORKAROUND , 1 );
	}
	/* Intel Ivy Bridge GPU's seems to have buggy cube-map array support. (see T75943) */
	if ( GPU_type_matches ( GPU_DEVICE_INTEL , GPU_OS_WIN , GPU_DRIVER_OFFICIAL ) &&
	     ( strstr ( renderer , "HD Graphics 4000" ) || strstr ( renderer , "HD Graphics 4400" ) ||
	       strstr ( renderer , "HD Graphics 2500" ) ) ) {
		GLContext::TextureCubeMapArraySupport = false;
	}
	/* Maybe not all of these drivers have problems with `GL_ARB_base_instance`.
	 * But it's hard to test each case.
	 * We get crashes from some crappy Intel drivers don't work well with shaders created in
	 * different rendering contexts. */
	if ( GPU_type_matches ( GPU_DEVICE_INTEL , GPU_OS_WIN , GPU_DRIVER_ANY ) &&
	     ( strstr ( version , "Build 10.18.10.3" ) || strstr ( version , "Build 10.18.10.4" ) ||
	       strstr ( version , "Build 10.18.10.5" ) || strstr ( version , "Build 10.18.14.4" ) ||
	       strstr ( version , "Build 10.18.14.5" ) ) ) {
		GLContext::BaseInstanceSupport = false;
		gpu_set_info_i ( GPU_INFO_USE_MAIN_CONTEXT_WORKAROUND , 1 );
	}
	/* Somehow fixes armature display issues (see T69743). */
	if ( GPU_type_matches ( GPU_DEVICE_INTEL , GPU_OS_WIN , GPU_DRIVER_ANY ) &&
	     ( strstr ( version , "Build 20.19.15.4285" ) ) ) {
		gpu_set_info_i ( GPU_INFO_USE_MAIN_CONTEXT_WORKAROUND , 1 );
	}
	/* See T70187: merging vertices fail. This has been tested from `18.2.2` till `19.3.0~dev`
	 * of the Mesa driver */
	if ( GPU_type_matches ( GPU_DEVICE_ATI , GPU_OS_UNIX , GPU_DRIVER_OPENSOURCE ) &&
	     ( strstr ( version , "Mesa 18." ) || strstr ( version , "Mesa 19.0" ) ||
	       strstr ( version , "Mesa 19.1" ) || strstr ( version , "Mesa 19.2" ) ) ) {
		GLContext::UnusedFbSlotWorkaround = true;
	}
	/* There is a bug on older Nvidia GPU where GL_ARB_texture_gather
	 * is reported to be supported but yield a compile error (see T55802). */
	if ( GPU_type_matches ( GPU_DEVICE_NVIDIA , GPU_OS_ANY , GPU_DRIVER_ANY ) &&
	     !( get_gl_version ( ) >= 40 ) ) {
		GLContext::TextureGatherSupport = false;
	}

	/* dFdx/dFdy calculation factors, those are dependent on driver. */
	if ( GPU_type_matches ( GPU_DEVICE_ATI , GPU_OS_ANY , GPU_DRIVER_ANY ) &&
	     strstr ( version , "3.3.10750" ) ) {
		GLContext::DerivativeSigns [ 0 ] = 1.0;
		GLContext::DerivativeSigns [ 1 ] = -1.0;
	} else if ( GPU_type_matches ( GPU_DEVICE_INTEL , GPU_OS_WIN , GPU_DRIVER_ANY ) ) {
		if ( strstr ( version , "4.0.0 - Build 10.18.10.3308" ) ||
		     strstr ( version , "4.0.0 - Build 9.18.10.3186" ) ||
		     strstr ( version , "4.0.0 - Build 9.18.10.3165" ) ||
		     strstr ( version , "3.1.0 - Build 9.17.10.3347" ) ||
		     strstr ( version , "3.1.0 - Build 9.17.10.4101" ) ||
		     strstr ( version , "3.3.0 - Build 8.15.10.2618" ) ) {
			GLContext::DerivativeSigns [ 0 ] = -1.0;
			GLContext::DerivativeSigns [ 1 ] = 1.0;
		}
	}

	/* Disable TF on macOS. */
	if ( GPU_type_matches ( GPU_DEVICE_ANY , GPU_OS_MAC , GPU_DRIVER_ANY ) ) {
		gpu_set_info_i ( GPU_INFO_TRANSFORM_FEEDBACK_SUPPORT , 0 );
	}

	/* Some Intel drivers have issues with using mips as frame-buffer targets if
	 * GL_TEXTURE_MAX_LEVEL is higher than the target MIP.
	 * Only check at the end after all other workarounds because this uses the drawing code.
	 * Also after device/driver flags to avoid the check that causes pre GCN Radeon to crash. */
	if ( !gpu_get_info_i(GPU_INFO_MIP_RENDER_WORKAROUND) ) {
		gpu_set_info_i ( GPU_INFO_MIP_RENDER_WORKAROUND , detect_mip_render_workaround ( ) );
	}
	/* Disable multi-draw if the base instance cannot be read. */
	if ( GLContext::ShaderDrawParametersSupport == false ) {
		GLContext::MultiDrawIndirectSupport = false;
	}
	/* Enable our own incomplete debug layer if no other is available. */
	if ( GLContext::DebugLayerSupport == false ) {
		GLContext::DebugLayerWorkaround = true;
	}

	/* Broken glGenerateMipmap on macOS 10.15.7 security update. */
	if ( GPU_type_matches ( GPU_DEVICE_INTEL , GPU_OS_MAC , GPU_DRIVER_ANY ) &&
	     strstr ( renderer , "HD Graphics 4000" ) ) {
		GLContext::GenerateMipMapWorkaround = true;
	}

	/* Buggy interface query functions cause crashes when handling SSBOs (T93680) */
	if ( GPU_type_matches ( GPU_DEVICE_INTEL , GPU_OS_ANY , GPU_DRIVER_ANY ) &&
	     ( strstr ( renderer , "HD Graphics 4400" ) || strstr ( renderer , "HD Graphics 4600" ) ) ) {
		gpu_set_info_i ( GPU_INFO_SHADER_STORAGE_BUFFER_OBJECTS_SUPPORT , 0 );
	}

	/* Certain Intel/AMD based platforms don't clear the viewport textures. Always clearing leads to
	 * noticeable performance regressions on other platforms as well. */
	if ( GPU_type_matches ( GPU_DEVICE_ANY , GPU_OS_MAC , GPU_DRIVER_ANY ) ||
	     GPU_type_matches ( GPU_DEVICE_INTEL , GPU_OS_ANY , GPU_DRIVER_ANY ) ) {
		gpu_set_info_i ( GPU_INFO_CLEAR_VIEWPORT_WORKAROUND , 1 );
	}

	gpu_set_info_i ( GPU_INFO_MINIMUM_PER_VERTEX_STRIDE , 1 );
}

// Capabilities.

int GLContext::MaxCubemabSize = 0;
int GLContext::MaxUboSize = 0;
int GLContext::MaxUboBinds = 0;
int GLContext::MaxSSBOSize = 0;
int GLContext::MaxSSBOBinds = 0;

// Extensions.

bool GLContext::BaseInstanceSupport = false;
bool GLContext::ClearTextureSupport = false;
bool GLContext::CopyImageSupport = false;
bool GLContext::DebugLayerSupport = false;
bool GLContext::DirectStateAccessSupport = false;
bool GLContext::ExplicitLocationSupport = false;
bool GLContext::GeometryShaderInvocations = false;
bool GLContext::FixedRestartIndexSupport = false;
bool GLContext::LayeredRenderingSupport = false;
bool GLContext::NativeBarycentricSupport = false;
bool GLContext::MultiBindSupport = false;
bool GLContext::MultiDrawIndirectSupport = false;
bool GLContext::ShaderDrawParametersSupport = false;
bool GLContext::StencilTexturingSupport = false;
bool GLContext::TextureCubeMapArraySupport = false;
bool GLContext::TextureFilterAnisotropicSupport = false;
bool GLContext::TextureGatherSupport = false;
bool GLContext::TextureStorageSupport = false;
bool GLContext::VertexAttribBindingSupport = false;

// Workarounds.

bool GLContext::DebugLayerWorkaround = false;
bool GLContext::UnusedFbSlotWorkaround = false;
bool GLContext::GenerateMipMapWorkaround = false;
float GLContext::DerivativeSigns [ 2 ] = { 1.0f , 1.0f };

void GLBackend::ComputeDispatch ( int groups_x_len , int groups_y_len , int groups_z_len ) {
	GLCompute::Dipsatch ( groups_x_len , groups_y_len , groups_z_len );
}

void GLBackend::InitCapabilities ( ) {
	/* Common Capabilities. */
	glGetIntegerv ( GL_MAX_TEXTURE_SIZE , &gpu_get_info_i ( GPU_INFO_MAX_TEXTURE_SIZE ) );
	glGetIntegerv ( GL_MAX_ARRAY_TEXTURE_LAYERS , &gpu_get_info_i ( GPU_INFO_MAX_TEXTURE_LAYERS ) );
	glGetIntegerv ( GL_MAX_TEXTURE_IMAGE_UNITS , &gpu_get_info_i( GPU_INFO_MAX_FRAG_TEXTURES ) );
	glGetIntegerv ( GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS , &gpu_get_info_i ( GPU_INFO_MAX_VERT_TEXTURES ) );
	glGetIntegerv ( GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS , &gpu_get_info_i ( GPU_INFO_MAX_GEOM_TEXTURES ) );
	glGetIntegerv ( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS , &gpu_get_info_i ( GPU_INFO_MAX_TEXTURES ) );
	glGetIntegerv ( GL_MAX_VERTEX_UNIFORM_COMPONENTS , &gpu_get_info_i ( GPU_INFO_MAX_UNIFORMS_VERT ) );
	glGetIntegerv ( GL_MAX_FRAGMENT_UNIFORM_COMPONENTS , &gpu_get_info_i ( GPU_INFO_MAX_UNIFORMS_FRAG ) );
	glGetIntegerv ( GL_MAX_ELEMENTS_INDICES , &gpu_get_info_i ( GPU_INFO_MAX_BATCH_INDICES ) );
	glGetIntegerv ( GL_MAX_ELEMENTS_VERTICES , &gpu_get_info_i ( GPU_INFO_MAX_BATCH_VERTICES ) );
	glGetIntegerv ( GL_MAX_VERTEX_ATTRIBS , &gpu_get_info_i ( GPU_INFO_MAX_VERTEX_ATTRIBS ) );
	if ( GPU_type_matches ( GPU_DEVICE_APPLE , GPU_OS_MAC , GPU_DRIVER_OFFICIAL ) ) { 
		/* Due to a bug, querying GL_MAX_VARYING_FLOATS is emitting GL_INVALID_ENUM.
		 * Force use minimum required value. */
		gpu_set_info_i ( GPU_INFO_MAX_VARYING_FLOATS , 32 );
	} else {
		glGetIntegerv ( GL_MAX_VARYING_FLOATS , &gpu_get_info_i ( GPU_INFO_MAX_VARYING_FLOATS ) );
	}

	glGetIntegerv ( GL_NUM_EXTENSIONS , &gpu_get_info_i ( GPU_INFO_NUM_EXTENSIONS ) );

	gpu_set_info_i ( GPU_INFO_MAX_SAMPLERS , gpu_get_info_i ( GPU_INFO_MAX_TEXTURES ) );

	gpu_set_info_i ( GPU_INFO_MEM_STATS_SUPPORT ,
			 has_gl_extension ( "GL_NVX_gpu_memory_info" ) ||
			 has_gl_extension ( "GL_ATI_meminfo" ) );

	gpu_set_info_i ( GPU_INFO_SHADER_IMAGE_LOAD_STORE_SUPPORT , has_gl_extension ( "GL_ARB_shader_image_load_store" ) );
	gpu_set_info_i ( GPU_INFO_SHADER_DRAW_PARAMETERS_SUPPORT , has_gl_extension ( "GL_ARB_shader_draw_parameters" ) );
	gpu_set_info_i ( GPU_INFO_COMPUTE_SHADER_SUPPORT , has_gl_extension ( "GL_ARB_compute_shader" ) && get_gl_version ( ) >= 43 );

	if ( gpu_get_info_i ( GPU_INFO_COMPUTE_SHADER_SUPPORT ) ) {
		glGetIntegeri_v ( GL_MAX_COMPUTE_WORK_GROUP_COUNT , 0 , &gpu_get_info_i ( GPU_INFO_MAX_WORK_GROUPS_X ) );
		glGetIntegeri_v ( GL_MAX_COMPUTE_WORK_GROUP_COUNT , 1 , &gpu_get_info_i ( GPU_INFO_MAX_WORK_GROUPS_Y ) );
		glGetIntegeri_v ( GL_MAX_COMPUTE_WORK_GROUP_COUNT , 2 , &gpu_get_info_i ( GPU_INFO_MAX_WORK_GROUPS_Z ) );
		glGetIntegeri_v ( GL_MAX_COMPUTE_WORK_GROUP_SIZE , 0 , &gpu_get_info_i ( GPU_INFO_MAX_WORK_GROUP_SIZE_X ) );
		glGetIntegeri_v ( GL_MAX_COMPUTE_WORK_GROUP_SIZE , 1 , &gpu_get_info_i ( GPU_INFO_MAX_WORK_GROUP_SIZE_Y ) );
		glGetIntegeri_v ( GL_MAX_COMPUTE_WORK_GROUP_SIZE , 2 , &gpu_get_info_i ( GPU_INFO_MAX_WORK_GROUP_SIZE_Z ) );
		glGetIntegerv ( GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS , &gpu_get_info_i ( GPU_INFO_MAX_SHADER_STORAGE_BUFFER_BINDINGS ) );
		glGetIntegerv ( GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS , &gpu_get_info_i ( GPU_INFO_MAX_COMPUTE_SHADER_STORAGE_BLOCKS ) );
	}

	gpu_set_info_i ( GPU_INFO_SHADER_STORAGE_BUFFER_OBJECTS_SUPPORT , has_gl_extension ( "GL_ARB_shader_storage_buffer_object" ) );
	gpu_set_info_i ( GPU_INFO_TRANSFORM_FEEDBACK_SUPPORT , 1 );

	/* GL specific capabilities. */
	glGetIntegerv ( GL_MAX_3D_TEXTURE_SIZE , &gpu_get_info_i ( GPU_INFO_MAX_3D_TEXTURE_SIZE ) );
	glGetIntegerv ( GL_MAX_CUBE_MAP_TEXTURE_SIZE , &gpu_get_info_i ( GPU_INFO_MAX_CUBEMAP_SIZE ) );
	glGetIntegerv ( GL_MAX_FRAGMENT_UNIFORM_BLOCKS , &GLContext::MaxUboBinds );
	glGetIntegerv ( GL_MAX_UNIFORM_BLOCK_SIZE , &GLContext::MaxUboSize );
	if ( gpu_get_info_i ( GPU_INFO_SHADER_STORAGE_BUFFER_OBJECTS_SUPPORT ) ) {
		GLint max_ssbo_binds;
		GLContext::MaxSSBOSize = 999999;
		glGetIntegerv ( GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS , &max_ssbo_binds );
		GLContext::MaxSSBOBinds = ROSE_MIN ( GLContext::MaxSSBOBinds , max_ssbo_binds );
		glGetIntegerv ( GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS , &max_ssbo_binds );
		GLContext::MaxSSBOBinds = ROSE_MIN ( GLContext::MaxSSBOBinds , max_ssbo_binds );
		glGetIntegerv ( GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS , &max_ssbo_binds );
		GLContext::MaxSSBOBinds = ROSE_MIN ( GLContext::MaxSSBOBinds , max_ssbo_binds );
		if ( GLContext::MaxSSBOBinds < 8 ) {
			/* Does not meet our minimum requirements. */
			gpu_set_info_i ( GPU_INFO_SHADER_STORAGE_BUFFER_OBJECTS_SUPPORT , 0 );
		}
		glGetIntegerv ( GL_MAX_SHADER_STORAGE_BLOCK_SIZE , &GLContext::MaxSSBOSize );
	}
	GLContext::BaseInstanceSupport = has_gl_extension ( "GL_ARB_base_instance" );
	GLContext::ClearTextureSupport = has_gl_extension ( "GL_ARB_clear_texture" );
	GLContext::CopyImageSupport = has_gl_extension ( "GL_ARB_copy_image" );
	GLContext::DebugLayerSupport = get_gl_version ( ) >= 43 || has_gl_extension ( "GL_KHR_debug" ) || has_gl_extension ( "GL_ARB_debug_output" );
	GLContext::DirectStateAccessSupport = has_gl_extension ( "GL_ARB_direct_state_access" );
	GLContext::ExplicitLocationSupport = get_gl_version ( ) >= 43;
	GLContext::GeometryShaderInvocations = has_gl_extension ( "GL_ARB_gpu_shader5" );
	GLContext::FixedRestartIndexSupport = has_gl_extension ( "GL_ARB_ES3_compatibility" );
	GLContext::LayeredRenderingSupport = has_gl_extension ( "GL_AMD_vertex_shader_layer" );
	GLContext::NativeBarycentricSupport = has_gl_extension ( "GL_AMD_shader_explicit_vertex_parameter" );
	GLContext::MultiBindSupport = has_gl_extension ( "GL_ARB_multi_bind" );
	GLContext::MultiDrawIndirectSupport = has_gl_extension ( "GL_ARB_multi_draw_indirect" );
	GLContext::ShaderDrawParametersSupport = has_gl_extension ( "GL_ARB_shader_draw_parameters" );
	GLContext::StencilTexturingSupport = get_gl_version ( ) >= 43;
	GLContext::TextureCubeMapArraySupport = has_gl_extension ( "GL_ARB_texture_cube_map_array" );
	GLContext::TextureFilterAnisotropicSupport = has_gl_extension ( "GL_EXT_texture_filter_anisotropic" );
	GLContext::TextureGatherSupport = has_gl_extension ( "GL_ARB_texture_gather" );
	GLContext::TextureStorageSupport = get_gl_version ( ) >= 43;
	GLContext::VertexAttribBindingSupport = has_gl_extension ( "GL_ARB_vertex_attrib_binding" );

	detect_workarounds ( );
}

}
}