#ifndef DNA_ID_ENUMS_H
#define DNA_ID_ENUMS_H

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name ID_Type
 * \{ */

#ifdef __BIG_ENDIAN__
/* Big Endian */
#	define MAKE_ID2(a, b) ((a) << 8 | (b))
#else
/* Little Endian */
#	define MAKE_ID2(a, b) ((b) << 8 | (a))
#endif

/**
 * ID from database.
 * The first 2 bytes of #ID.name (for runtime checks, see #GS macro).
 */
typedef enum ID_Type {
	ID_LI = MAKE_ID2('L', 'I'),		/* Library */
	ID_ME = MAKE_ID2('M', 'E'),		/* Mesh */
	ID_OB = MAKE_ID2('O', 'B'),		/* Object */
	ID_GR = MAKE_ID2('G', 'R'),		/* Collection */
	ID_SCE = MAKE_ID2('S', 'C'),	/* Scene */
	ID_SCR = MAKE_ID2('S', 'R'),	/* Screen */
	ID_WM = MAKE_ID2('W', 'M'),		/* WindowManager */
} ID_Type;

/* Only used as 'placeholder' in .rose files for directly linked data-blocks. */
#define ID_LINK_PLACEHOLDER MAKE_ID2('I', 'D')

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// DNA_ID_ENUMS_H
