#pragma once

#include "dna/dna_customdata_types.h"

#include "lib/lib_mempool.h"
#include "lib/lib_compiler_typecheck.h"
#include "lib/lib_error.h"
#include "lib/lib_typedef.h"
#include "lib/lib_alloc.h"

/* -------------------------------------------------------------------- */
/** \name Pro mesh element structures.
* \{ */

typedef struct BMHeader {
	// Customdata layers assosiated with this element.
	void *Data;

	/** Uninitialized to - 1 so we can easily tell its not set. 
	* Check PMesh.ElemIndexDirty for valid index values, this is might be 
	* set dirty by various tool. */
	int Index;

	/** The element's geometric type. 
	* This can be any of the following values  BM_VERT | BM_EDGE | BM_FACE | BM_LOOP. */
	byte ElemType;

	// The element's generic flag (i.e. PM_ELEM_HIDDEN).
	byte ElemFlag;

	/** Internal pro mesh api flag, do not touch!
	* \note We are verty picky about not bloating this struct but in this case it is already padded to 16 bytes 
	* anyway so adding a flag here gives no increase in size. */
	byte ApiFlag;
} BMHeader;

typedef struct BMVert {
	BMHeader Head;

	float Coord [ 3 ]; // Vertex coordinates.
	float Normal [ 3 ]; // Vertex normal.

	struct BMEdge *Edge;
} BMVert;

// Disk link structure, only uses by edges.
typedef struct BMDiskLink {
	struct BMEdge *Next , *Prev;
} BMDiskLink;

typedef struct BMEdge {
	BMHeader Head;

	/** Vertices (unordered), although the order can be used at times.
	* Operations that create/subdivide edges shouldn't flip the order unless there is a
	* good reason to do so. */
	struct BMVert *Vert1 , *Vert2;

	/** The list of loops around the edge, see #PMLoop.RadialNext for an example of
	* using this to loop overt all faces used by an edge. */
	struct BMLoop *Loop;

	/** Disk Cycle Pointers.
	* Relative data: Disk1 indicates the next/prev edge around Vert1 and Disk2 does
	* the same for Vert2. */
	struct BMDiskLink DiskLink1 , DiskLink2;
} BMEdge;

typedef struct BMLoop {
	BMHeader Head;

	/** The vertex this loop points to. 
	* - This vertex must be unique within the cycle. */
	struct BMVert *Vert;

	/** The edge this loop uses. 
	* 
	* Vertices #PMLoop.Vert & #PMLoop.Next.Vert always contain vertices from 
	* #PMEdge.Vert1 & #PMEdge.Vert2. Altough no assumtions can be made about the order, 
	* as this isn't meaningul for mesh topology.
	* 
	* - This edge must be unique within the cucle. */
	struct BMEdge *Edge;

	/** The face this loop is part of. 
	* 
	* - This face must be shared by all within the cycle. 
	* 
	* Used as a back-pointer so loops can know the face they define. */
	struct BMFace *Face;

	/** Other loops connected to this edge. 
	* This is typically use for accessing an edges faces, 
	* however this is done bu stepping over it's loops.
	* 
	* - This is a cirtular list, so there are no first/last storage of the "radial" data. 
	* Instead #PMEdge.Loop points to any one of the loops that use it. 
	* 
	* - The value for #PMLoop.Vert might not match the radial next/previous 
	* as this depends on the face-winding. You can be sure #BMLoop.Vert will be either 
	* #PMEdge.Vert1 or #PMEdge.Vert2 of #PMLoop.Edge. 
	* 
	* - Unlike face-winding (which defines if the direction the face points), 
	* next and previous are insignificant. The list could be reversed for example, 
	* without any impact on the topology. 
	* 
	* - This is an example of loop over an edges faces using PMLoop.RadialNext. 
	* 
	* \code{.c}
	* PMLoop *iter = edge->Loop;
	* do {
	*	operate_on_face ( iter->Face );
	* } while ( ( iter = iter->RadialNext ) != edge->Loop );
	* \endcode */
	struct BMLoop *RadialNext , *RadialPrev;

	/** Other loops that are part of this face. 
	* This is typically used for accessing all vertices/edges in a faces. 
	* 
	* - This is a circular list, so there are no first/last storage of the "cycle" data. 
	* Instead #PMFace.LoopFirst points to any one of the loops that are part of this face. 
	* 
	* - Since the list is circular, the particular loop referenced doesn't matter, 
	* as all other loops can be accessed from it. 
	* 
	* - Every loop in this "cycle" list has the same value for #PMLoop.Face. 
	* 
	* - The direction of this list defines the face winding. 
	* Reversing the list flips the face.
	* 
	* This is an example loop over all vertices and edges of a face. 
	* \code{.c}
	* PMLoop *l_first, *l_iter;
	* l_first = l_iter = PM_FACE_FIRST_LOOP(face);
	* do {
	*	operate_on_vert ( l_iter->Vert );
	*	operate_on_vert ( l_iter->Edge );
	* } while ( ( l_iter = l_iter->Next ) != l_first );
	* \endcode */
	struct BMLoop *Next , *Prev;
} BMLoop;

// Can cast anything to this.
typedef struct BMElem {
	BMHeader Head;
} BMElem;

typedef struct BMFace {
	BMHeader Head;

	/** Since the list is circular, the particular loop referenced doesn't matter, 
	* as all other loops can be accessed from it, so this points to any one of the 
	* loops that are part of this face.  */
	BMLoop *LoopFirst;

	/** Number of vertices in the face, (the length of #BMFace.LoopFirst circular 
	* linked list). */
	int Len;

	float Normal [ 3 ]; // Face normal.

	/** Material index, typically >= 0 & < #Mesh.TotCol although this isn't 
	* enforced. When using to index into a material array it's range should 
	* be checked first, values exceeding the range should be ignored or 
	* treated as zero ( if a material slots needs to be used - when drawing 
	* for e.g. ). */
	short MatNum;
} BMFace;

typedef struct BMesh {
	int TotVert;
	int TotEdge;
	int TotLoop;
	int TotFace;

	/** Flag index arrays as beeing dirty so we can check if they are clean 
	* and avoid looping over the entire vert/edge/face/loop array in those cases. 
	* valid flags are: `( BM_VERT | BM_EDGE | BM_FACE | BM_LOOP )`. */
	byte ElemIndexDirty;

	/** Flag array table as being dirty so we know when its safe to use it, 
	* or when it needs to be re-created. */
	byte ElemTableDirty;

	// Element pools.

	struct LIB_mempool *VertPool;
	struct LIB_mempool *EdgePool;
	struct LIB_mempool *LoopPool;
	struct LIB_mempool *FacePool;

	/** Mempool lookup tables (optional) index tables, to map 
	* indices. Don't touch this or read it directly. */
	BMVert **VertTable;
	BMEdge **EdgeTable;
	BMFace **FaceTable;

	int VertTableTot;
	int EdgeTableTot;
	int FaceTableTot;

	CustomData VertData , EdgeData , LoopData , FaceData;
} BMesh;

/** \} */

enum {
	BM_VERT = 1 ,
	BM_EDGE = 2 ,
	BM_LOOP = 4 ,
	BM_FACE = 8 ,
};

#define BM_ALL		(BM_VERT | BM_EDGE | BM_LOOP | BM_FACE)
#define BM_ALL_NOLOOP	(BM_VERT | BM_EDGE | BM_FACE)

/* -------------------------------------------------------------------- */
/** \name Common Defines.
* \{ */

#define _BM_GENERIC_TYPE_ELEM_NONCONST	void *, BMVert *, BMEdge *, BMLoop *, BMFace *, BMElem *, BMHeader *
#define _BM_GENERIC_TYPE_ELEM_CONST	const void *, const BMVert *, const BMEdge *, const BMLoop *, const BMFace *, const BMElem *, const BMHeader *

#define BM_CHECK_TYPE_ELEM_CONST(ele) \
  CHECK_TYPE_ANY(ele, _BM_GENERIC_TYPE_ELEM_CONST)

#define BM_CHECK_TYPE_ELEM_NONCONST(ele) \
  CHECK_TYPE_ANY(ele, _BM_GENERIC_TYPE_ELEM_NONCONST)

#define BM_CHECK_TYPE_ELEM(ele) \
  CHECK_TYPE_ANY(ele, _BM_GENERIC_TYPE_ELEM_NONCONST, _BM_GENERIC_TYPE_ELEM_CONST)

// Vertices

#define _BM_GENERIC_TYPE_VERT_NONCONST	BMVert *
#define _BM_GENERIC_TYPE_VERT_CONST	const BMVert *

#define BM_CHECK_TYPE_VERT_CONST(ele) \
  CHECK_TYPE_ANY(ele, _BM_GENERIC_TYPE_VERT_CONST)

#define BM_CHECK_TYPE_VERT_NONCONST(ele) \
  CHECK_TYPE_ANY(ele, _BM_GENERIC_TYPE_ELEM_NONCONST)

#define BM_CHECK_TYPE_VERT(ele) \
  CHECK_TYPE_ANY(ele, _BM_GENERIC_TYPE_VERT_NONCONST, _BM_GENERIC_TYPE_VERT_CONST)

// Edge

#define _BM_GENERIC_TYPE_EDGE_NONCONST	BMEdge *
#define _BM_GENERIC_TYPE_EDGE_CONST	const BMEdge *

#define BM_CHECK_TYPE_EDGE_CONST(ele) \
  CHECK_TYPE_ANY(ele, _BM_GENERIC_TYPE_EDGE_CONST)

#define BM_CHECK_TYPE_EDGE_NONCONST(ele) \
  CHECK_TYPE_ANY(ele, _BM_GENERIC_TYPE_ELEM_NONCONST)

#define BM_CHECK_TYPE_EDGE(ele) \
  CHECK_TYPE_ANY(ele, _BM_GENERIC_TYPE_EDGE_NONCONST, _BM_GENERIC_TYPE_EDGE_CONST)

// Face

#define _BM_GENERIC_TYPE_FACE_NONCONST	BMFace *
#define _BM_GENERIC_TYPE_FACE_CONST	const BMFace *
#define BM_CHECK_TYPE_FACE_CONST(ele) \
  CHECK_TYPE_ANY(ele, _BM_GENERIC_TYPE_FACE_CONST)
#define BM_CHECK_TYPE_FACE_NONCONST(ele) \
  CHECK_TYPE_ANY(ele, _BM_GENERIC_TYPE_ELEM_NONCONST)
#define BM_CHECK_TYPE_FACE(ele) \
  CHECK_TYPE_ANY(ele, _BM_GENERIC_TYPE_FACE_NONCONST, _BM_GENERIC_TYPE_FACE_CONST)

#ifdef __cplusplus
#  define BM_CHECK_TYPE_ELEM_ASSIGN(ele) (BM_CHECK_TYPE_ELEM(ele)), *((void **)&ele)
#else
#  define BM_CHECK_TYPE_ELEM_ASSIGN(ele) (BM_CHECK_TYPE_ELEM(ele)), ele
#endif

#define BM_ELEM_CD_SET_INT(ele, offset, f) \
		{ \
			CHECK_TYPE_NONCONST(ele); \
			LIB_assert(offset != -1); \
			*((int *)((char *)(ele)->Head.Data + (offset))) = (f); \
		} \
		(void)0

#define BM_ELEM_CD_GET_INT(ele, offset) \
		(LIB_assert(offset != -1), *((int *)((char *)(ele)->Head.Data + (offset))))

#define BM_ELEM_CD_SET_BOOL(ele, offset, f) \
		{ \
			CHECK_TYPE_NONCONST(ele); \
			LIB_assert(offset != -1); \
			*((bool *)((char *)(ele)->Head.Data + (offset))) = (f); \
		} \
		(void)0

#define BM_ELEM_CD_GET_BOOL(ele, offset) \
		(LIB_assert(offset != -1), *((bool *)((char *)(ele)->Head.Data + (offset))))

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#  define BM_ELEM_CD_GET_VOID_P(ele, offset) \
		(LIB_assert(offset != -1), \
		_Generic(ele, \
			GENERIC_TYPE_ANY(POINTER_OFFSET((ele)->Head.Data, offset), _BM_GENERIC_TYPE_ELEM_NONCONST), \
			GENERIC_TYPE_ANY((const void *)POINTER_OFFSET((ele)->Head.Data, offset), _BM_GENERIC_TYPE_ELEM_CONST)))
#else
#  define BM_ELEM_CD_GET_VOID_P(ele, offset) \
		(LIB_assert(offset != -1), (void *)((char *)(ele)->Head.Data + (offset)))
#endif

#define BM_ELEM_CD_SET_FLOAT(ele, offset, f) \
		{ \
			CHECK_TYPE_NONCONST(ele); \
			LIB_assert(offset != -1); \
			*((float *)((char *)(ele)->Head.Data + (offset))) = (f); \
		} \
		(void)0

#define BM_ELEM_CD_GET_FLOAT(ele, offset) \
		(LIB_assert(offset != -1), *((float *)((char *)(ele)->Head.Data + (offset))))

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#  define BM_ELEM_CD_GET_FLOAT_P(ele, offset) \
		(LIB_assert(offset != -1), \
		_Generic(ele, \
			GENERIC_TYPE_ANY((float *)POINTER_OFFSET((ele)->Head.Data, offset), _BM_GENERIC_TYPE_ELEM_NONCONST), \
			GENERIC_TYPE_ANY((const float *)POINTER_OFFSET((ele)->Head.Data, offset), _BM_GENERIC_TYPE_ELEM_CONST)))
#  define BM_ELEM_CD_GET_FLOAT2_P(ele, offset) \
		(LIB_assert(offset != -1), \
		_Generic(ele, \
			GENERIC_TYPE_ANY((float (*)[2])POINTER_OFFSET((ele)->Head.Data, offset), _BM_GENERIC_TYPE_ELEM_NONCONST), \
			GENERIC_TYPE_ANY((const float (*)[3])POINTER_OFFSET((ele)->Head.Data, offset), _BM_GENERIC_TYPE_ELEM_CONST)))
#  define BM_ELEM_CD_GET_FLOAT3_P(ele, offset) \
		(LIB_assert(offset != -1), \
		_Generic(ele, \
			GENERIC_TYPE_ANY((float (*)[3])POINTER_OFFSET((ele)->Head.Data, offset), _BM_GENERIC_TYPE_ELEM_NONCONST), \
			GENERIC_TYPE_ANY((const float (*)[3])POINTER_OFFSET((ele)->Head.Data, offset), _BM_GENERIC_TYPE_ELEM_CONST)))
#else
#  define BM_ELEM_CD_GET_FLOAT_P(ele, offset) \
		(LIB_assert(offset != -1), (float *)((char *)(ele)->Head.Data + (offset)))
#  define BM_ELEM_CD_GET_FLOAT2_P(ele, offset) \
		(LIB_assert(offset != -1), (float (*)[2])((char *)(ele)->Head.Data + (offset)))
#  define BM_ELEM_CD_GET_FLOAT3_P(ele, offset) \
		(LIB_assert(offset != -1), (float (*)[3])((char *)(ele)->Head.Data + (offset)))
#endif

#define BM_ELEM_CD_SET_FLOAT2(ele, offset, f) \
		{ \
			CHECK_TYPE_NONCONST(ele); \
			LIB_assert(offset != -1); \
			((float *)((char *)(ele)->Head.Data + (offset)))[0] = (f)[0]; \
			((float *)((char *)(ele)->Head.Data + (offset)))[1] = (f)[1]; \
		} \
		(void)0
#define BM_ELEM_CD_SET_FLOAT3(ele, offset, f) \
		{ \
			CHECK_TYPE_NONCONST(ele); \
			LIB_assert(offset != -1); \
			((float *)((char *)(ele)->Head.Data + (offset)))[0] = (f)[0]; \
			((float *)((char *)(ele)->Head.Data + (offset)))[1] = (f)[1]; \
			((float *)((char *)(ele)->Head.Data + (offset)))[2] = (f)[2]; \
		} \
		(void)0

#define BM_ELEM_CD_GET_FLOAT_AS_UCHAR(ele, offset) \
		(LIB_assert(offset != -1), (unsigned char)(BM_ELEM_CD_GET_FLOAT(ele, offset) * 255.0f))

#define BM_FACE_FIRST_LOOP(p)	((p)->LoopFirst)

#define BM_DISK_EDGE_NEXT(e,v) \
		(CHECK_TYPE_INLINE(e, MeshEdge *), \
		 CHECK_TYPE_INLINE(v, MeshVert *), \
		 (((&(e)->Vert1DiskLink)[v == (e)->Vert2]).Next))
#define BM_DISK_EDGE_PREV(e,v) \
		(CHECK_TYPE_INLINE(e, MeshEdge *), \
		 CHECK_TYPE_INLINE(v, MeshVert *), \
		 (((&(e)->Vert1DiskLink)[v == (e)->Vert2]).Prev))

/** \} */

enum {
	/** Mark this element as hiddent so that it will not 
	* be displayed. */
	BM_ELEM_HIDDEN = ( 1 << 0 ),

	/** Internal flag, used for ensuring corrent normals and any other time 
	* when temp tagging is handy. Always assume dirty & clear before use. */
	BM_ELEM_TAG = ( 1 << 1 ),

	// Edge display
	BM_ELEM_DRAW = ( 1 << 2 ),

	/** For low level internal API tagging, 
	* since tools may want to tag verts and not have functions clobber them. 
	* Leave cleared!
	*/
	BM_ELEM_INTERNAL_TAG = ( 1 << 3 ),
};

/* -------------------------------------------------------------------- */
/** \name Utility Defines
* \{ */

#define BM_FACE_FIRST_LOOP(p)	((p)->LoopFirst)

#define BM_DISK_EDGE_NEXT(e, v) \
  (CHECK_TYPE_INLINE(e, BMEdge *), \
   CHECK_TYPE_INLINE(v, BMVert *), \
   LIB_assert(ELEM(v,e->Vert1,e->Vert2)), \
   (((&e->DiskLink1)[v == e->Vert2]).Next))
#define BM_DISK_EDGE_PREV(e, v) \
  (CHECK_TYPE_INLINE(e, BMEdge *), \
   CHECK_TYPE_INLINE(v, BMVert *), \
   LIB_assert(ELEM(v,e->Vert1,e->Vert2)), \
   (((&e->DiskLink1)[v == e->Vert2]).Prev))

/** \} */

/* -------------------------------------------------------------------- */
/** \name BMesh Constants
* \{ */

#define BM_DEFAULT_NGON_STACK_SIZE	32
#define BM_DEFAULT_ITER_STACK_SIZE	16

#define BM_LOOP_RADIAL_MAX		((int)0x0000ffff)
#define BM_NGON_MAX			((int)0x000fffff)

/** \} */
