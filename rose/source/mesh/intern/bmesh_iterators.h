#pragma once

#include "mesh/bmesh_class.h"

typedef enum BMIterType {
	BM_VERTS_OF_MESH = 1 ,
	BM_EDGES_OF_MESH = 2 ,
	BM_FACES_OF_MESH = 3 ,
	
	BM_EDGES_OF_VERT = 4 ,
	BM_FACES_OF_VERT = 5 ,
	BM_LOOPS_OF_VERT = 6 ,
	BM_VERTS_OF_EDGE = 7 ,
	BM_FACES_OF_EDGE = 8 ,
	BM_VERTS_OF_FACE = 9 ,
	BM_EDGES_OF_FACE = 10 ,
	BM_LOOPS_OF_FACE = 11 ,

	/* iterate through loops around this loop, which are fetched
	* from the other faces in the radial cycle surrounding the
	* input loop's edge. */
	BM_LOOPS_OF_LOOP = 12 ,
	BM_LOOPS_OF_EDGE = 13 ,
} BMIterType;

#define BM_ITYPE_MAX 14

extern const byte bm_iter_itype_htype_map [ BM_ITYPE_MAX ];

/* -------------------------------------------------------------------- */
/** \name Defines for passing to #BM_iter_new.
* 
* "OF" can be substituted for "around" so #BM_VERTS_OF_FACE means "vertices* around a face."
* \{ */

#define BM_ITER_MESH(ele, iter, bm, itype) \
  for (BM_CHECK_TYPE_ELEM_ASSIGN(ele) = BM_iter_new(iter, bm, itype, NULL); ele; \
       BM_CHECK_TYPE_ELEM_ASSIGN(ele) = BM_iter_step(iter))

#define BM_ITER_MESH_INDEX(ele, iter, bm, itype, indexvar) \
  for (BM_CHECK_TYPE_ELEM_ASSIGN(ele) = BM_iter_new(iter, bm, itype, NULL), indexvar = 0; ele; \
       BM_CHECK_TYPE_ELEM_ASSIGN(ele) = BM_iter_step(iter), (indexvar)++)

/** a version of BM_ITER_MESH which keeps the next item in storage
* so we can delete the current item, see bug T36923. */
#ifdef _DEBUG
#  define BM_ITER_MESH_MUTABLE(ele, ele_next, iter, bm, itype) \
    for (BM_CHECK_TYPE_ELEM_ASSIGN(ele) = BM_iter_new(iter, bm, itype, NULL); \
         ele ? ((void)((iter)->count = BM_iter_mesh_count(itype, bm)), \
                (void)(BM_CHECK_TYPE_ELEM_ASSIGN(ele_next) = BM_iter_step(iter)), \
                1) : \
               0; \
         BM_CHECK_TYPE_ELEM_ASSIGN(ele) = ele_next)
#else
#  define BM_ITER_MESH_MUTABLE(ele, ele_next, iter, bm, itype) \
    for (BM_CHECK_TYPE_ELEM_ASSIGN(ele) = BM_iter_new(iter, bm, itype, NULL); \
         ele ? ((BM_CHECK_TYPE_ELEM_ASSIGN(ele_next) = BM_iter_step(iter)), 1) : 0; \
         ele = ele_next)
#endif

#define BM_ITER_ELEM(ele, iter, data, itype) \
  for (BM_CHECK_TYPE_ELEM_ASSIGN(ele) = BM_iter_new(iter, NULL, itype, data); ele; \
       BM_CHECK_TYPE_ELEM_ASSIGN(ele) = BM_iter_step(iter))

#define BM_ITER_ELEM_INDEX(ele, iter, data, itype, indexvar) \
  for (BM_CHECK_TYPE_ELEM_ASSIGN(ele) = BM_iter_new(iter, NULL, itype, data), indexvar = 0; ele; \
       BM_CHECK_TYPE_ELEM_ASSIGN(ele) = BM_iter_step(iter), (indexvar)++)

/** \} */

struct BMIter__elem_of_mesh {
	LIB_mempool_iter pooliter;
};
struct BMIter__edge_of_vert {
	BMVert *VertData;
	BMEdge *EdgeFirst , *EdgeNext;
};
struct BMIter__face_of_vert {
	BMVert *VertData;
	BMLoop *LoopFirst , *LoopNext;
	BMEdge *EdgeFirst , *EdgeNext;
};
struct BMIter__loop_of_vert {
	BMVert *VertData;
	BMLoop *LoopFirst , *LoopNext;
	BMEdge *EdgeFirst , *EdgeNext;
};
struct BMIter__loop_of_edge {
	BMEdge *EdgeData;
	BMLoop *LoopFirst , *LoopNext;
};
struct BMIter__loop_of_loop {
	BMLoop *LoopData;
	BMLoop *LoopFirst , *LoopNext;
};
struct BMIter__face_of_edge {
	BMEdge *EdgeData;
	BMLoop *LoopFirst , *LoopNext;
};
struct BMIter__vert_of_edge {
	BMEdge *EdgeData;
};
struct BMIter__vert_of_face {
	BMFace *FaceData;
	BMLoop *LoopFirst , *LoopNext;
};
struct BMIter__edge_of_face {
	BMFace *FaceData;
	BMLoop *LoopFirst , *LoopNext;
};
struct BMIter__loop_of_face {
	BMFace *FaceData;
	BMLoop *LoopFirst , *LoopNext;
};

typedef void ( *BMIter__begin_cb )( void * );
typedef void *( *BMIter__step_cb )( void * );

/* -------------------------------------------------------------------- */
/** \name Iterator Structure
* \{ */

typedef struct BMIter {
	union {
		struct BMIter__elem_of_mesh elem_of_mesh;
		struct BMIter__edge_of_vert edge_of_vert;
		struct BMIter__face_of_vert face_of_vert;
		struct BMIter__loop_of_vert loop_of_vert;
		struct BMIter__loop_of_edge loop_of_edge;
		struct BMIter__loop_of_loop loop_of_loop;
		struct BMIter__face_of_edge face_of_edge;
		struct BMIter__vert_of_edge vert_of_edge;
		struct BMIter__vert_of_face vert_of_face;
		struct BMIter__edge_of_face edge_of_face;
		struct BMIter__loop_of_face loop_of_face;
	} Data;

	BMIter__begin_cb Begin;
	BMIter__step_cb Step;

	int Count; /* NOTE: only some iterators set this, don't rely on it. */
	char IterType;
} BMIter;

/** \} */
