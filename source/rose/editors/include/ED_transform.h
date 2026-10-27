#ifndef ED_TRANSFORM_H
#define ED_TRANSFORM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum eTransformMode {
	TFM_INIT = -1,
	TFM_NONE = 0,
	TFM_TRANSLATION,
	TFM_ROTATION,
	TFM_RESIZE,
} eTransformMode;

#ifdef __cplusplus
}
#endif

#endif	// !ED_TRANSFORM_H
