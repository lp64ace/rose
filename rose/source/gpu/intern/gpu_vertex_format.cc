#include "gpu_vertex_format_private.h"
#include "gpu_shader_create_info.h"

#include <string.h>
#include <stdio.h>

using namespace rose::gpu;

void GPU_vertformat_clear ( GPU_VertFormat *format ) {
#ifdef TRUST_NO_ONE
	memset ( format , 0 , sizeof ( GPU_VertFormat ) );
#else
	format->AttrLen = 0;
	format->Packed = false;
	format->NameOffset = 0;
	format->NameLen = 0;
	format->Deinterleaved = false;

	for ( unsigned int i = 0; i < GPU_VERT_ATTR_MAX_LEN; i++ ) {
		format->Attrs [ i ].NameLen = 0;
	}
#endif
}

void GPU_vertformat_copy ( GPU_VertFormat *dest , const GPU_VertFormat *src ) {
	// copy regular struct fields.
	memcpy ( dest , src , sizeof ( GPU_VertFormat ) );
}

static unsigned int comp_size ( unsigned int type ) {
#ifdef TRUST_NO_ONE
	assert ( LIB_channel_bits ( type ) % 8 == 0 );
#endif
	return LIB_channel_bits ( type ) / 8;
}

static unsigned int attr_size ( const GPU_VertAttr *a ) {
	if ( a->CompType == GPU_COMP_I10 ) {
		return 4; /* always packed as 10_10_10_2 */
	}
	return a->CompLen * comp_size ( a->CompType );
}


static unsigned int attr_align ( const GPU_VertAttr *a , unsigned int minimum_stride ) {
	if ( a->CompLen == GPU_COMP_I10 ) {
		return 4; /* always packed as 10_10_10_2 */
	}
	unsigned int c = comp_size ( a->CompType );
	if ( a->CompLen == 3 && c <= 2 ) {
		return 4 * c; /* AMD HW can't fetch these well, so pad it out (other vendors too?) */
	}

	/* Most fetches are ok if components are naturally aligned.
	 * However, in Metal,the minimum supported per-vertex stride is 4,
	 * so we must query the GPU and pad out the size accordingly. */
	return ROSE_MAX ( minimum_stride , c );
}

unsigned int vertex_buffer_size ( const GPU_VertFormat *format , unsigned int vertex_len ) {
#ifdef TRUST_NO_ONE
	assert ( format->Packed );
#endif
	return format->Stride * vertex_len;
}

static unsigned char copy_attr_name ( GPU_VertFormat *format , const char *name ) {
	/* `strncpy` does 110% of what we need; let's do exactly 100% */
	unsigned char name_offset = format->NameOffset;
	char *name_copy = format->Names + name_offset;
	unsigned int available = GPU_VERT_ATTR_NAMES_BUF_LEN - name_offset;
	bool terminated = false;

	for ( unsigned int i = 0; i < available; i++ ) {
		const char c = name [ i ];
		name_copy [ i ] = c;
		if ( c == '\0' ) {
			terminated = true;
			format->NameOffset += ( i + 1 );
			break;
		}
	}
#ifdef TRUST_NO_ONE
	assert ( terminated );
	assert ( format->NameOffset <= GPU_VERT_ATTR_NAMES_BUF_LEN );
#else
	( void ) terminated;
#endif
	return name_offset;
}

unsigned int GPU_vertformat_attr_add ( GPU_VertFormat *format ,
				       const char *name ,
				       unsigned int comp_type ,
				       unsigned int comp_len ,
				       unsigned int fetch_mode ) {
#if TRUST_NO_ONE
	assert ( format->NameLen < GPU_VERT_FORMAT_MAX_NAMES ); // there's room for more
	assert ( format->AttrLen < GPU_VERT_ATTR_MAX_LEN );     // there's room for more
	assert ( !format->Packed );                             // packed means frozen/locked
	assert ( ( comp_len >= 1 && comp_len <= 4 ) || comp_len == 8 || comp_len == 12 || comp_len == 16 );

	switch ( comp_type ) {
		case GPU_COMP_F32: {
			// float type can only kept as float
			assert ( fetch_mode == GPU_FETCH_FLOAT );
		}break;
		case GPU_COMP_I10: {
			// 10_10_10 format intended for normals (xyz) or colors (rgb)
			// extra component packed.w can be manually set to { -2, -1, 0, 1 }
			assert ( ELEM ( comp_len , 3 , 4 ) );
			// Not strictly required, may relax later.
			assert ( fetch_mode == GPU_FETCH_INT_TO_FLOAT_UNIT );

		}break;
		default: {
			/* integer types can be kept as int or converted/normalized to float */
			assert ( fetch_mode != GPU_FETCH_FLOAT );
			/* only support float matrices (see Batch_update_program_bindings) */
			assert ( !ELEM ( comp_len , 8 , 12 , 16 ) );
		}break;
	}
#endif
	format->NameLen++; /* Multi-name support. */

	const unsigned int attr_id = format->AttrLen++;
	GPU_VertAttr *attr = &format->Attrs [ attr_id ];

	attr->Names [ attr->NameLen++ ] = copy_attr_name ( format , name );
	attr->CompType = comp_type;
	attr->CompLen = ( comp_type == GPU_COMP_I10 ) ? 4 : comp_len; // system needs 10_10_10_2 to be 4 or BGRA
	attr->Size = attr_size ( attr );
	attr->Offset = 0; // offsets & stride are calculated later (during pack)
	attr->FetchMode = fetch_mode;

	return attr_id;
}

void GPU_vertformat_alias_add ( GPU_VertFormat *format , const char *alias ) {
	GPU_VertAttr *attr = &format->Attrs [ format->AttrLen - 1 ];
#ifdef TRUST_NO_ONE
	assert ( format->NameLen < GPU_VERT_FORMAT_MAX_NAMES ); /* there's room for more */
	assert ( attr->NameLen < GPU_VERT_ATTR_MAX_NAMES );
#endif
	format->NameLen++; /* Multi-name support. */
	attr->Names [ attr->NameLen++ ] = copy_attr_name ( format , alias );
}

void GPU_vertformat_multiload_enable ( GPU_VertFormat *format , int load_count ) {
	/* Sanity check. Maximum can be upgraded if needed. */
	assert ( load_count > 1 && load_count < 5 );
	/* We need a packed format because of format->stride. */
	if ( !format->Packed ) {
		gpu_vertex_format_pack ( format );
	}

	assert ( ( format->NameLen + 1 ) * load_count < GPU_VERT_FORMAT_MAX_NAMES );
	assert ( format->AttrLen * load_count <= GPU_VERT_ATTR_MAX_LEN );
	assert ( format->NameOffset * load_count < GPU_VERT_ATTR_NAMES_BUF_LEN );

	const GPU_VertAttr *attr = format->Attrs;
	int attr_len = format->AttrLen;
	for ( int i = 0; i < attr_len; i++ , attr++ ) {
		const char *attr_name = GPU_vertformat_attr_name_get ( format , attr , 0 );
		for ( int j = 1; j < load_count; j++ ) {
			char load_name [ 64 ];
			_snprintf_s ( load_name , sizeof ( load_name ) , "%s%d" , attr_name , j );
			GPU_VertAttr *dst_attr = &format->Attrs [ format->AttrLen++ ];
			*dst_attr = *attr;

			dst_attr->Names [ 0 ] = copy_attr_name ( format , load_name );
			dst_attr->NameLen = 1;
			dst_attr->Offset += format->Stride * j;
		}
	}
}

int GPU_vertformat_attr_id_get ( const GPU_VertFormat *format , const char *name ) {
	for ( int i = 0; i < format->AttrLen; i++ ) {
		const GPU_VertAttr *attr = &format->Attrs [ i ];
		for ( int j = 0; j < attr->NameLen; j++ ) {
			const char *attr_name = GPU_vertformat_attr_name_get ( format , attr , j );
			if ( strcmp ( name , attr_name ) == 0 ) {
				return i;
			}
		}
	}
	return -1;
}

void GPU_vertformat_attr_rename ( GPU_VertFormat *format , int attr_id , const char *new_name ) {
	assert ( attr_id > -1 && attr_id < format->AttrLen );
	GPU_VertAttr *attr = &format->Attrs [ attr_id ];
	char *attr_name = ( char * ) GPU_vertformat_attr_name_get ( format , attr , 0 );
	assert ( strlen ( attr_name ) == strlen ( new_name ) );
	int i = 0;
	while ( attr_name [ i ] != '\0' ) {
		attr_name [ i ] = new_name [ i ];
		i++;
	}
	attr->NameLen = 1;
}

void GPU_vertformat_deinterleave ( GPU_VertFormat *format ) {
	/* Ideally we should change the stride and offset here. This would allow
	 * us to use GPU_vertbuf_attr_set / GPU_vertbuf_attr_fill. But since
	 * we use only 11 bits for attr->offset this limits the size of the
	 * buffer considerably. So instead we do the conversion when creating
	 * bindings in create_bindings(). */
	format->Deinterleaved = true;
}

unsigned int _padding ( unsigned int offset , unsigned int alignment ) {
	if ( alignment ) {
		const unsigned int mod = offset % alignment;
		return ( mod == 0 ) ? 0 : ( alignment - mod );
	}
	return offset;
}

static void gpu_vertex_format_pack_impl ( GPU_VertFormat *format , unsigned int minimum_stride ) {
	GPU_VertAttr *a0 = &format->Attrs [ 0 ];
	a0->Offset= 0;
	unsigned int offset = a0->Size;

	for ( unsigned int a_idx = 1; a_idx < format->AttrLen; a_idx++ ) {
		GPU_VertAttr *a = &format->Attrs [ a_idx ];
		unsigned int mid_padding = _padding ( offset , attr_align ( a , minimum_stride ) );
		offset += mid_padding;
		a->Offset = offset;
		offset += a->Size;
	}

	unsigned int end_padding = _padding ( offset , attr_align ( a0 , minimum_stride ) );

	format->Stride = offset + end_padding;
	format->Packed = true;
}

void gpu_vertex_format_pack ( GPU_VertFormat *format ) {
	/* Perform standard vertex packing, ensuring vertex format satisfies
	 * minimum stride requirements for vertex assembly. */
	gpu_vertex_format_pack_impl ( format , GPU_get_info_i ( GPU_INFO_MINIMUM_PER_VERTEX_STRIDE ) );
}

void gpu_vertex_format_texture_buffer_pack ( GPU_VertFormat *format ) {
	/* Validates packing for vertex formats used with texture buffers.
	* In these cases, there must only be a single vertex attribute.
	* This attribute should be tightly packed without padding, to ensure
	* it aligns with the backing texture data format, skipping
	* minimum per-vertex stride, which mandates 4-byte alignment in Metal.
	* This additional alignment padding caused smaller data types, e.g. U16,
	* to mis-align. */
	if ( format->AttrLen != 1 ) {
		fprintf ( stderr , "Texture buffer mode should only use a single vertex attribute.\n" );
	}

	// Pack vertex format without minimum stride, as this is not required by texture buffers.
	gpu_vertex_format_pack_impl ( format , 1 );
}

static unsigned int component_size_get ( const Type gpu_type ) {
	switch ( gpu_type ) {
		case Type::VEC2:
		case Type::IVEC2:
		case Type::UVEC2:
			return 2;
		case Type::VEC3:
		case Type::IVEC3:
		case Type::UVEC3:
			return 3;
		case Type::VEC4:
		case Type::IVEC4:
		case Type::UVEC4:
			return 4;
		case Type::MAT3:
			return 12;
		case Type::MAT4:
			return 16;
		default:
			return 1;
	}
}

static void recommended_fetch_mode_and_comp_type ( Type gpu_type , unsigned int *r_comp_type , unsigned int *r_fetch_mode ) {
	switch ( gpu_type ) {
		case Type::FLOAT:
		case Type::VEC2:
		case Type::VEC3:
		case Type::VEC4:
		case Type::MAT3:
		case Type::MAT4: {
			*r_comp_type = GPU_COMP_F32;
			*r_fetch_mode = GPU_FETCH_FLOAT;
		}break;
		case Type::INT:
		case Type::IVEC2:
		case Type::IVEC3:
		case Type::IVEC4: {
			*r_comp_type = GPU_COMP_I32;
			*r_fetch_mode = GPU_FETCH_INT;
		}break;
		case Type::UINT:
		case Type::UVEC2:
		case Type::UVEC3:
		case Type::UVEC4: {
			*r_comp_type = GPU_COMP_U32;
			*r_fetch_mode = GPU_FETCH_INT;
		}break;
		default: {
			fprintf ( stderr , "Unsupported vertex attribute type" );
			assert ( 0 );
		}
	}
}

void GPU_vertformat_from_shader ( GPU_VertFormat *format , const struct GPU_Shader *gpushader ) {
	GPU_vertformat_clear ( format );

	unsigned int attr_len = GPU_shader_get_attribute_len ( gpushader );
	int location_test = 0 , attrs_added = 0;
	while ( attrs_added < attr_len && location_test < GPU_MAX_ATTR ) {
		char name [ 256 ];
		Type gpu_type;

		if ( !GPU_shader_get_attribute_info ( gpushader , location_test++ , name , ( int * ) &gpu_type ) ) {
			continue;
		}

		unsigned int comp_type;
		unsigned int fetch_mode;
		recommended_fetch_mode_and_comp_type ( gpu_type , &comp_type , &fetch_mode );

		int comp_len = component_size_get ( gpu_type );

		GPU_vertformat_attr_add ( format , name , comp_type , comp_len , fetch_mode );
	}
}
