#ifndef ED_OBJECT_H
#define ED_OBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

struct Base;
struct Object;

typedef enum eObjectSelect_Mode {
	BA_DESELECT = 0,
	BA_SELECT = 1,
	BA_INVERT = 2,
} eObjectSelect_Mode;

/**
 * Simple API for object selection, rather than just using the flag
 * this takes into account the 'restrict selection in 3d view' flag.
 * deselect works always, the restriction just prevents selection
 *
 * \note Caller must send a `NC_SCENE | ND_OB_SELECT` notifier
 * (or a `NC_SCENE | ND_OB_VISIBLE` in case of visibility toggling).
 */
void ED_object_base_select(struct Base *base, eObjectSelect_Mode mode);

#ifdef __cplusplus
}
#endif

#endif	// !ED_OBJECT_H
