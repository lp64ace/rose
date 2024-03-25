#pragma once

#include "DNA_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(BIG_ENDIAN)
#	define MAKE_ID2(c, d) ((c) << 8 | (d))
#elif defined(LITTLE_ENDIAN)
#	define MAKE_ID2(c, d) ((d) << 8 | (c))
#endif

typedef enum {
	ID_LI = MAKE_ID2('L', 'I'),
	ID_SCR = MAKE_ID2('S', 'R'),
	ID_WS = MAKE_ID2('W', 'S'),
	ID_WM = MAKE_ID2('W', 'M'),
} ID_Type;

/** Only used as 'placeholder' in .blend files for directly linked data-blocks. */
#define ID_LINK_PLACEHOLDER MAKE_ID2('I', 'D')

#ifdef __cplusplus
}
#endif
