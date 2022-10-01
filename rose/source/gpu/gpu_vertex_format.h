#pragma once

#include "lib/lib_alloc.h"
#include "lib/lib_types.h"

#include "gpu_common.h"
#include "gpu_info.h"

#define GPU_VERT_ATTR_MAX_LEN 16
#define GPU_VERT_ATTR_MAX_NAMES 6
#define GPU_VERT_ATTR_NAMES_BUF_LEN 256
#define GPU_VERT_FORMAT_MAX_NAMES 63 /* More than enough, actual max is ~30. */
/* Computed as GPU_VERT_ATTR_NAMES_BUF_LEN / 30 (actual max format name). */
#define GPU_MAX_SAFE_ATTR_NAME 12

#define GPU_COMP_I8			ROSE_COMP_I8
#define GPU_COMP_U8			ROSE_COMP_U8
#define GPU_COMP_I16			ROSE_COMP_I16
#define GPU_COMP_U16			ROSE_COMP_U16
#define GPU_COMP_I32			ROSE_COMP_I32
#define GPU_COMP_U32			ROSE_COMP_U32
#define GPU_COMP_F32			ROSE_COMP_F32
#define GPU_COMP_I10			ROSE_COMP_I10

#define GPU_FETCH_FLOAT			0x00000000
#define GPU_FETCH_INT			0x00000001
#define GPU_FETCH_INT_TO_FLOAT_UNIT	0x00000002 // 127 (ubyte) -> 0.5 (and so on for other int types)
#define GPU_FETCH_INT_TO_FLOAT		0x00000003 // 127 (any int type) -> 127.0

typedef struct GPU_VertAttr {
	unsigned int FetchMode : 4;
	unsigned int CompType;
	unsigned int CompLen : 5;
	unsigned int Size : 8;
	unsigned int Offset : 12;
	unsigned int NameLen : 4;

	unsigned char Names [ GPU_VERT_ATTR_MAX_NAMES ];
};

typedef struct GPU_VertFormat {
	unsigned int AttrLen : 5;
	unsigned int NameLen : 6;
	unsigned int Stride : 11;
	unsigned int Packed : 1;
	unsigned int NameOffset : 8;
	unsigned int Deinterleaved : 1;

	GPU_VertAttr Attrs [ GPU_VERT_ATTR_MAX_LEN ];
	char Names [ GPU_VERT_ATTR_NAMES_BUF_LEN ];
};

struct GPU_Shader;

// Create empty vertex format.
inline GPU_VertFormat *GPU_vertformat_create ( ) {
	return ( GPU_VertFormat * ) MEM_callocN ( sizeof ( GPU_VertFormat ) , __FUNCTION__ );
}

inline void GPU_vertformat_free ( GPU_VertFormat *format ) {
	MEM_SAFE_FREE ( format );
}

// Remove every attribute from the vertex format.
void GPU_vertformat_clear ( GPU_VertFormat * );

// Copy vertex format.
void GPU_vertformat_copy ( GPU_VertFormat *dest , const GPU_VertFormat *src );

// Create vertex format from shader.
void GPU_vertformat_from_shader ( GPU_VertFormat *format , const struct GPU_Shader *shader );

/**
 * Insert new attribute to vertex format
 * \param format The format in which we want to insert the attribute.
 * \param name The name of the attribute we want to insert.
 * \param comp_type The type of the attribute's components, this can be any of the GPU_COMP_* defines.
 * \param comp_len The number of components the attribute has, for example vec3 has 3xfloat.
 * \param fetch_mode How to handle the data in the shader program.
 */
unsigned int GPU_vertformat_attr_add ( GPU_VertFormat *format ,
				       const char *name ,
				       unsigned int comp_type ,
				       unsigned int comp_len ,
				       unsigned int fetch_mode );

void GPU_vertformat_alias_add ( GPU_VertFormat *format , const char *alias );

/**
 * Makes vertex attribute from the next vertices to be accessible in the vertex shader.
 * For an attribute named "attr" you can access the next nth vertex using "attr{number}".
 * Use this function after specifying all the attributes in the format.
 *
 * NOTE: This does NOT work when using indexed rendering.
 * NOTE: Only works for first attribute name. (this limitation can be changed if needed)
 *
 * WARNING: this function creates a lot of aliases/attributes, make sure to keep the attribute
 * name short to avoid overflowing the name-buffer.
 */
void GPU_vertformat_multiload_enable ( GPU_VertFormat *format , int load_count );

/**
 * Make attribute layout non-interleaved.
 * Warning! This does not change data layout!
 * Use direct buffer access to fill the data.
 * This is for advanced usage.
 *
 * De-interleaved data means all attribute data for each attribute
 * is stored continuously like this:
 * 000011112222
 * instead of:
 * 012012012012
 *
 * \note This is per attribute de-interleaving, NOT per component.
 */
void GPU_vertformat_deinterleave ( GPU_VertFormat *format );

// Find specified attribute by name.
int GPU_vertformat_attr_id_get ( const GPU_VertFormat *format , const char *name );

inline const char *GPU_vertformat_attr_name_get ( const GPU_VertFormat *format , const GPU_VertAttr *attr , unsigned int n_idx ) {
	return format->Names + attr->Names [ n_idx ];
}

/**
 * \warning Can only rename using a string with same character count.
 * \warning This removes all other aliases of this attribute.
 */
void GPU_vertformat_attr_rename ( GPU_VertFormat *format , int attr , const char *new_name );
