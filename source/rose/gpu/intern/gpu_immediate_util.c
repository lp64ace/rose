#include <stdio.h>
#include <string.h>

#include "GPU_immediate.h"
#include "GPU_immediate_util.h"

#include "LIB_assert.h"
#include "LIB_math_base.h"
#include "LIB_math_vector.h"
#include "LIB_math_matrix.h"
#include "LIB_utildefines.h"

void immRectf(unsigned int pos, float x1, float y1, float x2, float y2) {
	immBegin(GPU_PRIM_TRI_FAN, 4);
	immVertex2f(pos, x1, y1);
	immVertex2f(pos, x2, y1);
	immVertex2f(pos, x2, y2);
	immVertex2f(pos, x1, y2);
	immEnd();
}

void immRecti(unsigned int pos, int x1, int y1, int x2, int y2) {
	immBegin(GPU_PRIM_TRI_FAN, 4);
	immVertex2i(pos, x1, y1);
	immVertex2i(pos, x2, y1);
	immVertex2i(pos, x2, y2);
	immVertex2i(pos, x1, y2);
	immEnd();
}

