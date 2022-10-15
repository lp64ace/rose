#pragma once

#include "lib/lib_typedef.h"

/* -------------------------------------------------------------------- */
/** \name Geometry Elements
 * \{ */

/** Mesh Vertices. 
* Typically accessed from #Mesh.mvert */
typedef struct MVert {
	float Coord [ 3 ];
	byte Flag;
} MVert;

/** #MVert.flag */

enum {
	/*  SELECT = (1 << 0), */
	/** Deprecated hide status. Now stored in ".hide_vert" attribute. */
	ME_HIDE = ( 1 << 4 ) ,
};

/** Mesh Edges.
* Typically accessed from #Mesh.medge */
typedef struct MEdge {
	/** Un-ordered vertex indices (cannot match). */
	unsigned int Vert1 , Vert2;

	byte Crease;

	short Flag;
} MEdge;

/** #MEdge.flag */
enum {
	ME_EDGEDRAW = ( 1 << 1 ) ,
	ME_SEAM = ( 1 << 2 ) ,
	/** Deprecated hide status. Now stored in ".hide_edge" attribute. */
	/*  ME_HIDE = (1 << 4), */
	ME_EDGERENDER = ( 1 << 5 ) ,
	ME_LOOSEEDGE = ( 1 << 7 ) ,
	ME_SHARP = ( 1 << 9 ) , /* only reason this flag remains a 'short' */
};

/** Mesh Faces 
* This only stores the polygon size & flags, the vertex & edge indices are stored in the #MLoop. 
* Typically accessed from #Mesh.mpoly. */
typedef struct MPoly {
	/** Offset into loop array and number of loops in the face. */
	int LoopStart;
	/** Keep signed since we need to subtract when getting the previous loop. */
	int TotLoop;
	/** Deprecated material index. Now stored in the "material_index" attribute, but kept for IO. */
	short MatNum;

	byte Flag;
} MPoly;

/** #MPoly.flag */
enum {
	ME_SMOOTH = ( 1 << 0 ) ,
	ME_FACE_SEL = ( 1 << 1 ) ,
	/** Deprecated hide status. Now stored in ".hide_poly" attribute. */
	/* ME_HIDE = (1 << 4), */
};

/** Mesh Face Corners. 
* "Loop" is an internal name for the corner of a polygon (#MPoly). 
* Typically accessed from #Mesh.mloop. */
typedef struct MLoop {
	/** Vertex index into an #MVert array. */
	unsigned int Vert;
	/** Edge index into an #MEdge array. */
	unsigned int Edge;
} MLoop;

/** \} */

typedef struct MLoopUV {
	float TexCoord [ 2 ];
	int Flag;
} MLoopUV;

typedef struct MLoopCol {
	unsigned char r , g , b , a;
} MLoopCol;

typedef struct MPropCol {
	float color [ 4 ];
} MPropCol;

typedef struct MStringProperty {
	char s [ 255 ] , len;
} MStringProperty;
