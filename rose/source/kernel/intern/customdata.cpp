#include "kernel/rke_customdata.h"
#include "kernel/rke_anonymous_attribute.h"
#include "kernel/rke_anonymous_attribute_id.h"

#include "lib/lib_math.h"
#include "lib/lib_mempool.h"

#include <string.h>
#include <limits.h>
#include <float.h>
#include <stdbool.h>

#define CUSTOMDATA_GROW	8

struct LayerTypeInfo {
	int mSize; // The memory size of one element of this layer's data

	const char *mStructName; // Name of the struct used, for file writing.
	int mStructNum; // Number of structs per element

	/** Default layer name.
	* \note when null this is a way to ensure there is only ever one item. */
	const char *mDefaultName;

	/** A function to copy count elements of this layer's data
	* (deep copy if appropriate)
	* if null, memcpy is used */
	void ( *mCopy )( void *dst , const void *src , int count );

	/** A function to free any dynamically allocated components of this 
	* layer's data (note the data pointer itself should not be freed) 
	* size should be the size of one element of this layer's data (e.g.
	* LayerTypeInfo.mSize) */
	void ( *mFree )( void *data , int count , int size );

	/** A function to interpolate between count source elements of this 
	* layer's data and store the result in dest 
	* if weights == null or sub_weights == null, they should default to 1 
	* 
	* weights gives the weight for each element in sources 
	* sub_weights gives the sub-element weights for each element in sources 
	*    (there should be (sub element count)^2 weights per element) 
	* count gives the number of elements in sources 
	*
	* \note in some cases \a dest pointer is in \a sources
	*       so all functions have to take this into account and delay
	*       applying changes while reading from sources.
	*/
	void ( *mInterp )( const void **sources , const float *weights , const float *sub_weights , int count , void *dest );

	/** Set values to the type's default. If undefined, the default is assumed to be zeroes. 
	* Memory pointed to by #data is expected to be uninitialized. */
	void ( *mSetDefaultValue ) ( void *data , int count );

	/** Construct and fill a valid value for the type. Necessary for non-trivial types. 
	* Memory pointed to by #data is expected to be uninitialized. */
	void ( *mConstruct ) ( void *data , int count );

	/** A function used by mesh validating code, must ensures passed item has valid data. */
	bool ( *mValidate ) ( void *item , unsigned int totitems , bool do_fixes );

	// Compare functions.

	bool ( *mEqual ) ( const void *data1 , const void *data2 );
	void ( *mMultiply ) ( void *data , float fac );
	void ( *mAdd ) ( void *data1 , const void *data2 );

	/** A function to determine max allowed number of layers, 
	* should be null or return -1 if no limit. */
	int ( *mLayersMax )( );
};

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#MVert, #CD_MVERT)
 * \{ */

static bool layerEqual_mvert ( const void *data1 , const void *data2 ) {
	const MVert *n1 = ( const MVert * ) data1;
	const MVert *n2 = ( const MVert * ) data2;
	return  fabs ( n1->Coord [ 0 ] - n2->Coord [ 0 ] ) < FLT_EPSILON &&
		fabs ( n1->Coord [ 1 ] - n2->Coord [ 1 ] ) < FLT_EPSILON &&
		fabs ( n1->Coord [ 2 ] - n2->Coord [ 2 ] ) < FLT_EPSILON;
}

static void layerMultiply_mvert ( void *data , float fac ) {
	MVert *n = ( MVert * ) data;
	n->Coord [ 0 ] *= fac;
	n->Coord [ 1 ] *= fac;
	n->Coord [ 2 ] *= fac;
}

static void layerAdd_mvert ( void *data1 , const void *data2 ) {
	MVert *n1 = ( MVert * ) data1;
	MVert *n2 = ( MVert * ) data2;
	n1->Coord [ 0 ] += n2->Coord [ 0 ];
	n1->Coord [ 1 ] += n2->Coord [ 1 ];
	n1->Coord [ 2 ] += n2->Coord [ 2 ];
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#vec3f, #CD_NORMAL)
 * \{ */

static void layerInterp_normal ( const void **sources ,
				 const float *weights ,
				 const float * ,
				 const int count ,
				 void *dest ) {
	/* NOTE: This is linear interpolation, which is not optimal for vectors.
	* Unfortunately, spherical interpolation of more than two values is hairy,
	* so for now it will do... */
	float no [ 3 ] = { 0.0f };

	for ( int i = 0; i < count; i++ ) {
		madd_v3_v3fl ( no , ( const float * ) sources [ i ] , weights [ i ] );
	}

	/* Weighted sum of normalized vectors will **not** be normalized, even if weights are. */
	normalize_v3_v3 ( ( float * ) dest , no );
}

static bool layerEqual_normal ( const void *data1 , const void *data2 ) {
	const vec3f *n1 = ( const vec3f * ) data1;
	const vec3f *n2 = ( const vec3f * ) data2;
	return  fabs ( n1->x - n2->x ) < FLT_EPSILON &&
		fabs ( n1->y - n2->y ) < FLT_EPSILON &&
		fabs ( n1->z - n2->z ) < FLT_EPSILON;
}

static void layerMultiply_normal ( void *data , float fac ) {
	vec3f *n = ( vec3f * ) data;
	n->x *= fac;
	n->y *= fac;
	n->z *= fac;
}

static void layerAdd_normal ( void *data1 , const void *data2 ) {
	vec3f *n1 = ( vec3f * ) data1;
	vec3f *n2 = ( vec3f * ) data2;
	n1->x += n2->x;
	n1->y += n2->y;
	n1->z += n2->z;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#float, #CD_PROP_FLOAT)
 * \{ */

static void layerCopy_propfloat ( void *dst , const void *src , const int count ) {
	memcpy ( dst , src , sizeof ( float ) * count );
}

static void layerInterp_propfloat ( const void **sources ,
				    const float *weights ,
				    const float * ,
				    const int count ,
				    void *dest ) {
	float result = 0.0f;
	for ( int i = 0; i < count; i++ ) {
		const float interp_weight = weights [ i ];
		const float src = *( const float * ) sources [ i ];
		result += src * interp_weight;
	}
	*( float * ) dest = result;
}

static bool layerValidate_propfloat ( void *data , const unsigned int totitems , const bool do_fixes ) {
	float *fp = static_cast< float * >( data );
	bool has_errors = false;

	for ( int i = 0; i < totitems; i++ , fp++ ) {
		if ( !isfinite ( *fp ) ) {
			if ( do_fixes ) {
				*fp = 0.0f;
			}
			has_errors = true;
		}
	}

	return has_errors;
}

static bool layerEqual_propfloat ( const void *data1 , const void *data2 ) {
	return fabs ( *( const float * ) data1 - *( const float * ) data2 ) < FLT_EPSILON;
}

static void layerMultiply_propfloat ( void *data , float fac ) {
	float *fp = static_cast< float * >( data );
	*fp *= fac;
}

static void layerAdd_propfloat ( void *data1 , const void *data2 ) {
	float *fp1 = ( float * ) ( data1 );
	float *fp2 = ( float * ) ( data2 );
	*fp1 += *fp2;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#int, #CD_PROP_INT32)
 * \{ */

static void layerInterp_propint ( const void **sources ,
				  const float *weights ,
				  const float * ,
				  const int count ,
				  void *dest ) {
	float result = 0.0f;
	for ( int i = 0; i < count; i++ ) {
		const float weight = weights [ i ];
		const float src = *static_cast< const int * >( sources [ i ] );
		result += src * weight;
	}
	const int rounded_result = static_cast< int >( round ( result ) );
	*static_cast< int * >( dest ) = rounded_result;
}

static bool layerEqual_propint ( const void *data1 , const void *data2 ) {
	return memcmp ( data1 , data2 , sizeof ( int ) );
}

static void layerMultiply_propint ( void *data , float fac ) {
	float *fp = static_cast< float * >( data );
	*fp *= fac;
}

static void layerAdd_propint ( void *data1 , const void *data2 ) {
	float *fp1 = ( float * ) ( data1 );
	float *fp2 = ( float * ) ( data2 );
	*fp1 += *fp2;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#MStringProperty, #CD_PROP_STRING)
 * \{ */

static void layerCopy_propstring ( void *dest , const void *source , const int count ) {
	memcpy ( dest , source , sizeof ( MStringProperty ) * count );
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#MLoopUV, #CD_MLOOPUV)
 * \{ */

static bool layerEqual_mloopuv ( const void *data1 , const void *data2 ) {
	const MLoopUV *luv1 = static_cast< const MLoopUV * >( data1 );
	const MLoopUV *luv2 = static_cast< const MLoopUV * >( data2 );
	return len_squared_v2v2 ( luv1->TexCoord , luv2->TexCoord ) < 1e-6f;
}

static void layerMultiply_mloopuv ( void *data , const float fac ) {
	MLoopUV *luv = static_cast< MLoopUV * >( data );
	mul_v2_fl ( luv->TexCoord , fac );
}

static void layerAdd_mloopuv ( void *data1 , const void *data2 ) {
	MLoopUV *l1 = static_cast< MLoopUV * >( data1 );
	const MLoopUV *l2 = static_cast< const MLoopUV * >( data2 );

	add_v2_v2 ( l1->TexCoord , l2->TexCoord );
}

static void layerInterp_mloopuv ( const void **sources ,
				  const float *weights ,
				  const float *sub_weights ,
				  int count ,
				  void *dest ) {
	float uv [ 2 ];
	int flag = 0;

	zero_v2 ( uv );

	for ( int i = 0; i < count; i++ ) {
		const float interp_weight = weights [ i ];
		const MLoopUV *src = static_cast< const MLoopUV * >( sources [ i ] );
		madd_v2_v2fl ( uv , src->TexCoord , interp_weight );
		if ( interp_weight > 0.0f ) {
			flag |= src->Flag;
		}
	}

	/* Delay writing to the destination in case dest is in sources. */
	copy_v2_v2 ( ( ( MLoopUV * ) dest )->TexCoord , uv );
	( ( MLoopUV * ) dest )->Flag = flag;
}

static bool layerValidate_mloopuv ( void *data , const unsigned int totitems , const bool do_fixes ) {
	MLoopUV *uv = static_cast< MLoopUV * >( data );
	bool has_errors = false;

	for ( int i = 0; i < totitems; i++ , uv++ ) {
		if ( !is_finite_v2 ( uv->TexCoord ) ) {
			if ( do_fixes ) {
				zero_v2 ( uv->TexCoord );
			}
			has_errors = true;
		}
	}

	return has_errors;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#MLoopCol, #CD_PROP_BYTE_COLOR)
 * \{ */

static bool layerEqual_mloopcol ( const void *data1 , const void *data2 ) {
	const MLoopCol *m1 = static_cast< const MLoopCol * >( data1 );
	const MLoopCol *m2 = static_cast< const MLoopCol * >( data2 );
	float r , g , b , a;

	r = m1->r - m2->r;
	g = m1->g - m2->g;
	b = m1->b - m2->b;
	a = m1->a - m2->a;

	return r * r + g * g + b * b + a * a < 0.001f;
}

static void layerMultiply_mloopcol ( void *data , const float fac ) {
	MLoopCol *m = static_cast< MLoopCol * >( data );

	m->r = ( float ) m->r * fac;
	m->g = ( float ) m->g * fac;
	m->b = ( float ) m->b * fac;
	m->a = ( float ) m->a * fac;
}

static void layerAdd_mloopcol ( void *data1 , const void *data2 ) {
	MLoopCol *m = static_cast< MLoopCol * >( data1 );
	const MLoopCol *m2 = static_cast< const MLoopCol * >( data2 );

	m->r += m2->r;
	m->g += m2->g;
	m->b += m2->b;
	m->a += m2->a;
}

static void layerDoMinMax_mloopcol ( const void *data , void *vmin , void *vmax ) {
	const MLoopCol *m = static_cast< const MLoopCol * >( data );
	MLoopCol *min = static_cast< MLoopCol * >( vmin );
	MLoopCol *max = static_cast< MLoopCol * >( vmax );

	if ( m->r < min->r ) {
		min->r = m->r;
	}
	if ( m->g < min->g ) {
		min->g = m->g;
	}
	if ( m->b < min->b ) {
		min->b = m->b;
	}
	if ( m->a < min->a ) {
		min->a = m->a;
	}
	if ( m->r > max->r ) {
		max->r = m->r;
	}
	if ( m->g > max->g ) {
		max->g = m->g;
	}
	if ( m->b > max->b ) {
		max->b = m->b;
	}
	if ( m->a > max->a ) {
		max->a = m->a;
	}
}

static void layerInitMinMax_mloopcol ( void *vmin , void *vmax ) {
	MLoopCol *min = static_cast< MLoopCol * >( vmin );
	MLoopCol *max = static_cast< MLoopCol * >( vmax );

	min->r = 255;
	min->g = 255;
	min->b = 255;
	min->a = 255;

	max->r = 0;
	max->g = 0;
	max->b = 0;
	max->a = 0;
}

static void layerDefault_mloopcol ( void *data , const int count ) {
	MLoopCol default_mloopcol = { 255, 255, 255, 255 };
	MLoopCol *mlcol = ( MLoopCol * ) data;
	for ( int i = 0; i < count; i++ ) {
		mlcol [ i ] = default_mloopcol;
	}
}

static void layerInterp_mloopcol ( const void **sources ,
				   const float *weights ,
				   const float * ,
				   int count ,
				   void *dest ) {
	MLoopCol *mc = static_cast< MLoopCol * >( dest );
	struct {
		float a;
		float r;
		float g;
		float b;
	} col = { 0 };

	for ( int i = 0; i < count; i++ ) {
		const float interp_weight = weights [ i ];
		const MLoopCol *src = static_cast< const MLoopCol * >( sources [ i ] );
		col.r += src->r * interp_weight;
		col.g += src->g * interp_weight;
		col.b += src->b * interp_weight;
		col.a += src->a * interp_weight;
	}

	/* Subdivide smooth or fractal can cause problems without clamping
	 * although weights should also not cause this situation */

	 /* Also delay writing to the destination in case dest is in sources. */
	mc->r = round_fl_to_uchar_clamp ( col.r );
	mc->g = round_fl_to_uchar_clamp ( col.g );
	mc->b = round_fl_to_uchar_clamp ( col.b );
	mc->a = round_fl_to_uchar_clamp ( col.a );
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#MPropCol, #CD_PROP_COLOR)
 * \{ */

static bool layerEqual_propcol ( const void *data1 , const void *data2 ) {
	const MPropCol *m1 = static_cast< const MPropCol * >( data1 );
	const MPropCol *m2 = static_cast< const MPropCol * >( data2 );
	float tot = 0;

	for ( int i = 0; i < 4; i++ ) {
		float c = ( m1->color [ i ] - m2->color [ i ] );
		tot += c * c;
	}

	return tot < 0.001f;
}

static void layerMultiply_propcol ( void *data , const float fac ) {
	MPropCol *m = static_cast< MPropCol * >( data );
	mul_v4_fl ( m->color , fac );
}

static void layerAdd_propcol ( void *data1 , const void *data2 ) {
	MPropCol *m = static_cast< MPropCol * >( data1 );
	const MPropCol *m2 = static_cast< const MPropCol * >( data2 );
	add_v4_v4 ( m->color , m2->color );
}

static void layerDoMinMax_propcol ( const void *data , void *vmin , void *vmax ) {
	const MPropCol *m = static_cast< const MPropCol * >( data );
	MPropCol *min = static_cast< MPropCol * >( vmin );
	MPropCol *max = static_cast< MPropCol * >( vmax );
	minmax_v4v4_v4 ( min->color , max->color , m->color );
}

static void layerInitMinMax_propcol ( void *vmin , void *vmax ) {
	MPropCol *min = static_cast< MPropCol * >( vmin );
	MPropCol *max = static_cast< MPropCol * >( vmax );

	copy_v4_fl ( min->color , FLT_MAX );
	copy_v4_fl ( max->color , FLT_MIN );
}

static void layerDefault_propcol ( void *data , const int count ) {
	/* Default to white, full alpha. */
	MPropCol default_propcol = { {1.0f, 1.0f, 1.0f, 1.0f} };
	MPropCol *pcol = ( MPropCol * ) data;
	for ( int i = 0; i < count; i++ ) {
		copy_v4_v4 ( pcol [ i ].color , default_propcol.color );
	}
}

static void layerInterp_propcol ( const void **sources ,
				  const float *weights ,
				  const float * ,
				  int count ,
				  void *dest ) {
	MPropCol *mc = static_cast< MPropCol * >( dest );
	float col [ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f };
	for ( int i = 0; i < count; i++ ) {
		const float interp_weight = weights [ i ];
		const MPropCol *src = static_cast< const MPropCol * >( sources [ i ] );
		madd_v4_v4fl ( col , src->color , interp_weight );
	}
	copy_v4_v4 ( mc->color , col );
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#vec3f, #CD_PROP_FLOAT3)
 * \{ */

static void layerInterp_propfloat3 ( const void **sources ,
				     const float *weights ,
				     const float * ,
				     int count ,
				     void *dest ) {
	vec3f result = { 0.0f, 0.0f, 0.0f };
	for ( int i = 0; i < count; i++ ) {
		const float interp_weight = weights [ i ];
		const vec3f *src = static_cast< const vec3f * >( sources [ i ] );
		madd_v3_v3fl ( &result.x , &src->x , interp_weight );
	}
	copy_v3_v3 ( ( float * ) dest , &result.x );
}

static void layerMultiply_propfloat3 ( void *data , const float fac ) {
	vec3f *vec = static_cast< vec3f * >( data );
	vec->x *= fac;
	vec->y *= fac;
	vec->z *= fac;
}

static bool layerEqual_propfloat3 ( const void *data1 , const void *data2 ) {
	const vec3f *n1 = ( const vec3f * ) data1;
	const vec3f *n2 = ( const vec3f * ) data2;
	return  fabs ( n1->x - n2->x ) < FLT_EPSILON &&
		fabs ( n1->y - n2->y ) < FLT_EPSILON &&
		fabs ( n1->z - n2->z ) < FLT_EPSILON;
}

static void layerAdd_propfloat3 ( void *data1 , const void *data2 ) {
	vec3f *vec1 = static_cast< vec3f * >( data1 );
	const vec3f *vec2 = static_cast< const vec3f * >( data2 );
	vec1->x += vec2->x;
	vec1->y += vec2->y;
	vec1->z += vec2->z;
}

static bool layerValidate_propfloat3 ( void *data , const unsigned int totitems , const bool do_fixes ) {
	float *values = static_cast< float * >( data );
	bool has_errors = false;
	for ( int i = 0; i < totitems * 3; i++ ) {
		if ( !isfinite ( values [ i ] ) ) {
			if ( do_fixes ) {
				values [ i ] = 0.0f;
			}
			has_errors = true;
		}
	}
	return has_errors;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#vec2f, #CD_PROP_FLOAT2)
 * \{ */

static void layerInterp_propfloat2 ( const void **sources ,
				     const float *weights ,
				     const float *sub_weights ,
				     int count ,
				     void *dest ) {
	vec2f result = { 0.0f, 0.0f };
	for ( int i = 0; i < count; i++ ) {
		const float interp_weight = weights [ i ];
		const vec2f *src = static_cast< const vec2f * >( sources [ i ] );
		madd_v2_v2fl ( &result.x , &src->x , interp_weight );
	}
	copy_v2_v2 ( ( float * ) dest , &result.x );
}

static void layerMultiply_propfloat2 ( void *data , const float fac ) {
	vec2f *vec = static_cast< vec2f * >( data );
	vec->x *= fac;
	vec->y *= fac;
}

static void layerAdd_propfloat2 ( void *data1 , const void *data2 ) {
	vec2f *vec1 = static_cast< vec2f * >( data1 );
	const vec2f *vec2 = static_cast< const vec2f * >( data2 );
	vec1->x += vec2->x;
	vec1->y += vec2->y;
}

static bool layerValidate_propfloat2 ( void *data , const unsigned int totitems , const bool do_fixes ) {
	float *values = static_cast< float * >( data );
	bool has_errors = false;
	for ( int i = 0; i < totitems * 2; i++ ) {
		if ( !isfinite ( values [ i ] ) ) {
			if ( do_fixes ) {
				values [ i ] = 0.0f;
			}
			has_errors = true;
		}
	}
	return has_errors;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (`bool`, #CD_PROP_BOOL)
 * \{ */

static void layerInterp_propbool ( const void **sources ,
				   const float *weights ,
				   const float * ,
				   int count ,
				   void *dest ) {
	bool result = false;
	for ( int i = 0; i < count; i++ ) {
		const float interp_weight = weights [ i ];
		const bool src = *( const bool * ) sources [ i ];
		result |= src && ( interp_weight > 0.0f );
	}
	*( bool * ) dest = result;
}

/** \} */

static int layerMaxNum_tface ( ) { return MAX_MTFACE; }

static const LayerTypeInfo LAYERTYPEINFO [ CD_NUMTYPES ] = {
	// CD_MVERT
	{
		sizeof ( MVert ) ,
		"MVert" ,
		1 ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		layerEqual_mvert ,
		layerMultiply_mvert ,
		layerAdd_mvert ,
		nullptr
	},
	// CD_NORMAL
	{
		sizeof ( float [3] ) ,
		"vec3f" ,
		1 ,
		nullptr ,
		nullptr ,
		nullptr ,
		layerInterp_normal ,
		nullptr ,
		nullptr ,
		nullptr ,
		layerEqual_normal ,
		layerMultiply_normal ,
		layerAdd_normal ,
		nullptr
	},
	// CD_PROP_FLOAT
	{
		sizeof ( float ) ,
		"float" ,
		1 ,
		"Float" ,
		layerCopy_propfloat ,
		nullptr ,
		layerInterp_propfloat ,
		nullptr ,
		nullptr ,
		layerValidate_propfloat ,
		layerEqual_propfloat ,
		layerMultiply_propfloat ,
		layerAdd_propfloat ,
		nullptr
	},
	// CD_PROP_INT32
	{
		sizeof ( int ) ,
		"int" ,
		1 ,
		"Int" ,
		nullptr ,
		nullptr ,
		layerInterp_propint ,
		nullptr ,
		nullptr ,
		nullptr ,
		layerEqual_propint ,
		layerMultiply_propint ,
		layerAdd_propint ,
		nullptr
	},
	// CD_PROP_STRING
	{
		sizeof ( MStringProperty ) ,
		"MStringProperty" ,
		1 ,
		"String" ,
		layerCopy_propstring ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr
	},
	// CD_MLOOPUV
	{
		sizeof ( MLoopUV ) ,
		"MLoopUV" ,
		1 ,
		"UVMap" ,
		nullptr ,
		nullptr ,
		layerInterp_mloopuv ,
		nullptr ,
		nullptr ,
		layerValidate_mloopuv ,
		layerEqual_mloopuv ,
		layerMultiply_mloopuv ,
		layerAdd_mloopuv ,
		layerMaxNum_tface ,
	},
	// CD_PROP_BYTE_COLOR
	{
		sizeof ( MLoopCol ) ,
		"MLoopCol" ,
		1 ,
		"Col" ,
		nullptr ,
		nullptr ,
		layerInterp_mloopcol ,
		layerDefault_mloopcol ,
		nullptr ,
		nullptr ,
		layerEqual_mloopcol ,
		layerMultiply_mloopcol ,
		layerAdd_mloopcol ,
		nullptr ,
	} ,
	// CD_SHAPE_KEYINDEX
	{
		sizeof ( int ) ,
		"" ,
		0 ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
	} ,
	// CD_PROP_INT8
	{
		sizeof ( int8_t ) ,
		"MInt8Property" ,
		1 ,
		"Int8" ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
	} ,
	// CD_PROP_COLOR
	{
		sizeof ( MPropCol ) ,
		"MPropCol" ,
		1 ,
		"Color" ,
		nullptr ,
		nullptr ,
		layerInterp_propcol ,
		layerDefault_propcol ,
		nullptr ,
		nullptr ,
		layerEqual_propcol ,
		layerMultiply_propcol ,
		layerAdd_propcol ,
		nullptr ,
	} ,
	// CD_PROP_FLOAT3
	{
		sizeof ( float [ 3 ] ) ,
		"vec3f" ,
		1 ,
		"Float3" ,
		nullptr ,
		nullptr ,
		layerInterp_propfloat3 ,
		nullptr ,
		nullptr ,
		layerValidate_propfloat3 ,
		nullptr ,
		layerMultiply_propfloat3 ,
		layerAdd_propfloat3 ,
		nullptr ,
	} ,
	// CD_PROP_FLOAT2
	{
		sizeof ( float [ 2 ] ) ,
		"vec2f" ,
		1 ,
		"Float2" ,
		nullptr ,
		nullptr ,
		layerInterp_propfloat2 ,
		nullptr ,
		nullptr ,
		layerValidate_propfloat2 ,
		nullptr ,
		layerMultiply_propfloat2 ,
		layerAdd_propfloat2 ,
		nullptr ,
	} ,
	// CD_PROP_BOOL
	{
		sizeof ( float [ 2 ] ) ,
		"bool" ,
		1 ,
		"Boolean" ,
		nullptr ,
		nullptr ,
		layerInterp_propbool ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
		nullptr ,
	} ,
};

static const char *LAYERTYPENAMES [ CD_NUMTYPES ] = {
	"CDMVert" , // CD_MVERT
	"CDNormal" , // CD_NORMAL
	"CDMFloatProperty" , // CD_PROP_FLOAT
	"CDMIntProperty" , // CD_PROP_INT32
	"CDMStringProperty" , // CD_PROP_STRING
	"CDMLoopUV" , // CD_MLOOPUV
	"CDMloopCol" , // CD_PROP_BYTE_COLOR
	"CDShapeKeyIndex" , // CD_SHAPE_KEYINDEX
	"CDPropInt8" , // CD_PROP_INT8
	"CDPropCol" , // CD_PROP_COLOR
	"CDPropFloat3" , // CD_PROP_FLOAT3
	"CDPropFloat2" , // CD_PROP_FLOAT2
	"CDPropBoolean", // CD_PROP_BOOL
};

static inline const LayerTypeInfo *layerType_getInfo ( int type ) {
	if ( type < 0 || type >= CD_NUMTYPES ) {
		return nullptr;
	}
	return &LAYERTYPEINFO [ type ];
}

static inline const char *layerType_getName ( int type ) {
	if ( type < 0 || type >= CD_NUMTYPES ) {
		return nullptr;
	}
	return LAYERTYPENAMES [ type ];
}

/* -------------------------------------------------------------------- */
/** \name CustomData Math Functions
 * \{ */

bool CustomData_layer_has_math ( const struct CustomData *data , int layer_n ) {
	const LayerTypeInfo *typeInfo = layerType_getInfo ( data->Layers [ layer_n ].Type );

	if ( typeInfo->mEqual && typeInfo->mAdd && typeInfo->mMultiply ) {
		return true;
	}

	return false;
}

bool CustomData_layer_has_interp ( const struct CustomData *data , int layer_n ) {
	const LayerTypeInfo *typeInfo = layerType_getInfo ( data->Layers [ layer_n ].Type );

	if ( typeInfo->mInterp ) {
		return true;
	}

	return false;
}

bool CustomData_has_math ( const struct CustomData *data ) {
	for ( int i = 0; i < data->TotLayer; i++ ) {
		if ( CustomData_layer_has_math ( data , i ) ) {
			return true;
		}
	}

	return false;
}

bool CustomData_has_interp ( const struct CustomData *data ) {
	for ( int i = 0; i < data->TotLayer; i++ ) {
		if ( CustomData_layer_has_interp ( data , i ) ) {
			return true;
		}
	}

	return false;
}

bool CustomData_data_equals ( int type , const void *data1 , const void *data2 ) {
	const LayerTypeInfo *typeInfo = layerType_getInfo ( type );

	if ( typeInfo->mEqual ) {
		return typeInfo->mEqual ( data1 , data2 );
	}

	return !memcmp ( data1 , data2 , typeInfo->mSize );
}

void CustomData_data_multiply ( int type , void *data , float factor ) {
	const LayerTypeInfo *typeInfo = layerType_getInfo ( type );

	if ( typeInfo->mMultiply ) {
		typeInfo->mMultiply ( data , factor );
	} else {
		LIB_assert_msg ( 0 , "%s missing multiplication method for custom data type.\n" , __FUNCTION__ );
	}
}

void CustomData_data_add ( int type , void *data1 , const void *data2 ) {
	const LayerTypeInfo *typeInfo = layerType_getInfo ( type );

	if ( typeInfo->mAdd ) {
		typeInfo->mAdd ( data1 , data2 );
	} else {
		LIB_assert_msg ( 0 , "%s missing addition method for custom data type.\n" , __FUNCTION__ );
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name CustomData Functions
 * \{ */

static bool customData_resize ( CustomData *data , const int amount ) {
	CustomDataLayer *tmp = static_cast< CustomDataLayer * >(
		MEM_calloc_arrayN ( size_t ( data->MaxLayer ) + amount , sizeof ( *tmp ) , __func__ ) );
	if ( !tmp ) {
		return false;
	}

	data->MaxLayer += amount;
	if ( data->Layers ) {
		memcpy ( tmp , data->Layers , sizeof ( *tmp ) * data->TotLayer );
		MEM_freeN ( data->Layers );
	}
	data->Layers = tmp;

	return true;
}

static void customData_update_offsets ( CustomData *data ) {
	const LayerTypeInfo *typeInfo;
	int offset = 0;

	for ( int i = 0; i < data->TotLayer; i++ ) {
		typeInfo = layerType_getInfo ( data->Layers [ i ].Type );

		data->Layers [ i ].Offset = offset;
		offset += typeInfo->mSize;
	}

	data->TotSize = offset;
	CustomData_update_typemap ( data );
}

static CustomDataLayer *customData_add_layer_internal ( CustomData *data ,
							const int type ,
							const eCDAllocType alloctype ,
							void *layerdata ,
							const int totelem ,
							const char *name ) {
	const LayerTypeInfo *typeInfo = layerType_getInfo ( type );
	int flag = 0;

	if ( !typeInfo->mDefaultName && ( CustomData_get_layer_index ( data , type ) != -1 ) ) {
		return &data->Layers [ CustomData_get_layer_index ( data , type ) ];
	}

	void *newlayerdata = nullptr;
	switch ( alloctype ) {
		case CD_SET_DEFAULT: {
			if ( totelem > 0 ) {
				if ( typeInfo->mSetDefaultValue ) {
					newlayerdata = MEM_malloc_arrayN ( totelem , typeInfo->mSize , layerType_getName ( type ) );
					typeInfo->mSetDefaultValue ( newlayerdata , totelem );
				} else {
					newlayerdata = MEM_calloc_arrayN ( totelem , typeInfo->mSize , layerType_getName ( type ) );
				}
			}
		}break;
		case CD_CONSTRUCT: {
			if ( totelem > 0 ) {
				newlayerdata = MEM_malloc_arrayN ( totelem , typeInfo->mSize , layerType_getName ( type ) );
				if ( typeInfo->mConstruct ) {
					typeInfo->mConstruct ( newlayerdata , totelem );
				}
			}
		}break;
		case CD_ASSIGN: {
			if ( totelem > 0 ) {
				LIB_assert ( layerdata != nullptr );
				newlayerdata = layerdata;
			} else {
				MEM_SAFE_FREE ( layerdata );
			}
		}break;
		case CD_REFERENCE: {
			if ( totelem > 0 ) {
				LIB_assert ( layerdata != nullptr );
				newlayerdata = layerdata;
				flag |= CD_FLAG_NOFREE;
			}
		}break;
		case CD_DUPLICATE: {
			if ( totelem > 0 ) {
				newlayerdata = MEM_malloc_arrayN ( totelem , typeInfo->mSize , layerType_getName ( type ) );
				if ( typeInfo->mCopy ) {
					typeInfo->mCopy ( layerdata , newlayerdata , totelem );
				} else {
					LIB_assert ( layerdata != nullptr );
					LIB_assert ( newlayerdata != nullptr );
					memcpy ( newlayerdata , layerdata , size_t ( totelem ) * typeInfo->mSize );
				}
			}
		}break;
	}

	int index = data->TotLayer;
	if ( index >= data->MaxLayer ) {
		if ( !customData_resize ( data , CUSTOMDATA_GROW ) ) {
			if ( newlayerdata != layerdata ) {
				MEM_freeN ( newlayerdata );
			}
			return nullptr;
		}
	}

	data->TotLayer++;

	/* keep layers ordered by type */
	for ( ; index > 0 && data->Layers [ index - 1 ].Type > type; index-- ) {
		data->Layers [ index ] = data->Layers [ index - 1 ];
	}

	CustomDataLayer &new_layer = data->Layers [ index ];

	/* Clear remaining data on the layer. The original data on the layer has been moved to another
	 * index. Without this, it can happen that information from the previous layer at that index
	 * leaks into the new layer. */
	memset ( &new_layer , 0 , sizeof ( CustomDataLayer ) );

	new_layer.Type = type;
	new_layer.Flag = flag;
	new_layer.Data = newlayerdata;

	/* Set default name if none exists. Note we only call DATA_()  once
	 * we know there is a default name, to avoid overhead of locale lookups
	 * in the depsgraph. */
	if ( !name && typeInfo->mDefaultName ) {
		name = typeInfo->mDefaultName;
	}

	if ( name ) {
		LIB_strncpy ( new_layer.Name , name , sizeof ( new_layer.Name ) );
		// TODO : Make name unique
	} else {
		new_layer.Name [ 0 ] = '\0';
	}

	if ( index > 0 && data->Layers [ index - 1 ].Type == type ) {
		new_layer.Active = data->Layers [ index - 1 ].Active;
	} else {
		new_layer.Active = 0;
	}

	customData_update_offsets ( data );

	return &data->Layers [ index ];
}

static void customData_free_layer_internal ( CustomDataLayer *layer , const int totelem ) {
	const LayerTypeInfo *typeInfo;

	if ( layer->AnonymousId != nullptr ) {
		RKE_anonymous_attribute_id_decrement_weak ( layer->AnonymousId );
		layer->AnonymousId = nullptr;
	}
	if ( !( layer->Flag & CD_FLAG_NOFREE ) && layer->Data ) {
		typeInfo = layerType_getInfo ( layer->Type );

		if ( typeInfo->mFree ) {
			typeInfo->mFree ( layer->Data , totelem , typeInfo->mSize );
		}

		if ( layer->Data ) {
			MEM_freeN ( layer->Data );
		}
	}
}

void CustomData_update_typemap ( CustomData *data ) {
	int lasttype = -1;

	for ( int i = 0; i < CD_NUMTYPES; i++ ) {
		data->TypeMap [ i ] = -1;
	}

	for ( int i = 0; i < data->TotLayer; i++ ) {
		const int type = data->Layers [ i ].Type;
		if ( type != lasttype ) {
			data->TypeMap [ type ] = i;
			lasttype = type;
		}
	}
}

static bool customdata_typemap_is_valid ( const CustomData *data ) {
	CustomData data_copy = *data;
	CustomData_update_typemap ( &data_copy );
	return ( memcmp ( data->TypeMap , data_copy.TypeMap , sizeof ( data->TypeMap ) ) == 0 );
}

void CustomData_realloc ( CustomData *data , const int totelem ) {
	LIB_assert ( totelem >= 0 );
	for ( int i = 0; i < data->TotLayer; i++ ) {
		CustomDataLayer *layer = &data->Layers [ i ];
		const LayerTypeInfo *typeInfo;
		if ( layer->Flag & CD_FLAG_NOFREE ) {
			continue;
		}
		typeInfo = layerType_getInfo ( layer->Type );
		/* Use calloc to avoid the need to manually initialize new data in layers.
		* Useful for types which contain a pointer. */
		layer->Data = MEM_recallocN ( layer->Data , ( size_t ) totelem * typeInfo->mSize );
	}
}

void CustomData_reset ( CustomData *data ) {
	memset ( data , 0 , sizeof ( *data ) );
	copy_vn_i ( data->TypeMap , CD_NUMTYPES , -1 );
}

void CustomData_free ( CustomData *data , const int totelem ) {
	for ( int i = 0; i < data->TotLayer; i++ ) {
		customData_free_layer_internal ( &data->Layers [ i ] , totelem );
	}

	if ( data->Layers ) {
		MEM_freeN ( data->Layers );
	}

	CustomData_reset ( data );
}

void CustomData_free_temporary ( CustomData *data , const int totelem ) {
	int i , j;
	bool changed = false;
	for ( i = 0 , j = 0; i < data->TotLayer; i++ ) {
		CustomDataLayer *layer = &data->Layers [ i ];

		if ( i != j ) {
			data->Layers [ j ] = data->Layers [ i ];
		}

		if ( ( layer->Flag & CD_FLAG_TEMPORARY ) == CD_FLAG_TEMPORARY ) {
			customData_free_layer_internal ( layer , totelem );
			changed = true;
		} else {
			j++;
		}
	}

	data->TotLayer = j;

	if ( data->TotLayer <= data->MaxLayer - CUSTOMDATA_GROW ) {
		customData_resize ( data , -CUSTOMDATA_GROW );
		changed = true;
	}

	if ( changed ) {
		customData_update_offsets ( data );
	}
}

/* -------------------------------------------------------------------- */
/** \name CustomData Index Values To Access The Layers
* \{ */

int CustomData_get_layer_index ( const CustomData *data , const int type ) {
	LIB_assert ( customdata_typemap_is_valid ( data ) );
	return data->TypeMap [ type ];
}

int CustomData_get_layer_index_n ( const CustomData *data , const int type , const int n ) {
	LIB_assert ( n >= 0 );
	int i = CustomData_get_layer_index ( data , type );

	if ( i != -1 ) {
		LIB_assert ( i + n < data->TotLayer );
		i = ( data->Layers [ i + n ].Type == type ) ? ( i + n ) : ( -1 );
	}

	return i;
}

int CustomData_get_named_layer_index ( const CustomData *data , const int type , const char *name ) {
	for ( int i = 0; i < data->TotLayer; i++ ) {
		if ( data->Layers [ i ].Type == type ) {
			if ( strcmp ( data->Layers [ i ].Name , name ) == 0 ) {
				return i;
			}
		}
	}

	return -1;
}

int CustomData_get_active_layer_index ( const CustomData *data , const int type ) {
	const int layer_index = data->TypeMap [ type ];
	LIB_assert ( customdata_typemap_is_valid ( data ) );
	return ( layer_index != -1 ) ? layer_index + data->Layers [ layer_index ].Active : -1;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name CustomData Index Values Per Layer Type
* \{ */

int CustomData_get_named_layer ( const CustomData *data , const int type , const char *name ) {
	const int named_index = CustomData_get_named_layer_index ( data , type , name );
	const int layer_index = data->TypeMap [ type ];
	LIB_assert ( customdata_typemap_is_valid ( data ) );
	return ( named_index != -1 ) ? named_index - layer_index : -1;
}

int CustomData_get_active_layer ( const CustomData *data , const int type ) {
	const int layer_index = data->TypeMap [ type ];
	LIB_assert ( customdata_typemap_is_valid ( data ) );
	return ( layer_index != -1 ) ? data->Layers [ layer_index ].Active : -1;
}

/** \} */

const char *CustomData_get_active_layer_name ( const CustomData *data , const int type ) {
	/* Get the layer index of the active layer of this type. */
	const int layer_index = CustomData_get_active_layer_index ( data , type );
	return layer_index < 0 ? nullptr : data->Layers [ layer_index ].Name;
}

bool CustomData_set_layer_name ( CustomData *data , const int type , const int n , const char *name ) {
	/* get the layer index of the first layer of type */
	const int layer_index = CustomData_get_layer_index_n ( data , type , n );

	if ( ( layer_index == -1 ) || !name ) {
		return false;
	}

	LIB_strncpy ( data->Layers [ layer_index ].Name , name , sizeof ( data->Layers [ layer_index ].Name ) );

	return true;
}

void CustomData_set_layer_active ( CustomData *data , const int type , const int n ) {
	for ( int i = 0; i < data->TotLayer; i++ ) {
		if ( data->Layers [ i ].Type == type ) {
			data->Layers [ i ].Active = n;
		}
	}
}

void CustomData_set_layer_active_index ( CustomData *data , const int type , const int n ) {
	const int layer_index = data->TypeMap [ type ];
	LIB_assert ( customdata_typemap_is_valid ( data ) );

	for ( int i = 0; i < data->TotLayer; i++ ) {
		if ( data->Layers [ i ].Type == type ) {
			data->Layers [ i ].Active = n - layer_index;
		}
	}
}

void *CustomData_get ( const CustomData *data , const int index , const int type ) {
	LIB_assert ( index >= 0 );

	/* get the layer index of the active layer of type */
	int layer_index = CustomData_get_active_layer_index ( data , type );
	if ( layer_index == -1 ) {
		return nullptr;
	}

	/* get the offset of the desired element */
	const size_t offset = ( size_t ) index * layerType_getInfo ( type )->mSize;

	return POINTER_OFFSET ( data->Layers [ layer_index ].Data , offset );
}

void *CustomData_get_n ( const struct CustomData *data , int type , int index , int n ) {
	LIB_assert ( index >= 0 && n >= 0 );

	/* get the layer index of the first layer of type */
	int layer_index = data->TypeMap [ type ];
	if ( layer_index == -1 ) {
		return nullptr;
	}

	const size_t offset = ( size_t ) index * layerType_getInfo ( type )->mSize;
	return POINTER_OFFSET ( data->Layers [ layer_index + n ].Data , offset );
}

void *CustomData_bmesh_get ( const CustomData *data , void *block , const int type ) {
	/* get the layer index of the first layer of type */
	int layer_index = CustomData_get_active_layer_index ( data , type );
	if ( layer_index == -1 ) {
		return nullptr;
	}

	return POINTER_OFFSET ( block , data->Layers [ layer_index ].Offset );
}

void *CustomData_bmesh_get_n ( const CustomData *data , void *block , const int type , const int n ) {
	/* get the layer index of the first layer of type */
	int layer_index = CustomData_get_layer_index ( data , type );
	if ( layer_index == -1 ) {
		return nullptr;
	}

	return POINTER_OFFSET ( block , data->Layers [ layer_index + n ].Offset );
}

void *CustomData_set_layer ( const CustomData *data , const int type , void *ptr ) {
	/* get the layer index of the first layer of type */
	int layer_index = CustomData_get_active_layer_index ( data , type );

	if ( layer_index == -1 ) {
		return nullptr;
	}

	data->Layers [ layer_index ].Data = ptr;

	return ptr;
}

void *CustomData_set_layer_n ( const CustomData *data , const int type , const int n , void *ptr ) {
	/* get the layer index of the first layer of type */
	int layer_index = CustomData_get_layer_index_n ( data , type , n );
	if ( layer_index == -1 ) {
		return nullptr;
	}

	data->Layers [ layer_index ].Data = ptr;

	return ptr;
}

void CustomData_set ( const struct CustomData *data , int index , int type , const void *source ) {
	void *dest = CustomData_get ( data , index , type );
	const LayerTypeInfo *typeInfo = layerType_getInfo ( type );

	if ( !dest ) {
		return;
	}

	if ( typeInfo->mCopy ) {
		typeInfo->mCopy ( dest , source , 1 );
	} else {
		memcpy ( dest , source , typeInfo->mSize );
	}
}

void *CustomData_add_layer ( CustomData *data , const int type , eCDAllocType alloctype , void *layerdata , const int totelem ) {
	const LayerTypeInfo *typeInfo = layerType_getInfo ( type );

	CustomDataLayer *layer = customData_add_layer_internal (
		data , type , alloctype , layerdata , totelem , typeInfo->mDefaultName );
	CustomData_update_typemap ( data );

	if ( layer ) {
		return layer->Data;
	}

	return nullptr;
}

void *CustomData_add_layer_named ( CustomData *data ,
				   const int type ,
				   const eCDAllocType alloctype ,
				   void *layerdata ,
				   const int totelem ,
				   const char *name ) {
	CustomDataLayer *layer = customData_add_layer_internal (
		data , type , alloctype , layerdata , totelem , name );
	CustomData_update_typemap ( data );

	if ( layer ) {
		return layer->Data;
	}

	return nullptr;
}

void *CustomData_add_layer_anonymous ( CustomData *data ,
				       const int type ,
				       const eCDAllocType alloctype ,
				       void *layerdata ,
				       const int totelem ,
				       const AnonymousAttributeID *anonymous_id ) {
	const char *name = RKE_anonymous_attribute_id_internal_name ( anonymous_id );
	CustomDataLayer *layer = customData_add_layer_internal (
		data , type , alloctype , layerdata , totelem , name );
	CustomData_update_typemap ( data );

	if ( layer == nullptr ) {
		return nullptr;
	}

	RKE_anonymous_attribute_id_increment_weak ( anonymous_id );
	layer->AnonymousId = anonymous_id;
	return layer->Data;
}

void CustomData_copy_elements ( const int type ,
				void *src_data_ofs ,
				void *dst_data_ofs ,
				const int count ) {
	const LayerTypeInfo *typeInfo = layerType_getInfo ( type );

	if ( typeInfo->mCopy ) {
		typeInfo->mCopy ( dst_data_ofs , src_data_ofs , count );
	} else {
		memcpy ( dst_data_ofs , src_data_ofs , ( size_t ) count * typeInfo->mSize );
	}
}

void CustomData_copy_data_layer ( const CustomData *source ,
				  CustomData *dest ,
				  const int src_layer_index ,
				  const int dst_layer_index ,
				  const int src_index ,
				  const int dst_index ,
				  const int count ) {
	const LayerTypeInfo *typeInfo;

	const void *src_data = source->Layers [ src_layer_index ].Data;
	void *dst_data = dest->Layers [ dst_layer_index ].Data;

	typeInfo = layerType_getInfo ( source->Layers [ src_layer_index ].Type );

	const size_t src_offset = ( size_t ) src_index * typeInfo->mSize;
	const size_t dst_offset = ( size_t ) dst_index * typeInfo->mSize;

	if ( !count || !src_data || !dst_data ) {
		if ( count && !( src_data == nullptr && dst_data == nullptr ) ) {
			fprintf ( stderr ,
				  "null data for %s type (%p --> %p), skipping" ,
				  layerType_getName ( source->Layers [ src_layer_index ].Type ) ,
				  ( void * ) src_data ,
				  ( void * ) dst_data );
		}
		return;
	}

	if ( typeInfo->mCopy ) {
		typeInfo->mCopy ( POINTER_OFFSET ( dst_data , dst_offset ) ,
				  POINTER_OFFSET ( src_data , src_offset ) , count );
	} else {
		memcpy ( POINTER_OFFSET ( dst_data , dst_offset ) ,
			 POINTER_OFFSET ( src_data , src_offset ) ,
			 ( size_t ) count * typeInfo->mSize );
	}
}

void CustomData_copy_data_named ( const CustomData *source ,
				  CustomData *dest ,
				  const int source_index ,
				  const int dest_index ,
				  const int count ) {
	// copies a layer at a time
	for ( int src_i = 0; src_i < source->TotLayer; src_i++ ) {

		int dest_i = CustomData_get_named_layer_index (
			dest , source->Layers [ src_i ].Type , source->Layers [ src_i ].Name );

		// if we found a matching layer, copy the data
		if ( dest_i != -1 ) {
			CustomData_copy_data_layer ( source , dest , src_i , dest_i , source_index , dest_index , count );
		}
	}
}

void CustomData_bmesh_free_block ( CustomData *data , void **block ) {
	if ( *block == nullptr ) {
		return;
	}

	for ( int i = 0; i < data->TotLayer; i++ ) {
		if ( !( data->Layers [ i ].Flag & CD_FLAG_NOFREE ) ) {
			const LayerTypeInfo *typeInfo = layerType_getInfo ( data->Layers [ i ].Type );

			if ( typeInfo->mFree ) {
				int offset = data->Layers [ i ].Offset;
				typeInfo->mFree ( POINTER_OFFSET ( *block , offset ) , 1 , typeInfo->mSize );
			}
		}
	}

	if ( data->TotSize ) {
		LIB_mempool_free ( data->Pool , *block );
	}

	*block = nullptr;
}

static void CustomData_bmesh_alloc_block ( CustomData *data , void **block ) {
	if ( *block ) {
		CustomData_bmesh_free_block ( data , block );
	}

	if ( data->TotSize > 0 ) {
		*block = LIB_mempool_alloc ( data->Pool );
	} else {
		*block = nullptr;
	}
}

static void CustomData_bmesh_set_default_n ( CustomData *data , void **block , const int n ) {
	int offset = data->Layers [ n ].Offset;
	const LayerTypeInfo *typeInfo = layerType_getInfo ( data->Layers [ n ].Type );

	if ( typeInfo->mSetDefaultValue ) {
		typeInfo->mSetDefaultValue ( POINTER_OFFSET ( *block , offset ) , 1 );
	} else {
		memset ( POINTER_OFFSET ( *block , offset ) , 0 , typeInfo->mSize );
	}
}

void CustomData_bmesh_set_default ( CustomData *data , void **block ) {
	if ( *block == nullptr ) {
		CustomData_bmesh_alloc_block ( data , block );
	}

	for ( int i = 0; i < data->TotLayer; i++ ) {
		CustomData_bmesh_set_default_n ( data , block , i );
	}
}

void CustomData_bmesh_copy_data_exclude_by_type ( const CustomData *source ,
						  CustomData *dest ,
						  void *src_block ,
						  void **dest_block ,
						  const eCustomDataMask mask_exclude ) {
	/* Note that having a version of this function without a 'mask_exclude'
	* would cause too much duplicate code, so add a check instead. */
	const bool no_mask = ( mask_exclude == 0 );

	if ( *dest_block == nullptr ) {
		CustomData_bmesh_alloc_block ( dest , dest_block );
		if ( *dest_block ) {
			memset ( *dest_block , 0 , dest->TotSize );
		}
	}

	// copies a layer at a time
	int dest_i = 0;
	for ( int src_i = 0; src_i < source->TotLayer; src_i++ ) {

		/* find the first dest layer with type >= the source type
		* (this should work because layers are ordered by type)
		*/
		while ( dest_i < dest->TotLayer && dest->Layers [ dest_i ].Type < source->Layers [ src_i ].Type ) {
			CustomData_bmesh_set_default_n ( dest , dest_block , dest_i );
			dest_i++;
		}

		// if there are no more dest layers, we're done
		if ( dest_i >= dest->TotLayer ) {
			return;
		}

		// if we found a matching layer, copy the data
		if ( dest->Layers [ dest_i ].Type == source->Layers [ src_i ].Type &&
		     strcmp ( dest->Layers [ dest_i ].Name , source->Layers [ src_i ].Name ) == 0 ) {
			if ( no_mask || ( ( CD_TYPE_AS_MASK ( dest->Layers [ dest_i ].Type ) & mask_exclude ) == 0 ) ) {
				const void *src_data = POINTER_OFFSET ( src_block , source->Layers [ src_i ].Offset );
				void *dest_data = POINTER_OFFSET ( *dest_block , dest->Layers [ dest_i ].Offset );
				const LayerTypeInfo *typeInfo = layerType_getInfo ( source->Layers [ src_i ].Type );
				if ( typeInfo->mCopy ) {
					typeInfo->mCopy ( dest_data , src_data , 1 );
				} else {
					memcpy ( dest_data , src_data , typeInfo->mSize );
				}
			}

			/* if there are multiple source & dest layers of the same type,
			 * we don't want to copy all source layers to the same dest, so
			 * increment dest_i
			 */
			dest_i++;
		}
	}

	while ( dest_i < dest->TotLayer ) {
		CustomData_bmesh_set_default_n ( dest , dest_block , dest_i );
		dest_i++;
	}
}

void CustomData_bmesh_copy_data ( const CustomData *source ,
				  CustomData *dest ,
				  void *src_block ,
				  void **dest_block ) {
	CustomData_bmesh_copy_data_exclude_by_type ( source , dest , src_block , dest_block , 0 );
}

void CustomData_bmesh_free_block_data_exclude_by_type ( CustomData *data ,
							void *block ,
							const eCustomDataMask mask_exclude ) {
	if ( block == nullptr ) {
		return;
	}
	for ( int i = 0; i < data->TotLayer; i++ ) {
		if ( ( CD_TYPE_AS_MASK ( data->Layers [ i ].Type ) & mask_exclude ) == 0 ) {
			const LayerTypeInfo *typeInfo = layerType_getInfo ( data->Layers [ i ].Type );
			const size_t offset = data->Layers [ i ].Offset;
			if ( !( data->Layers [ i ].Flag & CD_FLAG_NOFREE ) ) {
				if ( typeInfo->mFree ) {
					typeInfo->mFree ( POINTER_OFFSET ( block , offset ) , 1 , typeInfo->mSize );
				}
			}
			memset ( POINTER_OFFSET ( block , offset ) , 0 , typeInfo->mSize );
		}
	}
}

void CustomData_set_layer_flag ( struct CustomData *data , int type , int flag ) {
	for ( int i = 0; i < data->TotLayer; i++ ) {
		if ( data->Layers [ i ].Type == type ) {
			data->Layers [ i ].Flag |= flag;
		}
	}
}

void CustomData_clear_layer_flag ( struct CustomData *data , int type , int flag ) {
	const int nflag = ~flag;

	for ( int i = 0; i < data->TotLayer; i++ ) {
		if ( data->Layers [ i ].Type == type ) {
			data->Layers [ i ].Flag &= nflag;
		}
	}
}

void CustomData_bmesh_free_block_data ( CustomData *data , void *block ) {
	if ( block == nullptr ) {
		return;
	}
	for ( int i = 0; i < data->TotLayer; i++ ) {
		if ( !( data->Layers [ i ].Flag & CD_FLAG_NOFREE ) ) {
			const LayerTypeInfo *typeInfo = layerType_getInfo ( data->Layers [ i ].Type );
			if ( typeInfo->mFree ) {
				const size_t offset = data->Layers [ i ].Offset;
				typeInfo->mFree ( POINTER_OFFSET ( block , offset ) , 1 , typeInfo->mSize );
			}
		}
	}
	if ( data->TotSize ) {
		memset ( block , 0 , data->TotSize );
	}
}

bool CustomData_bmesh_has_free ( const CustomData *data ) {
	for ( int i = 0; i < data->TotLayer; i++ ) {
		if ( !( data->Layers [ i ].Flag & CD_FLAG_NOFREE ) ) {
			const LayerTypeInfo *typeInfo = layerType_getInfo ( data->Layers [ i ].Type );
			if ( typeInfo->mFree ) {
				return true;
			}
		}
	}
	return false;
}

/** \} */
