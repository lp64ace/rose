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
/** \name Custom Data (Vertex)
 * \{ */

typedef struct MDeformWeight {
	/** The index for the vertex group, must *always* be unique when in an array. */
	unsigned int def_nr;
	/** Weight between 0.0 and 1.0. */
	float weight;
} MDeformWeight;

typedef struct MDeformVert {
	/**
	 * Array of weight indices and values.
	 * - There must not be any duplicate #def_nr indices.
	 * - Groups in the array are unordered.
	 * - Indices outside the usable range of groups are ignored.
	 */
	struct MDeformWeight *dw;
	/**
	 * The length of the #dw array.
	 * \note This is not necessarily the same length as the total number of vertex groups.
	 * However, generally it isn't larger.
	 */
	int totweight;
	/** Flag is only in use as a run-time tag at the moment. */
	int flag;
} MDeformVert;

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