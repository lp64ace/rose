#include "gpu/gpu_info.h"
#include "gpu/gpu_context.h"
#include "gpu/gpu_matrix.h"

#include "gpu_shader_create_info.h"
#include "gpu_shader_create_info_private.h"
#include "gpu_shader_dependency_private.h"
#include "gpu_shader_private.h"
#include "gpu_backend.h"

#include "gpu_context_private.h"

#include "lib/lib_math_vec.h"
#include "lib/lib_math_matrix.h"
#include "lib/lib_math_base.h"

// gpu_shader.c
static bool gpu_shader_srgb_uniform_dirty_get ( );

namespace rose {
namespace gpu {

std::string Shader::DefinesDeclare ( const ShaderCreateInfo &info ) const {
	String defines;
	for ( size_t i = 0; i < info.Defines.Size ( ); i++ ) {
		const auto &def = info.Defines [ i ];
		defines.AppendFormated ( "#define %s %s\n" , def [ 0 ].CStr ( ) , def [ 1 ].CStr ( ) );
	}
	return defines.ToString ( );
}

Shader::Shader ( const char *name ) {
	strncpy_s ( this->mName , sizeof ( this->mName ) , name , sizeof ( this->mName ) );
}

Shader::~Shader ( ) {
	delete this->mInterface;
}

}
}

using namespace rose::gpu;

static void standard_defines ( Vector<const char *> &sources ) {
	assert ( sources.Size ( ) == 0 );
	/* Version needs to be first. Exact values will be added by implementation. */
	sources.Append ( "version" );
	/* Define to identify code usage in shading language. */
	sources.Append ( "#define GPU_SHADER\n" );
	/* some useful defines to detect GPU type */
	if ( GPU_type_matches ( GPU_DEVICE_ATI , GPU_OS_ANY , GPU_DRIVER_ANY ) ) {
		sources.Append ( "#define GPU_ATI\n" );
	} else if ( GPU_type_matches ( GPU_DEVICE_NVIDIA , GPU_OS_ANY , GPU_DRIVER_ANY ) ) {
		sources.Append ( "#define GPU_NVIDIA\n" );
	} else if ( GPU_type_matches ( GPU_DEVICE_INTEL , GPU_OS_ANY , GPU_DRIVER_ANY ) ) {
		sources.Append ( "#define GPU_INTEL\n" );
	}
	/* some useful defines to detect OS type */
	if ( GPU_type_matches ( GPU_DEVICE_ANY , GPU_OS_WIN , GPU_DRIVER_ANY ) ) {
		sources.Append ( "#define OS_WIN\n" );
	} else if ( GPU_type_matches ( GPU_DEVICE_ANY , GPU_OS_MAC , GPU_DRIVER_ANY ) ) {
		sources.Append ( "#define OS_MAC\n" );
	} else if ( GPU_type_matches ( GPU_DEVICE_ANY , GPU_OS_UNIX , GPU_DRIVER_ANY ) ) {
		sources.Append ( "#define OS_UNIX\n" );
	}
	/* API Definition */
	int backend = GPU_backend_get_type ( );
	switch ( backend ) {
		case GPU_BACKEND_OPENGL: {
			sources.Append ( "#define GPU_OPENGL\n" );
		}break;
			break;
	}

	if ( GPU_get_info_i ( GPU_INFO_BROKEN_AMD_DRIVER ) ) {
		sources.Append ( "#define GPU_DEPRECATED_AMD_DRIVER\n" );
	}
}

GPU_Shader *GPU_shader_create_ex ( const char *vertcode ,
				   const char *fragcode ,
				   const char *geomcode ,
				   const char *computecode ,
				   const char *libcode ,
				   const char *defines ,
				   const char *name ) {
	// At least a vertex shader and a fragment shader are required, or only a compute shader.
	assert ( ( ( fragcode != nullptr ) && ( vertcode != nullptr ) && ( computecode == nullptr ) ) ||
		 ( ( fragcode == nullptr ) && ( vertcode == nullptr ) && ( geomcode == nullptr ) &&
		   ( computecode != nullptr ) ) );

	Shader *shader = GPU_Backend::Get ( )->ShaderAlloc ( name );

	if ( vertcode ) {
		Vector<const char *> sources;
		standard_defines ( sources );
		sources.Append ( "#define GPU_VERTEX_SHADER\n" );
		sources.Append ( "#define IN_OUT out\n" );
		if ( geomcode ) {
			sources.Append ( "#define USE_GEOMETRY_SHADER\n" );
		}
		if ( defines ) {
			sources.Append ( defines );
		}
		sources.Append ( vertcode );

		shader->VertexShader ( sources );
	}

	if ( fragcode ) {
		Vector<const char *> sources;
		standard_defines ( sources );
		sources.Append ( "#define GPU_FRAGMENT_SHADER\n" );
		sources.Append ( "#define IN_OUT in\n" );
		if ( geomcode ) {
			sources.Append ( "#define USE_GEOMETRY_SHADER\n" );
		}
		if ( defines ) {
			sources.Append ( defines );
		}
		if ( libcode ) {
			sources.Append ( libcode );
		}
		sources.Append ( fragcode );

		shader->FragmentShader ( sources );
	}

	if ( geomcode ) {
		Vector<const char *> sources;
		standard_defines ( sources );
		sources.Append ( "#define GPU_GEOMETRY_SHADER\n" );
		if ( defines ) {
			sources.Append ( defines );
		}
		sources.Append ( geomcode );

		shader->GeometryShader ( sources );
	}

	if ( computecode ) {
		Vector<const char *> sources;
		standard_defines ( sources );
		sources.Append ( "#define GPU_COMPUTE_SHADER\n" );
		if ( defines ) {
			sources.Append ( defines );
		}
		if ( libcode ) {
			sources.Append ( libcode );
		}
		sources.Append ( computecode );

		shader->ComputeShader ( sources );
	}

	if ( !shader->Finalize ( ) ) {
		delete shader;
		return nullptr;
	}

	return wrap ( shader );
}

void GPU_shader_free ( GPU_Shader *shader ) {
	delete unwrap ( shader );
}

GPU_Shader *GPU_shader_create ( const char *vertcode , const char *fragcode , const char *geomcode , const char *libcode , const char *defines , const char *shname ) {
	return GPU_shader_create_ex ( vertcode , fragcode , geomcode , nullptr , libcode , defines , shname );
}

GPU_Shader *GPU_shader_create_compute ( const char *computecode , const char *libcode , const char *defines , const char *shname ) {
	return GPU_shader_create_ex ( nullptr , nullptr , nullptr , computecode , libcode , defines , shname );
}

const GPU_ShaderCreateInfo *GPU_shader_create_info_get ( const char *info_name ) {
	return gpu_shader_create_info_get ( info_name );
}

bool GPU_shader_create_info_check_error ( const GPU_ShaderCreateInfo *_info , char r_error [ 128 ] ) {
	const ShaderCreateInfo &info = *reinterpret_cast< const ShaderCreateInfo * >( _info );
	std::string error = info.CheckError ( );
	if ( error.length ( ) == 0 ) {
		return true;
	}

	strncpy_s ( r_error , 128 , error.c_str ( ) , 128 );
	return false;
}

GPU_Shader *GPU_shader_create_from_info_name ( const char *info_name ) {
	const GPU_ShaderCreateInfo *_info = gpu_shader_create_info_get ( info_name );
	const ShaderCreateInfo &info = *reinterpret_cast< const ShaderCreateInfo * >( _info );
	if ( !info.DoStaticComplilation ) {
		printf ( "Warning: Trying to compile \"%s\" which was not marked for static compilation.\n" , info.Name.CStr ( ) );
	}
	return GPU_shader_create_from_info ( _info );
}

GPU_Shader *GPU_shader_create_from_info ( const GPU_ShaderCreateInfo *_info ) {
	const ShaderCreateInfo &info = *reinterpret_cast< const ShaderCreateInfo * >( _info );

	const_cast< ShaderCreateInfo & >( info ).Finalize ( );

	const std::string error = info.CheckError ( );
	if ( !error.empty ( ) ) {
		printf ( "%s\n" , error.c_str ( ) );
		return nullptr;
	}

	Shader *shader = GPU_Backend::Get ( )->ShaderAlloc ( info.Name.CStr ( ) );

	std::string defines = shader->DefinesDeclare ( info );
	std::string resources = shader->ResourcesDeclare ( info );

	if ( info.LegacyResourceLocation == false ) {
		defines += "#define USE_GPU_SHADER_CREATE_INFO\n";
	}

	Vector<const char *> typedefs;
	if ( !info.TypedefSources.IsEmpty ( ) || !info.GeneratedTypedefSource.empty ( ) ) {
		typedefs.Append ( gpu_shader_dependency_get_source ( "GPU_shader_shared_utils.h" ).CStr ( ) );
	}
	if ( !info.GeneratedTypedefSource.empty ( ) ) {
		typedefs.Append ( info.GeneratedTypedefSource.c_str ( ) );
	}
	for ( size_t i = 0; i < info.TypedefSources.Size ( ); i++ ) {
		typedefs.Append ( gpu_shader_dependency_get_source ( info.TypedefSources [ i ] ).CStr ( ) );
	}

	if ( !info.VertexSource.IsEmpty ( ) ) {
		auto code = gpu_shader_dependency_get_resolved_source ( info.VertexSource );
		std::string interface = shader->VertexInterfaceDeclare ( info );

		Vector<const char *> sources;
		standard_defines ( sources );
		sources.Append ( "#define GPU_VERTEX_SHADER\n" );
		if ( !info.GeometrySource.IsEmpty ( ) ) {
			sources.Append ( "#define USE_GEOMETRY_SHADER\n" );
		}
		sources.Append ( defines.c_str ( ) );
		sources.Extend ( typedefs );
		sources.Append ( resources.c_str ( ) );
		sources.Append ( interface.c_str ( ) );
		sources.Extend ( code );
		sources.Extend ( info.Dependencies );
		sources.Append ( info.GeneratedVertexSource.c_str ( ) );

		shader->VertexShader ( sources );
	}

	if ( !info.FragmentSource.IsEmpty ( ) ) {
		auto code = gpu_shader_dependency_get_resolved_source ( info.FragmentSource );
		std::string interface = shader->FragmentInterfaceDeclare ( info );

		Vector<const char *> sources;
		standard_defines ( sources );
		sources.Append ( "#define GPU_FRAGMENT_SHADER\n" );
		if ( !info.GeometrySource.IsEmpty ( ) ) {
			sources.Append ( "#define USE_GEOMETRY_SHADER\n" );
		}
		sources.Append ( defines.c_str ( ) );
		sources.Extend ( typedefs );
		sources.Append ( resources.c_str ( ) );
		sources.Append ( interface.c_str ( ) );
		sources.Extend ( code );
		sources.Extend ( info.Dependencies );
		sources.Append ( info.GeneratedFragmentSource.c_str ( ) );

		shader->FragmentShader ( sources );
	}

	if ( !info.GeometrySource.IsEmpty ( ) ) {
		auto code = gpu_shader_dependency_get_resolved_source ( info.GeometrySource );
		std::string layout = shader->GeometryLayoutDeclare ( info );
		std::string interface = shader->GeometryInterfaceDeclare ( info );

		Vector<const char *> sources;
		standard_defines ( sources );
		sources.Append ( "#define GPU_GEOMETRY_SHADER\n" );
		sources.Append ( defines.c_str ( ) );
		sources.Extend ( typedefs );
		sources.Append ( resources.c_str ( ) );
		sources.Append ( layout.c_str ( ) );
		sources.Append ( interface.c_str ( ) );
		sources.Append ( info.GeneratedGeometrySource.c_str ( ) );
		sources.Extend ( code );

		shader->GeometryShader ( sources );
	}

	if ( !info.ComputeSource.IsEmpty ( ) ) {
		auto code = gpu_shader_dependency_get_resolved_source ( info.ComputeSource );
		std::string layout = shader->ComputeLayoutDeclare ( info );

		Vector<const char *> sources;
		standard_defines ( sources );
		sources.Append ( "#define GPU_COMPUTE_SHADER\n" );
		sources.Append ( defines.c_str ( ) );
		sources.Extend ( typedefs );
		sources.Append ( resources.c_str ( ) );
		sources.Append ( layout.c_str ( ) );
		sources.Extend ( code );
		sources.Extend ( info.Dependencies );
		sources.Append ( info.GeneratedComputeSource.c_str ( ) );

		shader->ComputeShader ( sources );
	}

	if ( !shader->Finalize ( &info ) ) {
		delete shader;
		return nullptr;
	}

	return wrap ( shader );
}

// Binding

void GPU_shader_bind ( GPU_Shader *gpu_shader ) {
	Shader *shader = unwrap ( gpu_shader );

	Context *ctx = Context::Get ( );

	if ( ctx->Shader != shader ) {
		ctx->Shader = shader;
		shader->Bind ( );
		GPU_matrix_bind ( gpu_shader );
		GPU_shader_set_srgb_uniform ( gpu_shader );
	} else {
		if ( gpu_shader_srgb_uniform_dirty_get ( ) ) {
		 	GPU_shader_set_srgb_uniform ( gpu_shader );
		}
		if ( GPU_matrix_dirty_get ( ) ) {
			GPU_matrix_bind ( gpu_shader );
		}
	}
}

void GPU_shader_unbind ( ) {
#ifndef NDEBUG
	Context *ctx = Context::Get ( );
	if ( ctx->Shader ) {
		ctx->Shader->Unbind ( );
	}
	ctx->Shader = nullptr;
#endif
}

int GPU_shader_get_program ( GPU_Shader *shader ) {
	return unwrap ( shader )->GetProgramHandle ( );
}

// Shader Name

const char *GPU_shader_get_name ( GPU_Shader *shader ) {
	return unwrap ( shader )->GetName ( );
}

// Uniforms / Resource location

int GPU_shader_get_uniform ( GPU_Shader *shader , const char *name ) {
	ShaderInterface *interface = unwrap ( shader )->mInterface;
	const ShaderInput *uniform = interface->UniformGet ( name );
	return uniform ? uniform->Location : -1;
}

int GPU_shader_get_builtin_uniform ( GPU_Shader *shader , int builtin ) {
	ShaderInterface *interface = unwrap ( shader )->mInterface;
	return interface->UniformBuiltin ( builtin );
}

int GPU_shader_get_builtin_block ( GPU_Shader *shader , int builtin ) {
	ShaderInterface *interface = unwrap ( shader )->mInterface;
	return interface->UboBuiltin ( builtin );
}

int GPU_shader_get_builtin_ssbo ( GPU_Shader *shader , int builtin ) {
	ShaderInterface *interface = unwrap ( shader )->mInterface;
	return interface->ShaderStorageBufferBuiltin ( builtin );
}

int GPU_shader_get_ssbo ( GPU_Shader *shader , const char *name ) {
	ShaderInterface *interface = unwrap ( shader )->mInterface;
	const ShaderInput *ssbo = interface->ShaderStorageBufferGet ( name );
	return ssbo ? ssbo->Location : -1;
}

int GPU_shader_get_uniform_block ( GPU_Shader *shader , const char *name ) {
	ShaderInterface *interface = unwrap ( shader )->mInterface;
	const ShaderInput *ubo = interface->UboGet ( name );
	return ubo ? ubo->Location : -1;
}

int GPU_shader_get_uniform_block_binding ( GPU_Shader *shader , const char *name ) {
	ShaderInterface *interface = unwrap ( shader )->mInterface;
	const ShaderInput *ubo = interface->UboGet ( name );
	return ubo ? ubo->Binding : -1;
}

int GPU_shader_get_texture_binding ( GPU_Shader *shader , const char *name ) {
	ShaderInterface *interface = unwrap ( shader )->mInterface;
	const ShaderInput *tex = interface->UniformGet ( name );
	return tex ? tex->Binding : -1;
}

unsigned int GPU_shader_get_attribute_len ( const GPU_Shader *shader ) {
	ShaderInterface *interface = unwrap ( shader )->mInterface;
	return interface->mAttrLen;
}

int GPU_shader_get_attribute ( GPU_Shader *shader , const char *name ) {
	ShaderInterface *interface = unwrap ( shader )->mInterface;
	const ShaderInput *attr = interface->AttrGet ( name );
	return attr ? attr->Location : -1;
}

bool GPU_shader_get_attribute_info ( const GPU_Shader *shader , int attr_location , char *r_name , int *r_type ) {
	ShaderInterface *interface = unwrap ( shader )->mInterface;

	const ShaderInput *attr = interface->AttrGet ( attr_location );
	if ( !attr ) {
		return false;
	}

	LIB_strncpy ( r_name , interface->InputNameGet ( attr ) , 256 );
	*r_type = attr->Location != -1 ? interface->mAttrTypes [ attr->Location ] : -1;
	return true;
}

// Uniform Setters


void GPU_shader_uniform_vector ( GPU_Shader *shader , int loc , int len , int arraysize , const float *value ) {
	unwrap ( shader )->UniformFloat ( loc , len , arraysize , value );
}

void GPU_shader_uniform_vector_int ( GPU_Shader *shader , int loc , int len , int arraysize , const int *value ) {
	unwrap ( shader )->UniformInt ( loc , len , arraysize , value );
}

void GPU_shader_uniform_int ( GPU_Shader *shader , int location , int value ) {
	GPU_shader_uniform_vector_int ( shader , location , 1 , 1 , &value );
}

void GPU_shader_uniform_float ( GPU_Shader *shader , int location , float value ) {
	GPU_shader_uniform_vector ( shader , location , 1 , 1 , &value );
}

void GPU_shader_uniform_1i ( GPU_Shader *sh , const char *name , int value ) {
	const int loc = GPU_shader_get_uniform ( sh , name );
	GPU_shader_uniform_int ( sh , loc , value );
}

void GPU_shader_uniform_1b ( GPU_Shader *sh , const char *name , bool value ) {
	GPU_shader_uniform_1i ( sh , name , value ? 1 : 0 );
}

void GPU_shader_uniform_2f ( GPU_Shader *sh , const char *name , float x , float y ) {
	const float data [ 2 ] = { x, y };
	GPU_shader_uniform_2fv ( sh , name , data );
}

void GPU_shader_uniform_3f ( GPU_Shader *sh , const char *name , float x , float y , float z ) {
	const float data [ 3 ] = { x, y, z };
	GPU_shader_uniform_3fv ( sh , name , data );
}

void GPU_shader_uniform_4f ( GPU_Shader *sh , const char *name , float x , float y , float z , float w ) {
	const float data [ 4 ] = { x, y, z, w };
	GPU_shader_uniform_4fv ( sh , name , data );
}

void GPU_shader_uniform_1f ( GPU_Shader *sh , const char *name , float value ) {
	const int loc = GPU_shader_get_uniform ( sh , name );
	GPU_shader_uniform_float ( sh , loc , value );
}

void GPU_shader_uniform_2fv ( GPU_Shader *sh , const char *name , const float data [ 2 ] ) {
	const int loc = GPU_shader_get_uniform ( sh , name );
	GPU_shader_uniform_vector ( sh , loc , 2 , 1 , data );
}

void GPU_shader_uniform_3fv ( GPU_Shader *sh , const char *name , const float data [ 3 ] ) {
	const int loc = GPU_shader_get_uniform ( sh , name );
	GPU_shader_uniform_vector ( sh , loc , 3 , 1 , data );
}

void GPU_shader_uniform_4fv ( GPU_Shader *sh , const char *name , const float data [ 4 ] ) {
	const int loc = GPU_shader_get_uniform ( sh , name );
	GPU_shader_uniform_vector ( sh , loc , 4 , 1 , data );
}

void GPU_shader_uniform_2iv ( GPU_Shader *sh , const char *name , const int data [ 2 ] ) {
	const int loc = GPU_shader_get_uniform ( sh , name );
	GPU_shader_uniform_vector_int ( sh , loc , 2 , 1 , data );
}

void GPU_shader_uniform_mat4 ( GPU_Shader *sh , const char *name , const float data [ 4 ][ 4 ] ) {
	const int loc = GPU_shader_get_uniform ( sh , name );
	GPU_shader_uniform_vector ( sh , loc , 16 , 1 , ( const float * ) data );
}

void GPU_shader_uniform_mat3_as_mat4 ( GPU_Shader *sh , const char *name , const float data [ 3 ][ 3 ] ) {
	float matrix [ 4 ][ 4 ];
	copy_m4_m3 ( matrix , data );
	GPU_shader_uniform_mat4 ( sh , name , matrix );
}

void GPU_shader_uniform_2fv_array ( GPU_Shader *sh , const char *name , int len , const float ( *val ) [ 2 ] ) {
	const int loc = GPU_shader_get_uniform ( sh , name );
	GPU_shader_uniform_vector ( sh , loc , 2 , len , ( const float * ) val );
}

void GPU_shader_uniform_4fv_array ( GPU_Shader *sh , const char *name , int len , const float ( *val ) [ 4 ] ) {
	const int loc = GPU_shader_get_uniform ( sh , name );
	GPU_shader_uniform_vector ( sh , loc , 4 , len , ( const float * ) val );
}

// S-RGB

static int g_shader_builtin_srgb_transform = 0;
static bool g_shader_builtin_srgb_is_dirty = false;

static bool gpu_shader_srgb_uniform_dirty_get ( ) {
	return g_shader_builtin_srgb_is_dirty;
}

void GPU_shader_set_srgb_uniform ( GPU_Shader *shader ) {
	int32_t loc = GPU_shader_get_builtin_uniform ( shader , GPU_UNIFORM_SRGB_TRANSFORM );
	if ( loc != -1 ) {
		GPU_shader_uniform_vector_int ( shader , loc , 1 , 1 , &g_shader_builtin_srgb_transform );
	}
	g_shader_builtin_srgb_is_dirty = false;
}

void GPU_shader_set_framebuffer_srgb_target ( int use_srgb_to_linear ) {
	if ( g_shader_builtin_srgb_transform != use_srgb_to_linear ) {
		g_shader_builtin_srgb_transform = use_srgb_to_linear;
		g_shader_builtin_srgb_is_dirty = true;
	}
}
