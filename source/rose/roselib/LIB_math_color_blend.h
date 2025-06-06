#ifndef LIB_MATH_COLOR_BLEND_H
#define LIB_MATH_COLOR_BLEND_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

ROSE_INLINE void blend_color_mix_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_add_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_sub_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_mul_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_lighten_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_darken_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_erase_alpha_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_add_alpha_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);

ROSE_INLINE void blend_color_overlay_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_hardlight_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_burn_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_linearburn_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_dodge_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_screen_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_softlight_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_pinlight_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_linearlight_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_vividlight_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_difference_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_exclusion_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_color_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_hue_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_saturation_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);
ROSE_INLINE void blend_color_luminosity_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4]);

ROSE_INLINE void blend_color_interpolate_byte(unsigned char dst[4], const unsigned char src1[4], const unsigned char src2[4], float t);

ROSE_INLINE void blend_color_mix_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_add_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_sub_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_mul_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_lighten_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_darken_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_erase_alpha_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_add_alpha_float(float dst[4], const float src1[4], const float src2[4]);

ROSE_INLINE void blend_color_overlay_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_hardlight_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_burn_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_linearburn_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_dodge_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_screen_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_softlight_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_pinlight_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_linearlight_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_vividlight_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_difference_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_exclusion_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_color_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_hue_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_saturation_float(float dst[4], const float src1[4], const float src2[4]);
ROSE_INLINE void blend_color_luminosity_float(float dst[4], const float src1[4], const float src2[4]);

ROSE_INLINE void blend_color_interpolate_float(float dst[4], const float src1[4], const float src2[4], float t);

#ifdef __cplusplus
}
#endif

#include "intern/math_color_blend_inline.c"

#endif	// LIB_MATH_COLOR_BLEND_H
