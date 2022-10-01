#pragma once

// Opaque type hiding #rose::gpu::Shader
typedef struct GPU_Shader GPU_Shader;
// Opaque type hiding #rose::gpu::shader::ShaderCreateInfo
typedef struct GPU_ShaderCreateInfo GPU_ShaderCreateInfo;

// Create default shader from code.
GPU_Shader *GPU_shader_create ( const char *vertcode ,
				const char *fragcode ,
				const char *geomcode ,
				const char *libcode ,
				const char *defines ,
				const char *name );

// Create compute shader from code.
GPU_Shader *GPU_shader_create_compute ( const char *computecode ,
					const char *libcode ,
					const char *defines ,
					const char *name );

// Create shader from code.
GPU_Shader *GPU_shader_create_ex ( const char *vertcode ,
				   const char *fragcode ,
				   const char *geomcode ,
				   const char *computecode ,
				   const char *libcode ,
				   const char *defines ,
				   const char *name );

// Desotry the shader.
void GPU_shader_free ( GPU_Shader *shader );

// Installs the specified the shader object as part of current rendering state.
void GPU_shader_bind ( GPU_Shader *shader );

// Restores the default shader object to the current rendering state.
void GPU_shader_unbind ( );

// Retrieves the name of the specified shader object.
const char *GPU_shader_get_name ( GPU_Shader *shader );

// Deprecated function, returns the program of the shader.
int GPU_shader_get_program ( GPU_Shader *shader );

GPU_Shader *GPU_shader_create_from_info ( const GPU_ShaderCreateInfo *_info );

GPU_Shader *GPU_shader_create_from_info_name ( const char *info_name );

// Builtin Uniforms

#define GPU_UNIFORM_MODEL			0x00000000 // ModelMatrix [Mat4]
#define GPU_UNIFORM_VIEW			0x00000001 // ViewMatrix [Mat4]
#define GPU_UNIFORM_MODELVIEW			0x00000002 // ModelViewMatrix [Mat4]
#define GPU_UNIFORM_PROJECTION			0x00000003 // ProjectionMatrix [Mat4]
#define GPU_UNIFORM_VIEWPROJECTION		0x00000004 // ViewProjectionMatrix [Mat4]
#define GPU_UNIFORM_MVP				0x00000005 // ModelViewProjectionMatrix [Mat4]

#define GPU_UNIFORM_MODEL_INV			0x00000006 // ModelMatrixInverse [Mat4]
#define GPU_UNIFORM_VIEW_INV			0x00000007 // ViewMatrixInverse [Mat4]
#define GPU_UNIFORM_MODELVIEW_INV		0x00000008 // ModelViewMatrixInverse [Mat4]
#define GPU_UNIFORM_PROJECTION_INV		0x00000009 // ProjectionMatrixInverse [Mat4]
#define GPU_UNIFORM_VIEWPROJECTION_INV		0x0000000A // ViewProjectionMatrixInverse [Mat4]

#define GPU_UNIFORM_NORMAL			0x0000000B // NormalMatrix [Mat3]
#define GPU_UNIFORM_ORCO			0x0000000C // OrcoTexCoFactors [Vec4 *]
#define GPU_UNIFORM_CLIPPLANES			0x0000000D // WorldClipPlanes [Vec4 *]

#define GPU_UNIFORM_COLOR			0x0000000E // color [Vec4]
#define GPU_UNIFORM_BASE_INSTANCE		0x0000000F // baseInstance [Int]
#define GPU_UNIFORM_RESOURCE_CHUNK		0x00000010 // resourceChunk [Int]
#define GPU_UNIFORM_RESOURCE_ID			0x00000011 // resourceId [Int]
#define GPU_UNIFORM_SRGB_TRANSFORM		0x00000012 // srgbTarget [Bool]

#define GPU_NUM_UNIFORMS			0x00000013 // Special value, denotes number of builtin uniforms.

// Builtin Uniform Blocks

#define GPU_UNIFORM_BLOCK_VIEW			0x00000000 // viewBlock
#define GPU_UNIFORM_BLOCK_MODEL			0x00000001 // modelBlock
#define GPU_UNIFORM_BLOCK_INFO			0x00000002 // infoBlock

#define GPU_UNIFORM_BLOCK_DRW_VIEW		0x00000003
#define GPU_UNIFORM_BLOCK_DRW_MODEL		0x00000004
#define GPU_UNIFORM_BLOCK_DRW_INFO		0x00000005

#define GPU_NUM_UNIFORM_BLOCKS			0x00000006 // Special value, denotes number of builtin uniforms block.

// Builtin Storage Buffers

#define GPU_STORAGE_BUFFER_DEBUG_VERTS		0x00000000 // drw_debug_verts_buf
#define GPU_STORAGE_BUFFER_DEBUG_PRINT		0x00000001 // drw_debug_print_buf

#define GPU_NUM_STORAGE_BUFFERS			0x00000002 // Special value, denotes number of builtin buffer blocks.

void GPU_shader_set_srgb_uniform ( GPU_Shader *shader );

int GPU_shader_get_uniform ( GPU_Shader *shader , const char *name );
int GPU_shader_get_builtin_uniform ( GPU_Shader *shader , int builtin );
int GPU_shader_get_builtin_block ( GPU_Shader *shader , int builtin );
int GPU_shader_get_builtin_ssbo ( GPU_Shader *shader , int builtin );

// DEPRECATED
int GPU_shader_get_uniform_block ( GPU_Shader *shader , const char *name );

// DEPRECATED
int GPU_shader_get_ssbo ( GPU_Shader *shader , const char *name );

int GPU_shader_get_uniform_block_binding ( GPU_Shader *shader , const char *name );

int GPU_shader_get_texture_binding ( GPU_Shader *shader , const char *name );

void GPU_shader_uniform_vector ( GPU_Shader *shader , int location , int length , int arraysize , const float *value );
void GPU_shader_uniform_vector_int ( GPU_Shader *shader , int location , int length , int arraysize , const int *value );

void GPU_shader_uniform_float ( GPU_Shader *shader , int location , float value );
void GPU_shader_uniform_int ( GPU_Shader *shader , int location , int value );

void GPU_shader_uniform_1i ( GPU_Shader *sh , const char *name , int value );
void GPU_shader_uniform_1b ( GPU_Shader *sh , const char *name , bool value );
void GPU_shader_uniform_1f ( GPU_Shader *sh , const char *name , float value );
void GPU_shader_uniform_2f ( GPU_Shader *sh , const char *name , float x , float y );
void GPU_shader_uniform_3f ( GPU_Shader *sh , const char *name , float x , float y , float z );
void GPU_shader_uniform_4f ( GPU_Shader *sh , const char *name , float x , float y , float z , float w );
void GPU_shader_uniform_2fv ( GPU_Shader *sh , const char *name , const float data [ 2 ] );
void GPU_shader_uniform_3fv ( GPU_Shader *sh , const char *name , const float data [ 3 ] );
void GPU_shader_uniform_4fv ( GPU_Shader *sh , const char *name , const float data [ 4 ] );
void GPU_shader_uniform_2iv ( GPU_Shader *sh , const char *name , const int data [ 2 ] );
void GPU_shader_uniform_mat4 ( GPU_Shader *sh , const char *name , const float data [ 4 ][ 4 ] );
void GPU_shader_uniform_mat3_as_mat4 ( GPU_Shader *sh , const char *name , const float data [ 3 ][ 3 ] );
void GPU_shader_uniform_2fv_array ( GPU_Shader *sh , const char *name , int len , const float ( *val ) [ 2 ] );
void GPU_shader_uniform_4fv_array ( GPU_Shader *sh , const char *name , int len , const float ( *val ) [ 4 ] );

unsigned int GPU_shader_get_attribute_len ( const GPU_Shader *shader );

int GPU_shader_get_attribute ( GPU_Shader *shader , const char *name );

// Extract attribute information, recommended length of #r_name is 256
bool GPU_shader_get_attribute_info ( const GPU_Shader *shader , int attr_location , char *r_name , int *r_type );

void GPU_shader_set_framebuffer_srgb_target ( int use_srgb_to_linear );
