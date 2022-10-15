#include "gpu/gpu_immediate.h"
#include "gpu/gpu_texture.h"
#include "gpu/gpu_matrix.h"
#include "gpu/gpu_state.h"

#include "gpu_context_private.h"
#include "gpu_immediate_private.h"
#include "gpu_shader_private.h"
#include "gpu_vertex_format_private.h"

#include "lib/lib_error.h"
#include "lib/lib_math.h"

using namespace rose::gpu;

static thread_local Immediate *imm = nullptr;

namespace rose {
namespace gpu {

}
}

void immActivate ( ) {
	imm = Context::Get ( )->Imm;
}

void immDeactivate ( ) {
	imm = nullptr;
}

GPU_VertFormat *immVertexFormat ( ) {
	GPU_vertformat_clear ( &imm->mVertexFormat );
	return &imm->mVertexFormat;
}

void immBindShader ( GPU_Shader *shader ) {
	LIB_assert ( imm->mShader == nullptr );

	imm->mShader = shader;

	if ( !imm->mVertexFormat.Packed ) {
		gpu_vertex_format_pack ( &imm->mVertexFormat );
		imm->mEnabledAttrBits = 0xFFFFu & ~( 0xFFFFu << imm->mVertexFormat.AttrLen );
	}

	GPU_shader_bind ( shader );
	GPU_matrix_bind ( shader );
	GPU_shader_set_srgb_uniform ( shader );
}

void immUnbindProgram ( ) {
	LIB_assert ( imm->mShader != nullptr );

	GPU_shader_unbind ( );
	imm->mShader = nullptr;
}

static void wide_line_workaround_start ( int prim ) {
	if ( !ELEM ( prim , GPU_PRIM_LINES , GPU_PRIM_LINE_STRIP , GPU_PRIM_LINE_LOOP ) ) {
		return;
	}

	float line_width = GPU_line_width_get ( );

	if ( line_width == 1.0f ) {
		// no need to change the shader.
		return;
	}

	fprintf ( stderr , "WARNING! %s is not implemented.\n" , __func__ );
}

static void wide_line_workaround_end ( ) {

}

void immBegin ( int prim , unsigned int vlen ) {
	LIB_assert ( imm->mPrimType == GPU_PRIM_NONE ); // Make sure we haven't already begun.

	wide_line_workaround_start ( prim );

	imm->mPrimType = prim;
	imm->mVertexLen = vlen;
	imm->mVertexIdx = 0;
	imm->mUnassignedAttrBits = imm->mEnabledAttrBits;

	imm->mVertexData = imm->Begin ( );
}

void immBeginAtMost ( int prim , unsigned int vlen ) {
	LIB_assert ( vlen > 0 );

	imm->mStrictVertexLen = false;
	immBegin ( prim , vlen );
}

GPU_Batch *immBeginBatch ( int prim , unsigned int vlen ) {
	LIB_assert ( imm->mPrimType == GPU_PRIM_NONE ); // Make sure we haven't already begun.
	
	imm->mPrimType = prim;
	imm->mVertexLen = vlen;
	imm->mVertexIdx = 0;
	imm->mUnassignedAttrBits = imm->mEnabledAttrBits;

	GPU_VertBuf *verts = GPU_vertbuf_create_with_format ( &imm->mVertexFormat );
	GPU_vertbuf_data_alloc ( verts , vlen );

	imm->mVertexData = ( unsigned char * ) GPU_vertbuf_get_data ( verts );

	imm->mBatch = GPU_batch_create_ex ( prim , verts , nullptr , GPU_BATCH_OWNS_VBO );
	imm->mBatch->Flag |= GPU_BATCH_BUILDING;

	return imm->mBatch;
}

GPU_Batch *immBeginBatchAtMost ( int prim , unsigned int vlen ) {
	LIB_assert ( vlen > 0 );

	imm->mStrictVertexLen = false;
	return immBeginBatch ( prim , vlen );
}

void immEnd ( ) {
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE ); // Make sure we're between a Begin/End pair.

	if ( imm->mStrictVertexLen ) {
		LIB_assert ( imm->mVertexIdx == imm->mVertexLen ); // With all vertices defined.
	} else {
		LIB_assert ( imm->mVertexIdx <= imm->mVertexLen );
	}

	if ( imm->mBatch ) {
		if ( imm->mVertexIdx < imm->mVertexLen ) {
			GPU_vertbuf_data_resize ( imm->mBatch->Verts [ 0 ] , imm->mVertexIdx );
			// TODO: resize only if vertex count is much smaller
		}
		GPU_batch_set_shader ( imm->mBatch , imm->mShader );
		imm->mBatch->Flag &= ~GPU_BATCH_BUILDING;
		imm->mBatch = nullptr; // Don't free, batch belongs to caller
	} else {
		imm->End ( );
	}

	// Prepare for next immBegin.
	imm->mPrimType = GPU_PRIM_NONE;
	imm->mStrictVertexLen = true;
	imm->mVertexData = nullptr;

	wide_line_workaround_end ( );
}

static void setAttrValueBit ( unsigned int attr_id ) {
	uint16_t mask = 1 << attr_id;
	LIB_assert ( imm->mUnassignedAttrBits & mask ); // not already set
	imm->mUnassignedAttrBits &= ~mask;
}

// generic attribute functions

void immAttr1f ( unsigned int attr_id , float x ) {
	GPU_VertAttr *attr = &imm->mVertexFormat.Attrs [ attr_id ];
	LIB_assert ( attr_id < imm->mVertexFormat.AttrLen );
	LIB_assert ( attr->CompType == GPU_COMP_F32 );
	LIB_assert ( attr->CompLen == 1 );
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE );
	setAttrValueBit ( attr_id );

	float *data = ( float * ) ( imm->mVertexData + attr->Offset );
	data [ 0 ] = x;
}

void immAttr2f ( unsigned int attr_id , float x , float y ) {
	GPU_VertAttr *attr = &imm->mVertexFormat.Attrs [ attr_id ];
	LIB_assert ( attr_id < imm->mVertexFormat.AttrLen );
	LIB_assert ( attr->CompType == GPU_COMP_F32 );
	LIB_assert ( attr->CompLen == 2 );
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE );
	setAttrValueBit ( attr_id );

	float *data = ( float * ) ( imm->mVertexData + attr->Offset );
	data [ 0 ] = x;
	data [ 1 ] = y;
}

void immAttr3f ( unsigned int attr_id , float x , float y , float z ) {
	GPU_VertAttr *attr = &imm->mVertexFormat.Attrs [ attr_id ];
	LIB_assert ( attr_id < imm->mVertexFormat.AttrLen );
	LIB_assert ( attr->CompType == GPU_COMP_F32 );
	LIB_assert ( attr->CompLen == 3 );
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE );
	setAttrValueBit ( attr_id );

	float *data = ( float * ) ( imm->mVertexData + attr->Offset );
	data [ 0 ] = x;
	data [ 1 ] = y;
	data [ 2 ] = z;
}

void immAttr4f ( unsigned int attr_id , float x , float y , float z , float w ) {
	GPU_VertAttr *attr = &imm->mVertexFormat.Attrs [ attr_id ];
	LIB_assert ( attr_id < imm->mVertexFormat.AttrLen );
	LIB_assert ( attr->CompType == GPU_COMP_F32 );
	LIB_assert ( attr->CompLen == 4 );
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE );
	setAttrValueBit ( attr_id );

	float *data = ( float * ) ( imm->mVertexData + attr->Offset );
	data [ 0 ] = x;
	data [ 1 ] = y;
	data [ 2 ] = y;
	data [ 3 ] = y;
}

void immAttr1u ( unsigned int attr_id , unsigned int x ) {
	GPU_VertAttr *attr = &imm->mVertexFormat.Attrs [ attr_id ];
	LIB_assert ( attr_id < imm->mVertexFormat.AttrLen );
	LIB_assert ( attr->CompType == GPU_COMP_I32 );
	LIB_assert ( attr->CompLen == 1 );
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE );
	setAttrValueBit ( attr_id );

	unsigned int *data = ( unsigned int * ) ( imm->mVertexData + attr->Offset );
	data [ 0 ] = x;
}

void immAttr1i ( unsigned int attr_id , int x ) {
	GPU_VertAttr *attr = &imm->mVertexFormat.Attrs [ attr_id ];
	LIB_assert ( attr_id < imm->mVertexFormat.AttrLen );
	LIB_assert ( attr->CompType == GPU_COMP_I32 );
	LIB_assert ( attr->CompLen == 1 );
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE );
	setAttrValueBit ( attr_id );

	int *data = ( int * ) ( imm->mVertexData + attr->Offset );
	data [ 0 ] = x;
}

void immAttr2i ( unsigned int attr_id , int x , int y ) {
	GPU_VertAttr *attr = &imm->mVertexFormat.Attrs [ attr_id ];
	LIB_assert ( attr_id < imm->mVertexFormat.AttrLen );
	LIB_assert ( attr->CompType == GPU_COMP_I32 );
	LIB_assert ( attr->CompLen == 2 );
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE );
	setAttrValueBit ( attr_id );

	int *data = ( int * ) ( imm->mVertexData + attr->Offset );
	data [ 0 ] = x;
	data [ 1 ] = y;
}

void immAttr3i ( unsigned int attr_id , int x , int y , int z ) {
	GPU_VertAttr *attr = &imm->mVertexFormat.Attrs [ attr_id ];
	LIB_assert ( attr_id < imm->mVertexFormat.AttrLen );
	LIB_assert ( attr->CompType == GPU_COMP_I32 );
	LIB_assert ( attr->CompLen == 3 );
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE );
	setAttrValueBit ( attr_id );

	int *data = ( int * ) ( imm->mVertexData + attr->Offset );
	data [ 0 ] = x;
	data [ 1 ] = y;
	data [ 2 ] = z;
}

void immAttr4i ( unsigned int attr_id , int x , int y , int z , int w ) {
	GPU_VertAttr *attr = &imm->mVertexFormat.Attrs [ attr_id ];
	LIB_assert ( attr_id < imm->mVertexFormat.AttrLen );
	LIB_assert ( attr->CompType == GPU_COMP_I32 );
	LIB_assert ( attr->CompLen == 4 );
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE );
	setAttrValueBit ( attr_id );

	int *data = ( int * ) ( imm->mVertexData + attr->Offset );
	data [ 0 ] = x;
	data [ 1 ] = y;
	data [ 2 ] = y;
	data [ 3 ] = y;
}

void immAttr1s ( unsigned int attr_id , short x ) {
	GPU_VertAttr *attr = &imm->mVertexFormat.Attrs [ attr_id ];
	LIB_assert ( attr_id < imm->mVertexFormat.AttrLen );
	LIB_assert ( attr->CompType == GPU_COMP_I16 );
	LIB_assert ( attr->CompLen == 1 );
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE );
	setAttrValueBit ( attr_id );

	short *data = ( short * ) ( imm->mVertexData + attr->Offset );
	data [ 0 ] = x;
}

void immAttr2s ( unsigned int attr_id , short x , short y ) {
	GPU_VertAttr *attr = &imm->mVertexFormat.Attrs [ attr_id ];
	LIB_assert ( attr_id < imm->mVertexFormat.AttrLen );
	LIB_assert ( attr->CompType == GPU_COMP_I16 );
	LIB_assert ( attr->CompLen == 2 );
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE );
	setAttrValueBit ( attr_id );

	short *data = ( short * ) ( imm->mVertexData + attr->Offset );
	data [ 0 ] = x;
	data [ 1 ] = y;
}

void immAttr3s ( unsigned int attr_id , int x , int y , int z ) {
	GPU_VertAttr *attr = &imm->mVertexFormat.Attrs [ attr_id ];
	LIB_assert ( attr_id < imm->mVertexFormat.AttrLen );
	LIB_assert ( attr->CompType == GPU_COMP_I16 );
	LIB_assert ( attr->CompLen == 3 );
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE );
	setAttrValueBit ( attr_id );

	short *data = ( short * ) ( imm->mVertexData + attr->Offset );
	data [ 0 ] = x;
	data [ 1 ] = y;
	data [ 2 ] = z;
}

void immAttr4s ( unsigned int attr_id , short x , short y , short z , short w ) {
	GPU_VertAttr *attr = &imm->mVertexFormat.Attrs [ attr_id ];
	LIB_assert ( attr_id < imm->mVertexFormat.AttrLen );
	LIB_assert ( attr->CompType == GPU_COMP_I16 );
	LIB_assert ( attr->CompLen == 4 );
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE );
	setAttrValueBit ( attr_id );

	short *data = ( short * ) ( imm->mVertexData + attr->Offset );
	data [ 0 ] = x;
	data [ 1 ] = y;
	data [ 2 ] = y;
	data [ 3 ] = y;
}

void immAttr2fv ( unsigned int attr_id , const float data [ 2 ] ) {
	immAttr2f ( attr_id , UNPACK2 ( data ) );
}

void immAttr3fv ( unsigned int attr_id , const float data [ 3 ] ) {
	immAttr3f ( attr_id , UNPACK3 ( data ) );
}

void immAttr4fv ( unsigned int attr_id , const float data [ 4 ] ) {
	immAttr4f ( attr_id , UNPACK4 ( data ) );
}

void immAttr1ub ( unsigned int attr_id , unsigned char x ) {
	GPU_VertAttr *attr = &imm->mVertexFormat.Attrs [ attr_id ];
	LIB_assert ( attr_id < imm->mVertexFormat.AttrLen );
	LIB_assert ( attr->CompType == GPU_COMP_U8 );
	LIB_assert ( attr->CompLen == 1 );
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE );
	setAttrValueBit ( attr_id );

	unsigned char *data = ( unsigned char * ) ( imm->mVertexData + attr->Offset );
	data [ 0 ] = x;
}

void immAttr2ub ( unsigned int attr_id , unsigned char x , unsigned char y ) {
	GPU_VertAttr *attr = &imm->mVertexFormat.Attrs [ attr_id ];
	LIB_assert ( attr_id < imm->mVertexFormat.AttrLen );
	LIB_assert ( attr->CompType == GPU_COMP_U8 );
	LIB_assert ( attr->CompLen == 2 );
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE );
	setAttrValueBit ( attr_id );

	unsigned char *data = ( unsigned char * ) ( imm->mVertexData + attr->Offset );
	data [ 0 ] = x;
	data [ 1 ] = y;
}

void immAttr3ub ( unsigned int attr_id , unsigned char x , unsigned char y , unsigned char z ) {
	GPU_VertAttr *attr = &imm->mVertexFormat.Attrs [ attr_id ];
	LIB_assert ( attr_id < imm->mVertexFormat.AttrLen );
	LIB_assert ( attr->CompType == GPU_COMP_U8 );
	LIB_assert ( attr->CompLen == 3 );
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE );
	setAttrValueBit ( attr_id );

	unsigned char *data = ( unsigned char * ) ( imm->mVertexData + attr->Offset );
	data [ 0 ] = x;
	data [ 1 ] = y;
	data [ 2 ] = z;
}

void immAttr4ub ( unsigned int attr_id , unsigned char x , unsigned char y , unsigned char z , unsigned char w ) {
	GPU_VertAttr *attr = &imm->mVertexFormat.Attrs [ attr_id ];
	LIB_assert ( attr_id < imm->mVertexFormat.AttrLen );
	LIB_assert ( attr->CompType == GPU_COMP_U8 );
	LIB_assert ( attr->CompLen == 4 );
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE );
	setAttrValueBit ( attr_id );

	unsigned char *data = ( unsigned char * ) ( imm->mVertexData + attr->Offset );
	data [ 0 ] = x;
	data [ 1 ] = y;
	data [ 2 ] = y;
	data [ 3 ] = y;
}

void immAttr3ubv ( unsigned int attr_id , const unsigned char data [ 3 ] ) {
	immAttr3ub ( attr_id , data [ 0 ] , data [ 1 ] , data [ 2 ] );
}

void immAttr4ubv ( unsigned int attr_id , const unsigned char data [ 4 ] ) {
	immAttr4ub ( attr_id , data [ 0 ] , data [ 1 ] , data [ 2 ] , data [ 3 ] );
}

void immAttrSkip ( unsigned int attr_id ) {
	LIB_assert ( attr_id < imm->mVertexFormat.AttrLen );
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE );
	setAttrValueBit ( attr_id );
}

static void immEndVertex ( ) {
	LIB_assert ( imm->mPrimType != GPU_PRIM_NONE ); /* make sure we're between a Begin/End pair */
	LIB_assert ( imm->mVertexIdx < imm->mVertexLen );

	/* Have all attributes been assigned values?
	 * If not, copy value from previous vertex. */
	if ( imm->mUnassignedAttrBits ) {
		LIB_assert ( imm->mVertexIdx > 0 ); /* first vertex must have all attributes specified */
		for ( unsigned int a_idx = 0; a_idx < imm->mVertexFormat.AttrLen; a_idx++ ) {
			if ( ( imm->mUnassignedAttrBits >> a_idx ) & 1 ) {
				const GPU_VertAttr *a = &imm->mVertexFormat.Attrs [ a_idx ];

#if 0
				printf ( "copying %s from vertex %u to %u\n" , a->name , imm->vertex_idx - 1 , imm->vertex_idx );
#endif

				unsigned char *data = imm->mVertexData + a->Offset;
				memcpy ( data , data - imm->mVertexFormat.Stride , a->Size );
				/* TODO: consolidate copy of adjacent attributes */
			}
		}
	}

	imm->mVertexIdx++;
	imm->mVertexData += imm->mVertexFormat.Stride;
	imm->mUnassignedAttrBits = imm->mEnabledAttrBits;
}

void immVertex2f ( unsigned int attr_id , float x , float y ) {
	immAttr2f ( attr_id , x , y );
	immEndVertex ( );
}

void immVertex3f ( unsigned int attr_id , float x , float y , float z ) {
	immAttr3f ( attr_id , x , y , z );
	immEndVertex ( );
}

void immVertex4f ( unsigned int attr_id , float x , float y , float z , float w ) {
	immAttr4f ( attr_id , x , y , z , w );
	immEndVertex ( );
}

void immVertex2i ( unsigned int attr_id , int x , int y ) {
	immAttr2i ( attr_id , x , y );
	immEndVertex ( );
}

void immVertex2s ( unsigned int attr_id , short x , short y ) {
	immAttr2s ( attr_id , x , y );
	immEndVertex ( );
}

void immVertex2fv ( unsigned int attr_id , const float data [ 2 ] ) {
	immAttr2f ( attr_id , data [ 0 ] , data [ 1 ] );
	immEndVertex ( );
}

void immVertex3fv ( unsigned int attr_id , const float data [ 3 ] ) {
	immAttr3f ( attr_id , data [ 0 ] , data [ 1 ] , data [ 2 ] );
	immEndVertex ( );
}

void immVertex2iv ( unsigned int attr_id , const int data [ 2 ] ) {
	immAttr2i ( attr_id , data [ 0 ] , data [ 1 ] );
	immEndVertex ( );
}

// generic uniform functions

void immUniform1f ( const char *name , float x ) {
	GPU_shader_uniform_1f ( imm->mShader , name , x );
}

void immUniform2f ( const char *name , float x , float y ) {
	GPU_shader_uniform_2f ( imm->mShader , name , x , y );
}

void immUniform2fv ( const char *name , const float data [ 2 ] ) {
	GPU_shader_uniform_2fv ( imm->mShader , name , data );
}

void immUniform3f ( const char *name , float x , float y , float z ) {
	GPU_shader_uniform_3f ( imm->mShader , name , x , y , z );
}

void immUniform3fv ( const char *name , const float data [ 3 ] ) {
	GPU_shader_uniform_3fv ( imm->mShader , name , data );
}

void immUniform4f ( const char *name , float x , float y , float z , float w ) {
	GPU_shader_uniform_4f ( imm->mShader , name , x , y , z , w );
}

void immUniform4fv ( const char *name , const float data [ 4 ] ) {
	GPU_shader_uniform_4fv ( imm->mShader , name , data );
}

void immUniformArray4fv ( const char *name , const float *data , int count ) {
	GPU_shader_uniform_4fv_array ( imm->mShader , name , count , ( const float ( * ) [ 4 ] )data );
}

void immUniformMatrix4fv ( const char *name , const float data [ 4 ][ 4 ] ) {
	GPU_shader_uniform_mat4 ( imm->mShader , name , data );
}

void immUniform1i ( const char *name , int x ) {
	GPU_shader_uniform_1i ( imm->mShader , name , x );
}

void immBindTexture ( const char *name , GPU_Texture *tex ) {
	int binding = GPU_shader_get_texture_binding ( imm->mShader , name );
	GPU_texture_bind ( tex , binding );
}

void immBindTextureSampler ( const char *name , GPU_Texture *tex , int state ) {
	int binding = GPU_shader_get_texture_binding ( imm->mShader , name );
	GPU_texture_bind_ex ( tex , state , binding , true );
}

void immBindUniformBuf ( const char *name , GPU_UniformBuf *ubo ) {
	int binding = GPU_shader_get_uniform_block_binding ( imm->mShader , name );
	GPU_uniformbuf_bind ( ubo , binding );
}

// convenience functions for setting "uniform vec4 color"

void immUniformColor4f ( float r , float g , float b , float a ) {
	int32_t uniform_loc = GPU_shader_get_builtin_uniform ( imm->mShader , GPU_UNIFORM_COLOR );
	LIB_assert ( uniform_loc != -1 );
	float data [ 4 ] = { r, g, b, a };
	GPU_shader_uniform_vector ( imm->mShader , uniform_loc , 4 , 1 , data );
	/* For wide Line workaround. */
	copy_v4_v4 ( imm->mUniformColor , data );
}

void immUniformColor4fv ( const float rgba [ 4 ] ) {
	immUniformColor4f ( rgba [ 0 ] , rgba [ 1 ] , rgba [ 2 ] , rgba [ 3 ] );
}

void immUniformColor3f ( float r , float g , float b ) {
	immUniformColor4f ( r , g , b , 1.0f );
}

void immUniformColor3fv ( const float rgb [ 3 ] ) {
	immUniformColor4f ( rgb [ 0 ] , rgb [ 1 ] , rgb [ 2 ] , 1.0f );
}

void immUniformColor3fvAlpha ( const float rgb [ 3 ] , float a ) {
	immUniformColor4f ( rgb [ 0 ] , rgb [ 1 ] , rgb [ 2 ] , a );
}

void immUniformColor3ub ( unsigned char r , unsigned char g , unsigned char b ) {
	const float scale = 1.0f / 255.0f;
	immUniformColor4f ( scale * r , scale * g , scale * b , 1.0f );
}

void immUniformColor4ub ( unsigned char r , unsigned char g , unsigned char b , unsigned char a ) {
	const float scale = 1.0f / 255.0f;
	immUniformColor4f ( scale * r , scale * g , scale * b , scale * a );
}

void immUniformColor3ubv ( const unsigned char rgb [ 3 ] ) {
	immUniformColor3ub ( rgb [ 0 ] , rgb [ 1 ] , rgb [ 2 ] );
}

void immUniformColor3ubvAlpha ( const unsigned char rgb [ 3 ] , unsigned char alpha ) {
	immUniformColor4ub ( rgb [ 0 ] , rgb [ 1 ] , rgb [ 2 ] , alpha );
}

void immUniformColor4ubv ( const unsigned char rgba [ 4 ] ) {
	immUniformColor4ub ( rgba [ 0 ] , rgba [ 1 ] , rgba [ 2 ] , rgba [ 3 ] );
}

//

GPU_Shader *immGetShader ( ) {
	return imm->mShader;
}
