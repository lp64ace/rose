#pragma once

#include "lib_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

	ROSE_INLINE void blend_color_mix_byte ( unsigned char dst [ 4 ] ,
						const unsigned char src1 [ 4 ] ,
						const unsigned char src2 [ 4 ] );
	ROSE_INLINE void blend_color_add_byte ( unsigned char dst [ 4 ] ,
						const unsigned char src1 [ 4 ] ,
						const unsigned char src2 [ 4 ] );
	ROSE_INLINE void blend_color_sub_byte ( unsigned char dst [ 4 ] ,
						const unsigned char src1 [ 4 ] ,
						const unsigned char src2 [ 4 ] );
	ROSE_INLINE void blend_color_mul_byte ( unsigned char dst [ 4 ] ,
						const unsigned char src1 [ 4 ] ,
						const unsigned char src2 [ 4 ] );
	ROSE_INLINE void blend_color_lighten_byte ( unsigned char dst [ 4 ] ,
						    const unsigned char src1 [ 4 ] ,
						    const unsigned char src2 [ 4 ] );
	ROSE_INLINE void blend_color_darken_byte ( unsigned char dst [ 4 ] ,
						   const unsigned char src1 [ 4 ] ,
						   const unsigned char src2 [ 4 ] );
	ROSE_INLINE void blend_color_erase_alpha_byte ( unsigned char dst [ 4 ] ,
							const unsigned char src1 [ 4 ] ,
							const unsigned char src2 [ 4 ] );
	ROSE_INLINE void blend_color_add_alpha_byte ( unsigned char dst [ 4 ] ,
						      const unsigned char src1 [ 4 ] ,
						      const unsigned char src2 [ 4 ] );

	ROSE_INLINE void blend_color_interpolate_byte ( unsigned char dst [ 4 ] ,
							const unsigned char src1 [ 4 ] ,
							const unsigned char src2 [ 4 ] ,
							float t );

	ROSE_INLINE void blend_color_mix_float ( float dst [ 4 ] , const float src1 [ 4 ] , const float src2 [ 4 ] );
	ROSE_INLINE void blend_color_add_float ( float dst [ 4 ] , const float src1 [ 4 ] , const float src2 [ 4 ] );
	ROSE_INLINE void blend_color_sub_float ( float dst [ 4 ] , const float src1 [ 4 ] , const float src2 [ 4 ] );
	ROSE_INLINE void blend_color_mul_float ( float dst [ 4 ] , const float src1 [ 4 ] , const float src2 [ 4 ] );
	ROSE_INLINE void blend_color_lighten_float ( float dst [ 4 ] , const float src1 [ 4 ] , const float src2 [ 4 ] );
	ROSE_INLINE void blend_color_darken_float ( float dst [ 4 ] , const float src1 [ 4 ] , const float src2 [ 4 ] );
	ROSE_INLINE void blend_color_erase_alpha_float ( float dst [ 4 ] , const float src1 [ 4 ] , const float src2 [ 4 ] );
	ROSE_INLINE void blend_color_add_alpha_float ( float dst [ 4 ] , const float src1 [ 4 ] , const float src2 [ 4 ] );

	ROSE_INLINE void blend_color_interpolate_float ( float dst [ 4 ] ,
							 const float src1 [ 4 ] ,
							 const float src2 [ 4 ] ,
							 float t );

	/* -------------------------------------------------------------------- */
	/** \name Inline Definitions
	 * \{ */

#ifdef ROSE_USE_INLINE
	#include "intern/lib_math_color_blend_inline.cc"
#endif

	 /** \} */

#ifdef __cplusplus
}
#endif
