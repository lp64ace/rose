#pragma once

#include "LIB_rect.h"

#ifdef __cplusplus
extern "C" {
#endif

void wmViewport(const struct rcti *winrct);
void wmPartialViewport(struct rcti *drawrct, const struct rcti *winrct, const struct rcti *partialrct);
void wmWindowViewport(struct wmWindow *win);

void wmOrtho2(float x1, float x2, float y1, float y2);
void wmOrtho2_region_pixelspace(const struct ARegion *region);
void wmOrtho2_pixelspace(float x, float y);

#ifdef __cplusplus
}
#endif
