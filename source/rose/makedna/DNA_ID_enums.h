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
	ID_LI = MAKE_ID2('I', 'D'),
	ID_SCR = MAKE_ID2('S', 'R'),
	ID_WM = MAKE_ID2('W', 'M'),
} ID_Type;

/* Only used as 'placeholder' in .rose files for directly linked data-blocks. */
#define ID_LINK_PLACEHOLDER MAKE_ID2('I', 'D')

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// DNA_ID_ENUMS_H
