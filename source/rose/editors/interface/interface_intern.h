#ifndef INTERFACE_INTERN_H
#define INTERFACE_INTERN_H

#include "DNA_vector_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct GPUBatch;

/**
 * Widget shader parameters, must match the shader layout.
 */
typedef struct uiWidgetBaseParameters {
	rctf recti, rect;
	float radi, rad;
	float facxi, facyi;
	float round_corners[4];
	float color_inner1[4], color_inner2[4];
	float color_outline[4], color_emboss[4];
	float color_tria[4];
	float tria1_center[2], tria2_center[2];
	float tria1_size, tria2_size;
	float shade_dir;
	/* We pack alpha check and discard factor in alpha_discard.
	* If the value is negative then we do alpha check.
	* The absolute value itself is the discard factor.
	* Initialize value to 1.0f if you don't want discard. */
	float alpha_discard;
	float tria_type;
	float _pad[3];
} uiWidgetBaseParameters;

enum {
	ROUNDBOX_TRIA_NONE = 0,
	ROUNDBOX_TRIA_ARROWS,
	ROUNDBOX_TRIA_SCROLL,
	ROUNDBOX_TRIA_MENU,
	ROUNDBOX_TRIA_CHECK,
	ROUNDBOX_TRIA_HOLD_ACTION_ARROW,
	ROUNDBOX_TRIA_DASH,

	ROUNDBOX_TRIA_MAX, /* don't use */
};

struct GPUBatch *ui_batch_roundbox_widget_get();
struct GPUBatch *ui_batch_roundbox_shadow_get();

#ifdef __cplusplus
}
#endif

#endif // INTERFACE_INTERN_H
