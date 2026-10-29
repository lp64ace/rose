#ifndef ED_TRANSFORM_H
#define ED_TRANSFORM_H

struct wmKeyConfig;

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

void ED_keymap_transform(struct wmKeyConfig *keyconf);
void transform_operatortypes(void);

#ifdef __cplusplus
}
#endif

#endif	// !ED_TRANSFORM_H
