#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

/* Draw 2D rectangles (replaces glRect functions) */
/* caller is responsible for vertex format & shader */
void immRectf(unsigned int pos, float x1, float y1, float x2, float y2);
void immRecti(unsigned int pos, int x1, int y1, int x2, int y2);

#if defined(__cplusplus)
}
#endif
