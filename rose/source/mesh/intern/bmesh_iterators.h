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
	byte IterType;
} BMIter;

/** \} */

/**
* \note Use #BM_vert_at_index / #BM_edge_at_index / #BM_face_at_index for mesh arrays.
*/
void *BM_iter_at_index ( BMesh *bm , byte itype , void *data , int index ) ATTR_WARN_UNUSED_RESULT;

/**
* \brief Iterator as Array
*
* Sometimes its convenient to get the iterator as an array
* to avoid multiple calls to #BM_iter_at_index.
*/
int BM_iter_as_array ( BMesh *bm , byte itype , void *data , void **array , int len );

/**
* \brief Iterator as Array
*
* Allocates a new array, has the advantage that you don't need to know the size ahead of time.
*
* Takes advantage of less common iterator usage to avoid counting twice,
* which you might end up doing when #BM_iter_as_array is used.
*
* Caller needs to free the array.
*/
void *BM_iter_as_arrayN ( BMesh *bm ,
			  byte itype ,
			  void *data ,
			  int *r_len ,
			  void **stack_array ,
			  int stack_array_size ) ATTR_WARN_UNUSED_RESULT;

int BM_iter_mesh_bitmap_from_filter ( byte itype , BMesh *bm , unsigned int *bitmap , bool ( *test_fn )( BMElem * , void *user_data ) , void *user_data );
/**
* Needed when we want to check faces, but return a loop aligned array.
*/
int BM_iter_mesh_bitmap_from_filter_tessface ( BMesh *bm , unsigned int *bitmap , bool ( *test_fn )( BMFace * , void *user_data ) , void *user_data );

/**
* \brief Elem Iter Flag Count
*
* Counts how many flagged / unflagged items are found in this element.
*/
int BM_iter_elem_count_flag ( char itype , void *data , char hflag , bool value );

/**
* \brief Elem Iter Tool Flag Count
*
* Counts how many flagged / unflagged items are found in this element.
*/
int BMO_iter_elem_count_flag ( BMesh *bm , byte itype , void *data , short oflag , bool value );

/**
* Utility function.
*/
int BM_iter_mesh_count ( byte itype , BMesh *bm );

/**
* \brief Mesh Iter Flag Count
*
* Counts how many flagged / unflagged items are found in this mesh.
*/
int BM_iter_mesh_count_flag ( byte itype , BMesh *bm , char hflag , bool value );

#define BMITER_CB_DEF(name) \
  struct BMIter__##name; \
  void bmiter__##name##_begin(struct BMIter__##name *iter); \
  void *bmiter__##name##_step(struct BMIter__##name *iter)

BMITER_CB_DEF ( elem_of_mesh );
BMITER_CB_DEF ( edge_of_vert );
BMITER_CB_DEF ( face_of_vert );
BMITER_CB_DEF ( loop_of_vert );
BMITER_CB_DEF ( loop_of_edge );
BMITER_CB_DEF ( loop_of_loop );
BMITER_CB_DEF ( face_of_edge );
BMITER_CB_DEF ( vert_of_edge );
BMITER_CB_DEF ( vert_of_face );
BMITER_CB_DEF ( edge_of_face );
BMITER_CB_DEF ( loop_of_face );

#undef BMITER_CB_DEF

#include "bmesh_iterators_inline.h"

#define BM_ITER_CHECK_TYPE_DATA(data) \
  CHECK_TYPE_ANY(data, void *, BMFace *, BMEdge *, BMVert *, BMLoop *, BMElem *)

#define BM_iter_new(iter, bm, itype, data) \
  (BM_ITER_CHECK_TYPE_DATA(data), BM_iter_new(iter, bm, itype, data))
#define BM_iter_init(iter, bm, itype, data) \
  (BM_ITER_CHECK_TYPE_DATA(data), BM_iter_init(iter, bm, itype, data))
