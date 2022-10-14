#include "gl_shader_interface.h"
#include "gl_context.h"

#include "gpu/gpu_info.h"

#include "lib/lib_bitmap.h"

#include <gl/glew.h>

namespace rose {
namespace gpu {

// Binding assignment

static inline int block_binding ( int32_t program , uint32_t block_index ) {
	/* For now just assign a consecutive index. In the future, we should set it in
	 * the shader using layout(binding = i) and query its value. */
	glUniformBlockBinding ( program , block_index , block_index );
	return block_index;
}

static inline int sampler_binding ( int32_t program , uint32_t uniform_index , int32_t uniform_location , int *sampler_len ) {
	/* Identify sampler uniforms and assign sampler units to them. */
	GLint type;
	glGetActiveUniformsiv ( program , 1 , &uniform_index , GL_UNIFORM_TYPE , &type );
	switch ( type ) {
		case GL_SAMPLER_1D:
		case GL_SAMPLER_2D:
		case GL_SAMPLER_3D:
		case GL_SAMPLER_CUBE:
		case GL_SAMPLER_CUBE_MAP_ARRAY_ARB: /* OpenGL 4.0 */
		case GL_SAMPLER_1D_SHADOW:
		case GL_SAMPLER_2D_SHADOW:
		case GL_SAMPLER_1D_ARRAY:
		case GL_SAMPLER_2D_ARRAY:
		case GL_SAMPLER_1D_ARRAY_SHADOW:
		case GL_SAMPLER_2D_ARRAY_SHADOW:
		case GL_SAMPLER_2D_MULTISAMPLE:
		case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
		case GL_SAMPLER_CUBE_SHADOW:
		case GL_SAMPLER_BUFFER:
		case GL_INT_SAMPLER_1D:
		case GL_INT_SAMPLER_2D:
		case GL_INT_SAMPLER_3D:
		case GL_INT_SAMPLER_CUBE:
		case GL_INT_SAMPLER_1D_ARRAY:
		case GL_INT_SAMPLER_2D_ARRAY:
		case GL_INT_SAMPLER_2D_MULTISAMPLE:
		case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
		case GL_INT_SAMPLER_BUFFER:
		case GL_UNSIGNED_INT_SAMPLER_1D:
		case GL_UNSIGNED_INT_SAMPLER_2D:
		case GL_UNSIGNED_INT_SAMPLER_3D:
		case GL_UNSIGNED_INT_SAMPLER_CUBE:
		case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
		case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
		case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
		case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
		case GL_UNSIGNED_INT_SAMPLER_BUFFER: {
			/* For now just assign a consecutive index. In the future, we should set it in
			 * the shader using layout(binding = i) and query its value. */
			int binding = *sampler_len;
			glUniform1i ( uniform_location , binding );
			( *sampler_len )++;
			return binding;
		}
		default:
		return -1;
	}
}

static inline int image_binding ( int32_t program , uint32_t uniform_index , int32_t uniform_location , int *image_len ) {
	/* Identify image uniforms and assign image units to them. */
	GLint type;
	glGetActiveUniformsiv ( program , 1 , &uniform_index , GL_UNIFORM_TYPE , &type );
	switch ( type ) {
		case GL_IMAGE_1D:
		case GL_IMAGE_2D:
		case GL_IMAGE_3D:
		case GL_IMAGE_CUBE:
		case GL_IMAGE_BUFFER:
		case GL_IMAGE_1D_ARRAY:
		case GL_IMAGE_2D_ARRAY:
		case GL_IMAGE_CUBE_MAP_ARRAY:
		case GL_INT_IMAGE_1D:
		case GL_INT_IMAGE_2D:
		case GL_INT_IMAGE_3D:
		case GL_INT_IMAGE_CUBE:
		case GL_INT_IMAGE_BUFFER:
		case GL_INT_IMAGE_1D_ARRAY:
		case GL_INT_IMAGE_2D_ARRAY:
		case GL_INT_IMAGE_CUBE_MAP_ARRAY:
		case GL_UNSIGNED_INT_IMAGE_1D:
		case GL_UNSIGNED_INT_IMAGE_2D:
		case GL_UNSIGNED_INT_IMAGE_3D:
		case GL_UNSIGNED_INT_IMAGE_CUBE:
		case GL_UNSIGNED_INT_IMAGE_BUFFER:
		case GL_UNSIGNED_INT_IMAGE_1D_ARRAY:
		case GL_UNSIGNED_INT_IMAGE_2D_ARRAY:
		case GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY: {
			/* For now just assign a consecutive index. In the future, we should set it in
			 * the shader using layout(binding = i) and query its value. */
			int binding = *image_len;
			glUniform1i ( uniform_location , binding );
			( *image_len )++;
			return binding;
		}
		default:
		return -1;
	}
}

static inline int ssbo_binding ( int32_t program , uint32_t ssbo_index ) {
	GLint binding = -1;
	GLenum property = GL_BUFFER_BINDING;
	GLint values_written = 0;
	glGetProgramResourceiv ( program , GL_SHADER_STORAGE_BLOCK , ssbo_index , 1 , &property , 1 , &values_written , &binding );
	return binding;
}

// Creation / Destruction

static Type gpu_type_from_gl_type ( int gl_type ) {
	switch ( gl_type ) {
		case GL_FLOAT: return Type::FLOAT;
		case GL_FLOAT_VEC2: return Type::VEC2;
		case GL_FLOAT_VEC3: return Type::VEC3;
		case GL_FLOAT_VEC4: return Type::VEC4;
		case GL_FLOAT_MAT3: return Type::MAT3;
		case GL_FLOAT_MAT4: return Type::MAT4;
		case GL_UNSIGNED_INT: return Type::UINT;
		case GL_UNSIGNED_INT_VEC2: return Type::UVEC2;
		case GL_UNSIGNED_INT_VEC3: return Type::UVEC3;
		case GL_UNSIGNED_INT_VEC4: return Type::UVEC4;
		case GL_INT: return Type::INT;
		case GL_INT_VEC2: return Type::IVEC2;
		case GL_INT_VEC3: return Type::IVEC3;
		case GL_INT_VEC4: return Type::IVEC4;
		case GL_BOOL: return Type::BOOL;
		case GL_FLOAT_MAT2:
		case GL_FLOAT_MAT2x3:
		case GL_FLOAT_MAT2x4:
		case GL_FLOAT_MAT3x2:
		case GL_FLOAT_MAT3x4:
		case GL_FLOAT_MAT4x2:
		case GL_FLOAT_MAT4x3:
		default: {
			fprintf ( stderr , "Unrecognized OpenGL type.\n" );
		}break;
	}
	return Type::FLOAT;
}

GLShaderInterface::GLShaderInterface ( unsigned int program ) {
	/* Necessary to make #glUniform works. */
	glUseProgram ( program );

	GLint max_attr_name_len = 0 , attr_len = 0;
	glGetProgramiv ( program , GL_ACTIVE_ATTRIBUTE_MAX_LENGTH , &max_attr_name_len );
	glGetProgramiv ( program , GL_ACTIVE_ATTRIBUTES , &attr_len );

	GLint max_ubo_name_len = 0 , ubo_len = 0;
	glGetProgramiv ( program , GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH , &max_ubo_name_len );
	glGetProgramiv ( program , GL_ACTIVE_UNIFORM_BLOCKS , &ubo_len );

	GLint max_uniform_name_len = 0 , active_uniform_len = 0 , uniform_len = 0;
	glGetProgramiv ( program , GL_ACTIVE_UNIFORM_MAX_LENGTH , &max_uniform_name_len );
	glGetProgramiv ( program , GL_ACTIVE_UNIFORMS , &active_uniform_len );
	uniform_len = active_uniform_len;

	GLint max_ssbo_name_len = 0 , ssbo_len = 0;
	if ( GPU_get_info_i ( GPU_INFO_SHADER_STORAGE_BUFFER_OBJECTS_SUPPORT ) ) {
		glGetProgramInterfaceiv ( program , GL_SHADER_STORAGE_BLOCK , GL_ACTIVE_RESOURCES , &ssbo_len );
		glGetProgramInterfaceiv ( program , GL_SHADER_STORAGE_BLOCK , GL_MAX_NAME_LENGTH , &max_ssbo_name_len );
	}

	if ( ubo_len > 16 ) {
		fprintf ( stderr , "enabled_ubo_mask_ is uint16_t" );
	}

	/* Work around driver bug with Intel HD 4600 on Windows 7/8, where
	 * GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH does not work. */
	if ( attr_len > 0 && max_attr_name_len == 0 ) {
		max_attr_name_len = 256;
	}
	if ( ubo_len > 0 && max_ubo_name_len == 0 ) {
		max_ubo_name_len = 256;
	}
	if ( uniform_len > 0 && max_uniform_name_len == 0 ) {
		max_uniform_name_len = 256;
	}
	if ( ssbo_len > 0 && max_ssbo_name_len == 0 ) {
		max_ssbo_name_len = 256;
	}

	/* GL_ACTIVE_UNIFORMS lied to us! Remove the UBO uniforms from the total before
	 * allocating the uniform array. */
	GLint max_ubo_uni_len = 0;
	for ( int i = 0; i < ubo_len; i++ ) {
		GLint ubo_uni_len;
		glGetActiveUniformBlockiv ( program , i , GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS , &ubo_uni_len );
		max_ubo_uni_len = ROSE_MAX ( max_ubo_uni_len , ubo_uni_len );
		uniform_len -= ubo_uni_len;
	}
	/* Bit set to true if uniform comes from a uniform block. */
	LIB_bitmap *uniforms_from_blocks = LIB_BITMAP_NEW ( active_uniform_len , __func__ );
	/* Set uniforms from block for exclusion. */
	GLint *ubo_uni_ids = ( GLint * ) MEM_mallocN ( sizeof ( GLint ) * max_ubo_uni_len , __func__ );
	for ( int i = 0; i < ubo_len; i++ ) {
		GLint ubo_uni_len;
		glGetActiveUniformBlockiv ( program , i , GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS , &ubo_uni_len );
		glGetActiveUniformBlockiv ( program , i , GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES , ubo_uni_ids );
		for ( int u = 0; u < ubo_uni_len; u++ ) {
			LIB_BITMAP_ENABLE ( uniforms_from_blocks , ubo_uni_ids [ u ] );
		}
	}
	MEM_freeN ( ubo_uni_ids );

	int input_tot_len = attr_len + ubo_len + uniform_len + ssbo_len;
	this->mInputs = ( ShaderInput * ) MEM_mallocN ( sizeof ( ShaderInput ) * input_tot_len , __FUNCTION__ );

	const uint32_t name_buffer_len = attr_len * max_attr_name_len + ubo_len * max_ubo_name_len +
		uniform_len * max_uniform_name_len + ssbo_len * max_ssbo_name_len;
	this->mNameBuffer = ( char * ) MEM_mallocN ( name_buffer_len , "name_buffer" );
	uint32_t name_buffer_offset = 0;

	/* Attributes */
	this->mEnabledAttrMask = 0;
	for ( int i = 0; i < attr_len; i++ ) {
		char *name = this->mNameBuffer + name_buffer_offset;
		GLsizei remaining_buffer = name_buffer_len - name_buffer_offset;
		GLsizei name_len = 0;
		GLenum type;
		GLint size;

		glGetActiveAttrib ( program , i , remaining_buffer , &name_len , &size , &type , name );
		GLint location = glGetAttribLocation ( program , name );
		/* Ignore OpenGL names like `gl_BaseInstanceARB`, `gl_InstanceID` and `gl_VertexID`. */
		if ( location == -1 ) {
			continue;
		}

		ShaderInput *input = &this->mInputs [ this->mAttrLen++ ];
		input->Location = input->Binding = location;

		name_buffer_offset += SetInputName ( input , name , name_len );
		this->mEnabledAttrMask |= ( 1 << input->Location );

		/* Used in `GPU_shader_get_attribute_info`. */
		this->mAttrTypes [ input->Location ] = ( int32_t ) gpu_type_from_gl_type ( type );
	}

	/* Uniform Blocks */
	for ( int i = 0; i < ubo_len; i++ ) {
		char *name = this->mNameBuffer + name_buffer_offset;
		GLsizei remaining_buffer = name_buffer_len - name_buffer_offset;
		GLsizei name_len = 0;

		glGetActiveUniformBlockName ( program , i , remaining_buffer , &name_len , name );

		ShaderInput *input = &this->mInputs [ this->mAttrLen + this->mUboLen++ ];
		input->Binding = input->Location = block_binding ( program , i );

		name_buffer_offset += this->SetInputName ( input , name , name_len );
		this->mEnabledUboMask |= ( 1 << input->Binding );
	}

	/* Uniforms & samplers & images */
	for ( int i = 0 , sampler = 0 , image = 0; i < active_uniform_len; i++ ) {
		if ( LIB_BITMAP_TEST ( uniforms_from_blocks , i ) ) {
			continue;
		}
		char *name = this->mNameBuffer + name_buffer_offset;
		GLsizei remaining_buffer = name_buffer_len - name_buffer_offset;
		GLsizei name_len = 0;

		glGetActiveUniformName ( program , i , remaining_buffer , &name_len , name );

		ShaderInput *input = &this->mInputs [ this->mAttrLen + this->mUboLen + this->mUniformLen++ ];
		input->Location = glGetUniformLocation ( program , name );
		input->Binding = sampler_binding ( program , i , input->Location , &sampler );

		name_buffer_offset += this->SetInputName ( input , name , name_len );
		this->mEnabledTexMask |= ( input->Binding != -1 ) ? ( 1lu << input->Binding ) : 0lu;

		if ( input->Binding == -1 ) {
			input->Binding = image_binding ( program , i , input->Location , &image );

			this->mEnabledImaMask |= ( input->Binding != -1 ) ? ( 1lu << input->Binding ) : 0lu;
		}
	}

	/* SSBOs */
	for ( int i = 0; i < ssbo_len; i++ ) {
		char *name = this->mNameBuffer + name_buffer_offset;
		GLsizei remaining_buffer = name_buffer_len - name_buffer_offset;
		GLsizei name_len = 0;
		glGetProgramResourceName (
			program , GL_SHADER_STORAGE_BLOCK , i , remaining_buffer , &name_len , name );

		const GLint binding = ssbo_binding ( program , i );

		ShaderInput *input = &this->mInputs [ this->mAttrLen + this->mUboLen + this->mUniformLen + this->mShaderStorageBufferLen++ ];
		input->Binding = input->Location = binding;

		name_buffer_offset += this->SetInputName ( input , name , name_len );
		this->mEnabledSSBOMask |= ( input->Binding != -1 ) ? ( 1lu << input->Binding ) : 0lu;
	}

	/* Builtin Uniforms */
	for ( int32_t u_int = 0; u_int < GPU_NUM_UNIFORMS; u_int++ ) {
		int u = static_cast< int >( u_int );
		this->mBuiltins [ u ] = glGetUniformLocation ( program , BuiltinUniformName ( u ) );
	}

	/* Builtin Uniforms Blocks */
	for ( int32_t u_int = 0; u_int < GPU_NUM_UNIFORM_BLOCKS; u_int++ ) {
		int u = static_cast< int >( u_int );
		const ShaderInput *block = this->UboGet ( BuiltinUniformBlockName ( u ) );
		this->mBuiltinBlocks [ u ] = ( block != nullptr ) ? block->Binding : -1;
	}

	/* Builtin Storage Buffers */
	for ( int32_t u_int = 0; u_int < GPU_NUM_STORAGE_BUFFERS; u_int++ ) {
		int u = static_cast< int >( u_int );
		const ShaderInput *block = this->ShaderStorageBufferGet ( BuiltinStorageBlockName ( u ) );
		this->mBuiltinBuffers [ u ] = ( block != nullptr ) ? block->Binding : -1;
	}

	MEM_freeN ( uniforms_from_blocks );

	/* Resize name buffer to save some memory. */
	if ( name_buffer_offset < name_buffer_len ) {
		this->mNameBuffer = ( char * ) MEM_reallocN ( this->mNameBuffer , name_buffer_offset );
	}

	// this->debug_print();

	this->SortInputs ( );
}

GLShaderInterface::GLShaderInterface ( unsigned int program , const ShaderCreateInfo &info ) {
	this->mAttrLen = info.VertexInputs.Size ( );
	this->mUniformLen = info.PushConstants.Size ( );
	this->mUboLen = 0;
	this->mShaderStorageBufferLen = 0;

	Vector<ShaderCreateInfo::Resource> all_resources;
	all_resources.Extend ( info.PassResources );
	all_resources.Extend ( info.BatchResources );

	for ( ShaderCreateInfo::Resource &res : all_resources ) {
		switch ( res.BindMethod ) {
			case ShaderCreateInfo::Resource::BindType::UNIFORM_BUFFER: {
				this->mUboLen++;
			}break;
			case ShaderCreateInfo::Resource::BindType::STORAGE_BUFFER: {
				this->mShaderStorageBufferLen++;
			}break;
			case ShaderCreateInfo::Resource::BindType::SAMPLER: {
				this->mUniformLen++;
			}break;
			case ShaderCreateInfo::Resource::BindType::IMAGE: {
				this->mUniformLen++;
			}break;
		}
	}

	size_t workaround_names_size = 0;
	Vector<StringRefNull> workaround_uniform_names;
	auto check_enabled_uniform = [ & ] ( const char *uniform_name ) {
		if ( glGetUniformLocation ( program , uniform_name ) != -1 ) {
			workaround_uniform_names.Append ( uniform_name );
			workaround_names_size += StringRefNull ( uniform_name ).Size ( ) + 1;
			this->mUniformLen++;
		}
	};

	if ( !GLContext::ShaderDrawParametersSupport ) {
		check_enabled_uniform ( "gpu_BaseInstance" );
	}

	if ( this->mUboLen > 16 ) {
		fprintf ( stderr , "Warning! enabled_ubo_mask_ is uint16_t" );
	}

	int input_tot_len = this->mAttrLen + this->mUboLen + this->mUniformLen + this->mShaderStorageBufferLen;
	this->mInputs = ( ShaderInput * ) MEM_mallocN ( sizeof ( ShaderInput ) * input_tot_len , __FUNCTION__ );
	ShaderInput *input = this->mInputs;

	this->mNameBuffer = ( char * ) MEM_mallocN ( info.InterfaceNamesSize + workaround_names_size , "name_buffer" );
	uint32_t name_buffer_offset = 0;

	/* Necessary to make #glUniform works. TODO(fclem) Remove. */
	glUseProgram ( program );

	/* Attributes */
	for ( const ShaderCreateInfo::VertIn &attr : info.VertexInputs ) {
		CopyInputName ( input , attr.Name , this->mNameBuffer , name_buffer_offset );
		if ( true || !GLContext::ExplicitLocationSupport ) {
			input->Location = input->Binding = glGetAttribLocation ( program , attr.Name.CStr ( ) );
		} else {
			input->Location = input->Binding = attr.Index;
		}
		if ( input->Location != -1 ) {
			this->mEnabledAttrMask |= ( 1 << input->Location );

			/* Used in `GPU_shader_get_attribute_info`. */
			this->mAttrTypes [ input->Location ] = ( int32_t ) attr.Type;
		}

		input++;
	}

	/* Uniform Blocks */
	for ( const ShaderCreateInfo::Resource &res : all_resources ) {
		if ( res.BindMethod == ShaderCreateInfo::Resource::BindType::UNIFORM_BUFFER ) {
			CopyInputName ( input , res.UniformBuf.Name , this->mNameBuffer , name_buffer_offset );
			if ( true || !GLContext::ExplicitLocationSupport ) {
				input->Location = glGetUniformBlockIndex ( program , this->mNameBuffer + input->NameOffset );
				glUniformBlockBinding ( program , input->Location , res.Slot );
			}
			input->Binding = res.Slot;
			this->mEnabledUboMask |= ( 1 << input->Binding );
			input++;
		}
	}

	/* Uniforms & samplers & images */
	for ( const ShaderCreateInfo::Resource &res : all_resources ) {
		if ( res.BindMethod == ShaderCreateInfo::Resource::BindType::SAMPLER ) {
			CopyInputName ( input , res.Sampler.Name , this->mNameBuffer , name_buffer_offset );
			/* Until we make use of explicit uniform location or eliminate all
			 * sampler manually changing. */
			if ( true || !GLContext::ExplicitLocationSupport ) {
				input->Location = glGetUniformLocation ( program , res.Sampler.Name.CStr ( ) );
				glUniform1i ( input->Location , res.Slot );
			}
			input->Binding = res.Slot;
			this->mEnabledTexMask |= ( 1ull << input->Binding );
			input++;
		} else if ( res.BindMethod == ShaderCreateInfo::Resource::BindType::IMAGE ) {
			CopyInputName ( input , res.Image.Name , this->mNameBuffer , name_buffer_offset );
			/* Until we make use of explicit uniform location. */
			if ( true || !GLContext::ExplicitLocationSupport ) {
				input->Location = glGetUniformLocation ( program , res.Image.Name.CStr ( ) );
				glUniform1i ( input->Location , res.Slot );
			}
			input->Binding = res.Slot;
			this->mEnabledImaMask |= ( 1 << input->Binding );
			input++;
		}
	}
	for ( const ShaderCreateInfo::PushConst &uni : info.PushConstants ) {
		CopyInputName ( input , uni.Name , this->mNameBuffer , name_buffer_offset );
		input->Location = glGetUniformLocation ( program , this->mNameBuffer + input->NameOffset );
		input->Binding = -1;
		input++;
	}

	/* Compatibility uniforms. */
	for ( auto &name : workaround_uniform_names ) {
		CopyInputName ( input , name , this->mNameBuffer , name_buffer_offset );
		input->Location = glGetUniformLocation ( program , this->mNameBuffer + input->NameOffset );
		input->Binding = -1;
		input++;
	}

	/* SSBOs */
	for ( const ShaderCreateInfo::Resource &res : all_resources ) {
		if ( res.BindMethod == ShaderCreateInfo::Resource::BindType::STORAGE_BUFFER ) {
			CopyInputName ( input , res.StorageBuf.Name , this->mNameBuffer , name_buffer_offset );
			input->Location = input->Binding = res.Slot;
			this->mEnabledSSBOMask |= ( 1 << input->Binding );
			input++;
		}
	}

	/* Builtin Uniforms */
	for ( int32_t u_int = 0; u_int < GPU_NUM_UNIFORMS; u_int++ ) {
		int u = static_cast< int >( u_int );
		const ShaderInput *uni = this->UniformGet ( BuiltinUniformName ( u ) );
		this->mBuiltins [ u ] = ( uni != nullptr ) ? uni->Location : -1;
	}

	/* Builtin Uniforms Blocks */
	for ( int32_t u_int = 0; u_int < GPU_NUM_UNIFORM_BLOCKS; u_int++ ) {
		int u = static_cast< int >( u_int );
		const ShaderInput *block = this->UboGet ( BuiltinUniformBlockName ( u ) );
		this->mBuiltinBlocks [ u ] = ( block != nullptr ) ? block->Binding : -1;
	}

	/* Builtin Storage Buffers */
	for ( int32_t u_int = 0; u_int < GPU_NUM_STORAGE_BUFFERS; u_int++ ) {
		int u = static_cast< int >( u_int );
		const ShaderInput *block = this->ShaderStorageBufferGet ( BuiltinStorageBlockName ( u ) );
		this->mBuiltinBuffers [ u ] = ( block != nullptr ) ? block->Binding : -1;
	}

	this->SortInputs ( );
}

GLShaderInterface::~GLShaderInterface ( ) {

}

void GLShaderInterface::RefAdd ( GLVaoCache *ref ) {
	for ( int i = 0; i < this->mRefs.Size ( ); i++ ) {
		if ( this->mRefs [ i ] == nullptr ) {
			this->mRefs [ i ] = ref;
			return;
		}
	}
	this->mRefs.Append ( ref );
}

void GLShaderInterface::RefRemove ( GLVaoCache *ref ) {
	for ( int i = 0; i < this->mRefs.Size ( ); i++ ) {
		if ( this->mRefs [ i ] == ref ) {
			this->mRefs [ i ] = nullptr;
			break; // cannot have duplicates.
		}
	}
}

}
}
