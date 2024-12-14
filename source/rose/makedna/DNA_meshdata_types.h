#ifndef DNA_MESHDATA_TYPES_H
#define DNA_MESHDATA_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Loop Tessellation Runtime Data
 * \{ */

typedef struct MLoopTri {
	unsigned int tri[3];
} MLoopTri;

typedef struct MVertTri {
	unsigned int tri[3];
} MVertTri;

/* \} */

/* -------------------------------------------------------------------- */
/** \name Custom Data (Generic)
 * \{ */

/** Custom Data Properties */
typedef struct MFloatProperty {
	float f;
} MFloatProperty;
typedef struct MIntProperty {
	int i;
} MIntProperty;
typedef struct MStringProperty {
	char s[255], s_len;
} MStringProperty;
typedef struct MBoolProperty {
	uint8_t b;
} MBoolProperty;
typedef struct MInt8Property {
	int8_t i;
} MInt8Property;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Utility Macros
 * \{ */

/** Number of tri's that make up this polygon once tessellated. */
#define ME_POLY_TRI_TOT(size) (size - 2)

/**
 * Check out-of-bounds material, note that this is nearly always prevented,
 * yet its still possible in rare cases.
 * So usage such as array lookup needs to check.
 */
#define ME_MAT_NR_TEST(mat_nr, totmat) ((mat_nr < totmat) ? mat_nr : 0)

/** \} */

/* -------------------------------------------------------------------- */
/** \name Custom Data (Loop)
 * \{ */

/**
 * \note While alpha is not currently in the 3D Viewport,
 * this may eventually be added back, keep this value set to 255.
 */
typedef struct MLoopCol {
	unsigned char r, g, b, a;
} MLoopCol;

typedef struct MPropCol {
	float color[4];
} MPropCol;

/** \} */

#ifdef __cplusplus
}
#endif

#endif // DNA_MESHDATA_TYPES_H