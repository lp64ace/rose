#include "gl_shader.h"
#include "gl_backend.h"
#include "gl_context.h"

#include <gl/glew.h>

#include <sstream>
#include <iostream>
#include <string>

namespace rose {
namespace gpu {

#define STR_CONCAT(dst, len, suffix)    len += sprintf_s(dst + len,(sizeof(dst)/sizeof(dst[0])) - len, suffix)

static char *glsl_patch_default_get ( ) {
	/** Used for shader patching. Init once. */
	static char patch [ 2048 ] = "\0";
	if ( patch [ 0 ] != '\0' ) {
		return patch;
	}

	size_t slen = 0;
	/* Version need to go first. */
	if ( get_gl_version ( ) >= 43 ) {
		STR_CONCAT ( patch , slen , "#version 430\n" );
	} else {
		STR_CONCAT ( patch , slen , "#version 330\n" );
	}

	/* Enable extensions for features that are not part of our base GLSL version
	 * don't use an extension for something already available! */
	if ( GLContext::TextureGatherSupport ) {
		STR_CONCAT ( patch , slen , "#extension GL_ARB_texture_gather: enable\n" );
		/* Some drivers don't agree on epoxy_has_gl_extension("GL_ARB_texture_gather") and the actual
		 * support in the shader so double check the preprocessor define (see T56544). */
		STR_CONCAT ( patch , slen , "#ifdef GL_ARB_texture_gather\n" );
		STR_CONCAT ( patch , slen , "#  define GPU_ARB_texture_gather\n" );
		STR_CONCAT ( patch , slen , "#endif\n" );
	}
	if ( GLContext::ShaderDrawParametersSupport ) {
		STR_CONCAT ( patch , slen , "#extension GL_ARB_shader_draw_parameters : enable\n" );
		STR_CONCAT ( patch , slen , "#define GPU_ARB_shader_draw_parameters\n" );
		STR_CONCAT ( patch , slen , "#define gpu_BaseInstance gl_BaseInstanceARB\n" );
	}
	if ( GLContext::GeometryShaderInvocations ) {
		STR_CONCAT ( patch , slen , "#extension GL_ARB_gpu_shader5 : enable\n" );
		STR_CONCAT ( patch , slen , "#define GPU_ARB_gpu_shader5\n" );
	}
	if ( GLContext::TextureCubeMapArraySupport ) {
		STR_CONCAT ( patch , slen , "#extension GL_ARB_texture_cube_map_array : enable\n" );
		STR_CONCAT ( patch , slen , "#define GPU_ARB_texture_cube_map_array\n" );
	}
	if ( has_gl_extension ( "GL_ARB_conservative_depth" ) ) {
		STR_CONCAT ( patch , slen , "#extension GL_ARB_conservative_depth : enable\n" );
	}
	if ( GPU_get_info_i ( GPU_INFO_SHADER_IMAGE_LOAD_STORE_SUPPORT ) ) {
		STR_CONCAT ( patch , slen , "#extension GL_ARB_shader_image_load_store: enable\n" );
		STR_CONCAT ( patch , slen , "#extension GL_ARB_shading_language_420pack: enable\n" );
	}
	if ( GLContext::LayeredRenderingSupport ) {
		STR_CONCAT ( patch , slen , "#extension GL_AMD_vertex_shader_layer: enable\n" );
		STR_CONCAT ( patch , slen , "#define gpu_Layer gl_Layer\n" );
	}
	if ( GLContext::NativeBarycentricSupport ) {
		STR_CONCAT ( patch , slen , "#extension GL_AMD_shader_explicit_vertex_parameter: enable\n" );
	}

	/* Fallbacks. */
	if ( !GLContext::ShaderDrawParametersSupport ) {
		STR_CONCAT ( patch , slen , "uniform int gpu_BaseInstance;\n" );
	}

	/* Vulkan GLSL compat. */
	STR_CONCAT ( patch , slen , "#define gpu_InstanceIndex (gl_InstanceID + gpu_BaseInstance)\n" );

	/* Array compat. */
	STR_CONCAT ( patch , slen , "#define gpu_Array(_type) _type[]\n" );

	/* Derivative sign can change depending on implementation. */
	STR_CONCAT ( patch , slen , "#define DFDX_SIGN %1.1f\n" , GLContext::DerivativeSigns [ 0 ] );
	STR_CONCAT ( patch , slen , "#define DFDY_SIGN %1.1f\n" , GLContext::DerivativeSigns [ 1 ] );

	assert ( slen < sizeof ( patch ) );
	return patch;
}

static char *glsl_patch_compute_get ( ) {
	/** Used for shader patching. Init once. */
	static char patch [ 2048 ] = "\0";
	if ( patch [ 0 ] != '\0' ) {
		return patch;
	}

	size_t slen = 0;
	/* Version need to go first. */
	STR_CONCAT ( patch , slen , "#version 430\n" );
	STR_CONCAT ( patch , slen , "#extension GL_ARB_compute_shader :enable\n" );

	/* Array compat. */
	STR_CONCAT ( patch , slen , "#define gpu_Array(_type) _type[]\n" );

	assert ( slen < sizeof ( patch ) );
	return patch;
}

static const char *interpolation_to_string ( const int &interp ) {
	switch ( interp ) {
		case GPU_INTERPOLATION_SMOOTH: return "smooth";
		case GPU_INTERPOLATION_FLAT: return "flat";
		case GPU_INTERPOLATION_NO_PERSPECTIVE: return "noperspective";
		default: return "unknown";
	}
}

static const char *type_to_string ( const Type &type ) {
	switch ( type ) {
		case Type::FLOAT: return "float";
		case Type::VEC2: return "vec2";
		case Type::VEC3: return "vec3";
		case Type::VEC4: return "vec4";
		case Type::MAT3: return "mat3";
		case Type::MAT4: return "mat4";
		case Type::UINT: return "uint";
		case Type::UVEC2: return "uvec2";
		case Type::UVEC3: return "uvec3";
		case Type::UVEC4: return "uvec4";
		case Type::INT: return "int";
		case Type::IVEC2: return "ivec2";
		case Type::IVEC3: return "ivec3";
		case Type::IVEC4: return "ivec4";
		default: return "unknown";
	}
}

static const char *texture_format_to_string ( const int &type ) {
	switch ( type ) {
		case GPU_RGBA8UI: return "rgba8ui";
		case GPU_RGBA8I: return "rgba8i";
		case GPU_RGBA8: return "rgba8";
		case GPU_RGBA32UI: return "rgba32ui";
		case GPU_RGBA32I: return "rgba32i";
		case GPU_RGBA32F: return "rgba32f";
		case GPU_RGBA16UI: return "rgba16ui";
		case GPU_RGBA16I: return "rgba16i";
		case GPU_RGBA16F: return "rgba16f";
		case GPU_RGBA16: return "rgba16";
		case GPU_RG8UI: return "rg8ui";
		case GPU_RG8I: return "rg8i";
		case GPU_RG8: return "rg8";
		case GPU_RG32UI: return "rg32ui";
		case GPU_RG32I: return "rg32i";
		case GPU_RG32F: return "rg32f";
		case GPU_RG16UI: return "rg16ui";
		case GPU_RG16I: return "rg16i";
		case GPU_RG16F: return "rg16f";
		case GPU_RG16: return "rg16";
		case GPU_R8UI: return "r8ui";
		case GPU_R8I: return "r8i";
		case GPU_R8: return "r8";
		case GPU_R32UI: return "r32ui";
		case GPU_R32I: return "r32i";
		case GPU_R32F: return "r32f";
		case GPU_R16UI: return "r16ui";
		case GPU_R16I: return "r16i";
		case GPU_R16F: return "r16f";
		case GPU_R16: return "r16";
		case GPU_R11F_G11F_B10F: return "r11f_g11f_b10f";
		case GPU_RGB10_A2: return "rgb10_a2";
		default: return "unknown";
	}
}

static const char *primitive_in_to_string ( const int &layout ) {
	switch ( layout ) {
		case GPU_PRIMITIVE_IN_POINTS: return "points";
		case GPU_PRIMITIVE_IN_LINES: return "lines";
		case GPU_PRIMITIVE_IN_LINES_ADJ: return "lines_adjacency";
		case GPU_PRIMITIVE_IN_TRIANGLES: return "triangles";
		case GPU_PRIMITIVE_IN_TRIANGLES_ADJ: return "triangles_adjacency";
		default: return "unknown";
	}
}

static const char *primitive_out_to_string ( const int &layout ) {
	switch ( layout ) {
		case GPU_PRIMITIVE_OUT_POINTS: return "points";
		case GPU_PRIMITIVE_OUT_LINE_STRIP: return "line_strip";
		case GPU_PRIMITIVE_OUT_TRIANGLE_STRIP: return "triangle_strip";
		case GPU_PRIMITIVE_OUT_TRIANGLES: return "triangles";
		case GPU_PRIMITIVE_OUT_LINES: return "lines";
		default: return "unknown";
	}
}

static const char *depth_to_string ( const int &value ) {
	switch ( value ) {
		case GPU_DEPTH_WRITE_ANY: return "depth_any";
		case GPU_DEPTH_WRITE_GREATER: return "depth_greater";
		case GPU_DEPTH_WRITE_LESS: return "depth_less";
		default: return "depth_unchanged";
	}
}

static void print_image_type ( std::ostream &os , const ImageType &type , const ShaderCreateInfo::Resource::BindType bind_type ) {
	switch ( type ) {
		case ImageType::INT_BUFFER:
		case ImageType::INT_1D:
		case ImageType::INT_1D_ARRAY:
		case ImageType::INT_2D:
		case ImageType::INT_2D_ARRAY:
		case ImageType::INT_3D:
		case ImageType::INT_CUBE:
		case ImageType::INT_CUBE_ARRAY: {
			os << "i";
		}break;
		case ImageType::UINT_BUFFER:
		case ImageType::UINT_1D:
		case ImageType::UINT_1D_ARRAY:
		case ImageType::UINT_2D:
		case ImageType::UINT_2D_ARRAY:
		case ImageType::UINT_3D:
		case ImageType::UINT_CUBE:
		case ImageType::UINT_CUBE_ARRAY: {
			os << "u";
		}break;
		default:
		break;
	}

	if ( bind_type == ShaderCreateInfo::Resource::BindType::IMAGE ) {
		os << "image";
	} else {
		os << "sampler";
	}

	switch ( type ) {
		case ImageType::FLOAT_BUFFER:
		case ImageType::INT_BUFFER:
		case ImageType::UINT_BUFFER: {
			os << "Buffer";
		}break;
		case ImageType::FLOAT_1D:
		case ImageType::FLOAT_1D_ARRAY:
		case ImageType::INT_1D:
		case ImageType::INT_1D_ARRAY:
		case ImageType::UINT_1D:
		case ImageType::UINT_1D_ARRAY: {
			os << "1D";
		}break;
		case ImageType::FLOAT_2D:
		case ImageType::FLOAT_2D_ARRAY:
		case ImageType::INT_2D:
		case ImageType::INT_2D_ARRAY:
		case ImageType::UINT_2D:
		case ImageType::UINT_2D_ARRAY:
		case ImageType::SHADOW_2D:
		case ImageType::SHADOW_2D_ARRAY:
		case ImageType::DEPTH_2D:
		case ImageType::DEPTH_2D_ARRAY: {
			os << "2D";
		}break;
		case ImageType::FLOAT_3D:
		case ImageType::INT_3D:
		case ImageType::UINT_3D: {
			os << "3D";
		}break;
		case ImageType::FLOAT_CUBE:
		case ImageType::FLOAT_CUBE_ARRAY:
		case ImageType::INT_CUBE:
		case ImageType::INT_CUBE_ARRAY:
		case ImageType::UINT_CUBE:
		case ImageType::UINT_CUBE_ARRAY:
		case ImageType::SHADOW_CUBE:
		case ImageType::SHADOW_CUBE_ARRAY:
		case ImageType::DEPTH_CUBE:
		case ImageType::DEPTH_CUBE_ARRAY: {
			os << "Cube";
		}break;
		default:
		break;
	}

	switch ( type ) {
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
		case ImageType::DEPTH_CUBE_ARRAY: {
			os << "Array";
		}break;
		default:
		break;
	}

	switch ( type ) {
		case ImageType::SHADOW_2D:
		case ImageType::SHADOW_2D_ARRAY:
		case ImageType::SHADOW_CUBE:
		case ImageType::SHADOW_CUBE_ARRAY: {
			os << "Shadow";
		}break;
		default:
		break;
	}
	os << " ";
}

static std::ostream &print_qualifier ( std::ostream &os , const int &qualifiers ) {
	if ( bool ( qualifiers & GPU_QUALIFIER_NO_RESTRICT ) == false ) {
		os << "restrict ";
	}
	if ( bool ( qualifiers & GPU_QUALIFIER_READ ) == false ) {
		os << "writeonly ";
	}
	if ( bool ( qualifiers & GPU_QUALIFIER_WRITE ) == false ) {
		os << "readonly ";
	}
	return os;
}

static void print_resource ( std::ostream &os , const ShaderCreateInfo::Resource &res ) {
	if ( GLContext::ExplicitLocationSupport ) {
		os << "layout(binding = " << res.Slot;
		if ( res.BindMethod == ShaderCreateInfo::Resource::BindType::IMAGE ) {
			os << ", " << texture_format_to_string ( res.Image.Format );
		} else if ( res.BindMethod == ShaderCreateInfo::Resource::BindType::UNIFORM_BUFFER ) {
			os << ", std140";
		} else if ( res.BindMethod == ShaderCreateInfo::Resource::BindType::STORAGE_BUFFER ) {
			os << ", std430";
		}
		os << ") ";
	} else if ( res.BindMethod == ShaderCreateInfo::Resource::BindType::UNIFORM_BUFFER ) {
		os << "layout(std140) ";
	}

	int64_t array_offset;
	StringRef name_no_array;

	switch ( res.BindMethod ) {
		case ShaderCreateInfo::Resource::BindType::SAMPLER: {
			os << "uniform ";
			print_image_type ( os , res.Sampler.Type , res.BindMethod );
			os << res.Sampler.Name << ";\n";
		}break;
		case ShaderCreateInfo::Resource::BindType::IMAGE: {
			os << "uniform ";
			print_qualifier ( os , res.Image.Qualifiers );
			print_image_type ( os , res.Image.Type , res.BindMethod );
			os << res.Image.Name << ";\n";
		}break;
		case ShaderCreateInfo::Resource::BindType::UNIFORM_BUFFER: {
			array_offset = res.UniformBuf.Name.FindFirstOf ( "[" );
			name_no_array = ( array_offset == StringRefBase::npos ) ? res.UniformBuf.Name :
				StringRef ( res.UniformBuf.Name.CStr ( ) , array_offset );
			os << "uniform " << name_no_array << " { " << res.UniformBuf.TypeName << " _" << res.UniformBuf.Name << "; };\n";
		}break;
		case ShaderCreateInfo::Resource::BindType::STORAGE_BUFFER: {
			array_offset = res.StorageBuf.Name.FindFirstOf ( "[" );
			name_no_array = ( array_offset == StringRefBase::npos ) ? res.StorageBuf.Name :
				StringRef ( res.StorageBuf.Name.CStr ( ) , array_offset );
			print_qualifier ( os , res.StorageBuf.Qualifiers );
			os << "buffer ";
			os << name_no_array << " { " << res.StorageBuf.TypeName << " _" << res.StorageBuf.Name
				<< "; };\n";
		}break;
	}
}

static void print_resource_alias ( std::ostream &os , const ShaderCreateInfo::Resource &res ) {
	int64_t array_offset;
	StringRef name_no_array;

	switch ( res.BindMethod ) {
		case ShaderCreateInfo::Resource::BindType::UNIFORM_BUFFER: {
			array_offset = res.UniformBuf.Name.FindFirstOf ( "[" );
			name_no_array = ( array_offset == StringRefBase::npos ) ? res.UniformBuf.Name :
				StringRef ( res.UniformBuf.Name.CStr ( ) , array_offset );
			os << "#define " << name_no_array << " (_" << name_no_array << ")\n";
		}break;
		case ShaderCreateInfo::Resource::BindType::STORAGE_BUFFER: {
			array_offset = res.StorageBuf.Name.FindFirstOf ( "[" );
			name_no_array = ( array_offset == StringRefBase::npos ) ? res.StorageBuf.Name :
				StringRef ( res.StorageBuf.Name.CStr ( ) , array_offset );
			os << "#define " << name_no_array << " (_" << name_no_array << ")\n";
		}break;
		default:
		break;
	}
}

static void print_interface ( std::ostream &os , const StringRefNull &prefix , const StageInterfaceInfo &iface , const StringRefNull &suffix = "" ) {
	/* TODO(@fclem): Move that to interface check. */
	// if (iface.instance_name.is_empty()) {
	//   BLI_assert_msg(0, "Interfaces require an instance name for geometry shader.");
	//   std::cout << iface.name << ": Interfaces require an instance name for geometry shader.\n";
	//   continue;
	// }
	os << prefix << " " << iface.Name << "{" << std::endl;
	for ( const StageInterfaceInfo::InOut &inout : iface.InOuts ) {
		os << "  " << interpolation_to_string ( inout.Interp ) << " " << type_to_string ( inout.Type ) << " " << inout.Name << ";\n";
	}
	os << "}";
	os << ( iface.InstanceName.IsEmpty ( ) ? "" : "\n" ) << iface.InstanceName << suffix << ";\n";
}

static std::string main_function_wrapper ( std::string &pre_main , std::string &post_main ) {
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
	return ss.str ( );
}

//

GLShader::GLShader ( const char *name ) : Shader ( name ) {
	this->mShaderProgram = glCreateProgram ( );
}

GLShader::~GLShader ( ) {
	glDeleteProgram ( this->mShaderProgram ); this->mShaderProgram = 0;
}

void GLShader::VertexShader ( MutableSpan<const char *> sources ) {
	this->mVertShader = this->CreateShaderStage ( GL_VERTEX_SHADER , sources );
}

void GLShader::GeometryShader ( MutableSpan<const char *> sources ) {
	this->mGeomShader = this->CreateShaderStage ( GL_GEOMETRY_SHADER , sources );
}

void GLShader::FragmentShader ( MutableSpan<const char *> sources ) {
	this->mFragShader = this->CreateShaderStage ( GL_FRAGMENT_SHADER , sources );
}

void GLShader::ComputeShader ( MutableSpan<const char *> sources ) {
	this->mCompShader = this->CreateShaderStage ( GL_COMPUTE_SHADER , sources );
}

bool GLShader::Finalize ( const ShaderCreateInfo *info ) {
	if ( this->mCompilationFailed ) {
		return false;
	}

	if ( info && DoGeometryShaderInjection ( info ) ) {
		std::string source = WorkaroundGeometryShaderSourceCreate ( *info );
		Vector<const char *> sources;
		sources.Append ( "version" );
		sources.Append ( source.c_str ( ) );
		GeometryShader ( sources );
	}

	glLinkProgram ( this->mShaderProgram );
	GLint status , length;
	glGetProgramiv ( this->mShaderProgram , GL_LINK_STATUS , &status );
	glGetProgramiv ( this->mShaderProgram , GL_INFO_LOG_LENGTH , &length );
	std::vector<char> error ( length + 1 );
	if ( !status ) {
		glGetProgramInfoLog ( this->mShaderProgram , length + 1 , nullptr , &error [ 0 ] );
		fprintf ( stderr , "Failed to link program for shader %s\n%s\n" , this->GetName ( ) , error.data ( ) );
		return false;
	}

	if ( info != nullptr && info->LegacyResourceLocation == false ) {
		this->mInterface = new GLShaderInterface ( this->mShaderProgram , *info );
	} else {
		this->mInterface = new GLShaderInterface ( this->mShaderProgram );
	}

	return true;
}

void GLShader::Bind ( ) {
	glUseProgram ( this->mShaderProgram );
}

void GLShader::Unbind ( ) {
#ifndef NDEBUG
	glUseProgram ( 0 );
#endif
}

void GLShader::UniformFloat ( int location , int comp , int size , const float *data ) {
	switch ( comp ) {
		case 1: {
			glUniform1fv ( location , size , data );
		}break;
		case 2: {
			glUniform2fv ( location , size , data );
		}break;
		case 3: {
			glUniform3fv ( location , size , data );
		}break;
		case 4: {
			glUniform4fv ( location , size , data );
		}break;
		case 9: {
			glUniformMatrix3fv ( location , size , GL_FALSE , data );
		}break;
		case 16: {
			glUniformMatrix4fv ( location , size , GL_FALSE , data );
		}break;
		default: {
			fprintf ( stderr , "Unknown uniform!\n" );
		}break;
	}
}

void GLShader::UniformInt ( int location , int comp , int size , const int *data ) {
	switch ( comp ) {
		case 1: {
			glUniform1iv ( location , size , data );
		}break;
		case 2: {
			glUniform2iv ( location , size , data );
		}break;
		case 3: {
			glUniform3iv ( location , size , data );
		}break;
		case 4: {
			glUniform4iv ( location , size , data );
		}break;
		default: {
			fprintf ( stderr , "Unknown uniform!\n" );
		}break;
	}
}

std::string GLShader::ResourcesDeclare ( const ShaderCreateInfo &info ) const {
	std::stringstream ss;

	/* NOTE: We define macros in GLSL to trigger compilation error if the resource names
	 * are reused for local variables. This is to match other backend behavior which needs accessors
	 * macros. */

	ss << "\n/* Pass Resources. */\n";
	for ( size_t i = 0; i < info.PassResources.Size ( ); i++ ) {
		const ShaderCreateInfo::Resource &res = info.PassResources [ i ];
		print_resource ( ss , res );
	}
	for ( size_t i = 0; i < info.PassResources.Size ( ); i++ ) {
		const ShaderCreateInfo::Resource &res = info.PassResources [ i ];
		print_resource_alias ( ss , res );
	}
	ss << "\n/* Batch Resources. */\n";
	for ( size_t i = 0; i < info.BatchResources.Size ( ); i++ ) {
		const ShaderCreateInfo::Resource &res = info.BatchResources [ i ];
		print_resource ( ss , res );
	}
	for ( size_t i = 0; i < info.BatchResources.Size ( ); i++ ) {
		const ShaderCreateInfo::Resource &res = info.BatchResources [ i ];
		print_resource_alias ( ss , res );
	}
	ss << "\n/* Push Constants. */\n";
	for ( size_t i = 0; i < info.PushConstants.Size ( ); i++ ) {
		const ShaderCreateInfo::PushConst &uniform = info.PushConstants [ i ];
		ss << "uniform " << type_to_string ( uniform.Type ) << " " << uniform.Name;
		if ( uniform.ArraySize > 0 ) {
			ss << "[" << uniform.ArraySize << "]";
		}
		ss << ";\n";
	}
	ss << "\n";
	return ss.str ( );
}

std::string GLShader::VertexInterfaceDeclare ( const ShaderCreateInfo &info ) const {
	std::stringstream ss;
	std::string post_main;

	ss << "\n/* Inputs. */\n";
	for ( const ShaderCreateInfo::VertIn &attr : info.VertexInputs ) {
		if ( GLContext::ExplicitLocationSupport &&
		     /* Fix issue with AMDGPU-PRO + workbench_prepass_mesh_vert.glsl being quantized. */
		     GPU_type_matches ( GPU_DEVICE_ATI , GPU_OS_ANY , GPU_DRIVER_OFFICIAL ) == false ) {
			ss << "layout(location = " << attr.Index << ") ";
		}
		ss << "in " << type_to_string ( attr.Type ) << " " << attr.Name << ";\n";
	}
	/* NOTE(D4490): Fix a bug where shader without any vertex attributes do not behave correctly. */
	if ( GPU_type_matches_ex ( GPU_DEVICE_APPLE , GPU_OS_MAC , GPU_DRIVER_ANY , GPU_BACKEND_OPENGL ) &&
	     info.VertexInputs.IsEmpty ( ) ) {
		ss << "in float gpu_dummy_workaround;\n";
	}
	ss << "\n/* Interfaces. */\n";
	for ( const StageInterfaceInfo *iface : info.VertexOutInterfaces ) {
		print_interface ( ss , "out" , *iface );
	}
	if ( !GLContext::LayeredRenderingSupport && bool ( info.BuiltinBits & GPU_BUILTIN_BITS_LAYER ) ) {
		ss << "out int gpu_Layer;\n";
	}
	if ( bool ( info.BuiltinBits & GPU_BUILTIN_BITS_BARYCENTRIC_COORD ) ) {
		if ( !GLContext::NativeBarycentricSupport ) {
			/* Disabled or unsupported. */
		} else if ( has_gl_extension ( "GL_AMD_shader_explicit_vertex_parameter" ) ) {
			/* Need this for stable barycentric. */
			ss << "flat out vec4 gpu_pos_flat;\n";
			ss << "out vec4 gpu_pos;\n";

			post_main += "  gpu_pos = gpu_pos_flat = gl_Position;\n";
		}
	}
	ss << "\n";

	if ( post_main.empty ( ) == false ) {
		std::string pre_main;
		ss << main_function_wrapper ( pre_main , post_main );
	}
	return ss.str ( );
}

std::string GLShader::FragmentInterfaceDeclare ( const ShaderCreateInfo &info ) const {
	std::stringstream ss;
	std::string pre_main;

	ss << "\n/* Interfaces. */\n";
	const Vector<StageInterfaceInfo *> &in_interfaces = ( info.GeometrySource.IsEmpty ( ) ) ?
		info.VertexOutInterfaces : info.GeometryOutInterfaces;
	for ( const StageInterfaceInfo *iface : in_interfaces ) {
		print_interface ( ss , "in" , *iface );
	}
	if ( bool ( info.BuiltinBits & GPU_BUILTIN_BITS_BARYCENTRIC_COORD ) ) {
		if ( !GLContext::NativeBarycentricSupport ) {
			ss << "flat in vec4 gpu_pos[3];\n";
			ss << "smooth in vec3 gpu_BaryCoord;\n";
			ss << "noperspective in vec3 gpu_BaryCoordNoPersp;\n";
			ss << "#define gpu_position_at_vertex(v) gpu_pos[v]\n";
		} else if ( has_gl_extension ( "GL_AMD_shader_explicit_vertex_parameter" ) ) {
			std::cout << "native" << std::endl;
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
	if ( info.EarlyFragmentTest ) {
		ss << "layout(early_fragment_tests) in;\n";
	}
	if ( has_gl_extension ( "GL_ARB_conservative_depth" ) ) {
		ss << "layout(" << depth_to_string ( info.DepthWrite ) << ") out float gl_FragDepth;\n";
	}
	ss << "\n/* Outputs. */\n";
	for ( const ShaderCreateInfo::FragOut &output : info.FragmentOutputs ) {
		ss << "layout(location = " << output.Index;
		switch ( output.Blend ) {
			case GPU_DUAL_BLEND_SRC_0: {
				ss << ", index = 0";
			}break;
			case GPU_DUAL_BLEND_SRC_1: {
				ss << ", index = 1";
			}break;
			default:
			break;
		}
		ss << ") ";
		ss << "out " << type_to_string ( output.Type ) << " " << output.Name << ";\n";
	}
	ss << "\n";

	if ( pre_main.empty ( ) == false ) {
		std::string post_main;
		ss << main_function_wrapper ( pre_main , post_main );
	}
	return ss.str ( );
}

static StageInterfaceInfo *find_interface_by_name ( const Vector<StageInterfaceInfo *> &ifaces , const StringRefNull &name ) {
	for ( auto *iface : ifaces ) {
		if ( iface->InstanceName == name ) {
			return iface;
		}
	}
	return nullptr;
}

std::string GLShader::GeometryInterfaceDeclare ( const ShaderCreateInfo &info ) const {
	std::stringstream ss;

	ss << "\n/* Interfaces. */\n";
	for ( const StageInterfaceInfo *iface : info.VertexOutInterfaces ) {
		bool has_matching_output_iface = find_interface_by_name ( info.GeometryOutInterfaces , iface->InstanceName ) != nullptr;
		const char *suffix = ( has_matching_output_iface ) ? "_in[]" : "[]";
		print_interface ( ss , "in" , *iface , suffix );
	}
	ss << "\n";
	for ( const StageInterfaceInfo *iface : info.GeometryOutInterfaces ) {
		bool has_matching_input_iface = find_interface_by_name ( info.VertexOutInterfaces , iface->InstanceName ) != nullptr;
		const char *suffix = ( has_matching_input_iface ) ? "_out" : "";
		print_interface ( ss , "out" , *iface , suffix );
	}
	ss << "\n";
	return ss.str ( );
}

std::string GLShader::GeometryLayoutDeclare ( const ShaderCreateInfo &info ) const {
	int max_verts = info.GeometryLayout.MaxVertices;
	int invocations = info.GeometryLayout.Invocations;

	if ( GLContext::GeometryShaderInvocations == false && invocations != -1 ) {
		max_verts *= invocations;
		invocations = -1;
	}

	std::stringstream ss;
	ss << "\n/* Geometry Layout. */\n";
	ss << "layout(" << primitive_in_to_string ( info.GeometryLayout.PrimIn );
	if ( invocations != -1 ) {
		ss << ", invocations = " << invocations;
	}
	ss << ") in;\n";

	ss << "layout(" << primitive_out_to_string ( info.GeometryLayout.PrimOut )
		<< ", max_vertices = " << max_verts << ") out;\n";
	ss << "\n";
	return ss.str ( );
}

std::string GLShader::ComputeLayoutDeclare ( const ShaderCreateInfo &info ) const {
	std::stringstream ss;
	ss << "\n/* Compute Layout. */\n";
	ss << "layout(local_size_x = " << info.ComputeLayout.LocalSizeX;
	if ( info.ComputeLayout.LocalSizeY != -1 ) {
		ss << ", local_size_y = " << info.ComputeLayout.LocalSizeY;
	}
	if ( info.ComputeLayout.LocalSizeZ != -1 ) {
		ss << ", local_size_z = " << info.ComputeLayout.LocalSizeZ;
	}
	ss << ") in;\n";
	ss << "\n";
	return ss.str ( );
}

//

char *GLShader::GetPatch ( unsigned int gl_stage ) {
	if ( gl_stage == GL_COMPUTE_SHADER ) {
		return glsl_patch_compute_get ( );
	}
	return glsl_patch_default_get ( );
}

unsigned int GLShader::CreateShaderStage ( unsigned int gl_stage , MutableSpan<const char *> sources ) {
	GLuint shader = glCreateShader ( gl_stage );
	if ( shader == 0 ) {
		fprintf ( stderr , "GLShader: Error: Could not create shader object.\n" );
		return 0;
	}

	/* Patch the shader code using the first source slot. */
	sources [ 0 ] = GetPatch ( gl_stage );

	glShaderSource ( shader , sources.Size ( ) , sources.Data ( ) , nullptr );
	glCompileShader ( shader );

	GLint status;
	glGetShaderiv ( shader , GL_COMPILE_STATUS , &status );
	if ( !status ) {
		GLint length;
		glGetShaderiv ( shader , GL_INFO_LOG_LENGTH , &length );
		std::vector<char> error ( size_t ( length ) + 1 );
		glGetShaderInfoLog ( shader , length + 1 , nullptr , &error [ 0 ] );
		switch ( gl_stage ) {
			case GL_VERTEX_SHADER: fprintf ( stderr , "Vertex Shader Error\n%s\n" , error.data ( ) ); break;
			case GL_GEOMETRY_SHADER: fprintf ( stderr , "Geometry Shader Error\n%s\n" , error.data ( ) ); break;
			case GL_FRAGMENT_SHADER: fprintf ( stderr , "Fragment Shader Error\n%s\n" , error.data ( ) ); break;
			case GL_COMPUTE_SHADER: fprintf ( stderr , "Compute Shader Error\n%s\n" , error.data ( ) ); break;
			default: fprintf ( stderr , "Unknown Shader Stage Error\n%s\n" , error.data ( ) ); break;
		}
	}
	if ( !status ) {
		glDeleteShader ( shader );
		this->mCompilationFailed = true;
		return 0;
	}

	glAttachShader ( this->mShaderProgram , shader );
	return shader;
}

std::string GLShader::WorkaroundGeometryShaderSourceCreate ( const ShaderCreateInfo &info ) {
	std::stringstream ss;

	const bool do_layer_workaround = !GLContext::LayeredRenderingSupport && bool ( info.BuiltinBits & GPU_BUILTIN_BITS_LAYER );
	const bool do_barycentric_workaround = !GLContext::NativeBarycentricSupport && bool ( info.BuiltinBits & GPU_BUILTIN_BITS_BARYCENTRIC_COORD );

	ShaderCreateInfo info_modified = info;
	info_modified.GeometryOutInterfaces = info_modified.VertexOutInterfaces;
	/**
	 * NOTE(@fclem): Assuming we will render TRIANGLES. This will not work with other primitive
	 * types. In this case, it might not trigger an error on some implementations.
	 */
	info_modified.SetGeometryLayout ( GPU_PRIMITIVE_IN_TRIANGLES , GPU_PRIMITIVE_OUT_TRIANGLE_STRIP , 3 );

	ss << GeometryLayoutDeclare ( info_modified );
	ss << GeometryInterfaceDeclare ( info_modified );
	if ( do_layer_workaround ) {
		ss << "in int gpu_Layer[];\n";
	}
	if ( do_barycentric_workaround ) {
		ss << "flat out vec4 gpu_pos[3];\n";
		ss << "smooth out vec3 gpu_BaryCoord;\n";
		ss << "noperspective out vec3 gpu_BaryCoordNoPersp;\n";
	}
	ss << "\n";

	ss << "void main()\n";
	ss << "{\n";
	if ( do_layer_workaround ) {
		ss << "  gl_Layer = gpu_Layer[0];\n";
	}
	if ( do_barycentric_workaround ) {
		ss << "  gpu_pos[0] = gl_in[0].gl_Position;\n";
		ss << "  gpu_pos[1] = gl_in[1].gl_Position;\n";
		ss << "  gpu_pos[2] = gl_in[2].gl_Position;\n";
	}
	for ( size_t i = 0 ; i < 3 ; i++ ) {
		for ( StageInterfaceInfo *iface : info_modified.VertexOutInterfaces ) {
			for ( auto &inout : iface->InOuts ) {
				ss << "  " << iface->InstanceName << "_out." << inout.Name;
				ss << " = " << iface->InstanceName << "_in[" << i << "]." << inout.Name << ";\n";
			}
		}
		if ( do_barycentric_workaround ) {
			ss << "  gpu_BaryCoordNoPersp = gpu_BaryCoord =";
			ss << " vec3(" << int ( i == 0 ) << ", " << int ( i == 1 ) << ", " << int ( i == 2 ) << ");\n";
		}
		ss << "  gl_Position = gl_in[" << i << "].gl_Position;\n";
		ss << "  EmitVertex();\n";
	}
	ss << "}\n";
	return ss.str ( );
}

bool GLShader::DoGeometryShaderInjection ( const ShaderCreateInfo *info ) {
	int builtins = info->BuiltinBits;
	if ( !GLContext::NativeBarycentricSupport && bool ( builtins & GPU_BUILTIN_BITS_BARYCENTRIC_COORD ) ) {
		return true;
	}
	if ( !GLContext::LayeredRenderingSupport && bool ( builtins & GPU_BUILTIN_BITS_LAYER ) ) {
		return true;
	}
	return false;
}

}
}
