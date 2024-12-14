#ifndef RM_ITERATORS_H
#define RM_ITERATORS_H

#include "LIB_compiler_checktype.h"
#include "LIB_mempool.h"
#include "LIB_utildefines.h"

#ifdef __cplusplus
#	include "LIB_bit_span.hh"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RMIterType {
	RM_VERTS_OF_MESH = 1,
	RM_EDGES_OF_MESH = 2,
	RM_FACES_OF_MESH = 3,

	RM_EDGES_OF_VERT = 4,
	RM_FACES_OF_VERT = 5,
	RM_LOOPS_OF_VERT = 6,
	RM_VERTS_OF_EDGE = 7,
	RM_FACES_OF_EDGE = 8,
	RM_VERTS_OF_FACE = 9,
	RM_EDGES_OF_FACE = 10,
	RM_LOOPS_OF_FACE = 11,
	RM_LOOPS_OF_LOOP = 12,
	RM_LOOPS_OF_EDGE = 13,
} RMIterType;

#define RM_ITYPE_MAX 14

/* -------------------------------------------------------------------- */
/** \name Defines for passing to #RM_iter_new.
 *
 * "OF" can be substituted for "around" so #RM_VERTS_OF_FACE means "vertices* around a face."
 * \{ */

#define RM_ITER_MESH(ele, iter, mesh, itype) for (RM_CHECK_TYPE_ELEM_ASSIGN(ele) = RM_iter_new(iter, mesh, itype, NULL); ele; RM_CHECK_TYPE_ELEM_ASSIGN(ele) = RM_iter_step(iter))
#define RM_ITER_MESH_INDEX(ele, iter, mesh, itype, indexvar) for (RM_CHECK_TYPE_ELEM_ASSIGN(ele) = RM_iter_new(iter, mesh, itype, NULL), indexvar = 0; ele; RM_CHECK_TYPE_ELEM_ASSIGN(ele) = RM_iter_step(iter), (indexvar)++)

#ifndef NDEBUG
#	define RM_ITER_MESH_MUTABLE(ele, ele_next, iter, mesh, itype) for (RM_CHECK_TYPE_ELEM_ASSIGN(ele) = RM_iter_new(iter, mesh, itype, NULL); ele ? ((void)((iter)->count = RM_iter_mesh_count(itype, mesh)), (void)(RM_CHECK_TYPE_ELEM_ASSIGN(ele_next) = RM_iter_step(iter)), 1) : 0; RM_CHECK_TYPE_ELEM_ASSIGN(ele) = ele_next)
#else
#	define RM_ITER_MESH_MUTABLE(ele, ele_next, iter, mesh, itype) for (RM_CHECK_TYPE_ELEM_ASSIGN(ele) = RM_iter_new(iter, mesh, itype, NULL); ele ? ((RM_CHECK_TYPE_ELEM_ASSIGN(ele_next) = RM_iter_step(iter)), 1) : 0; ele = ele_next)
#endif

#define RM_ITER_ELEM(ele, iter, data, itype) for (RM_CHECK_TYPE_ELEM_ASSIGN(ele) = RM_iter_new(iter, NULL, itype, data); ele; RM_CHECK_TYPE_ELEM_ASSIGN(ele) = RM_iter_step(iter))
#define RM_ITER_ELEM_INDEX(ele, iter, data, itype, indexvar) for (RM_CHECK_TYPE_ELEM_ASSIGN(ele) = RM_iter_new(iter, NULL, itype, data), indexvar = 0; ele; RM_CHECK_TYPE_ELEM_ASSIGN(ele) = RM_iter_step(iter), (indexvar)++)

/* \} */

struct RMIter__elem_of_mesh {
	MemPoolIter pooliter;
};
struct RMIter__edge_of_vert {
	struct RMVert *vdata;
	struct RMEdge *e_first, *e_next;
};
struct RMIter__face_of_vert {
	struct RMVert *vdata;
	struct RMLoop *l_first, *l_next;
	struct RMEdge *e_first, *e_next;
};
struct RMIter__loop_of_vert {
	struct RMVert *vdata;
	struct RMLoop *l_first, *l_next;
	struct RMEdge *e_first, *e_next;
};
struct RMIter__loop_of_edge {
	struct RMEdge *edata;
	struct RMLoop *l_first, *l_next;
};
struct RMIter__loop_of_loop {
	struct RMLoop *ldata;
	struct RMLoop *l_first, *l_next;
};
struct RMIter__face_of_edge {
	struct RMEdge *edata;
	struct RMLoop *l_first, *l_next;
};
struct RMIter__vert_of_edge {
	struct RMEdge *edata;
};
struct RMIter__vert_of_face {
	struct RMFace *pdata;
	struct RMLoop *l_first, *l_next;
};
struct RMIter__edge_of_face {
	struct RMFace *pdata;
	struct RMLoop *l_first, *l_next;
};
struct RMIter__loop_of_face {
	struct RMFace *pdata;
	struct RMLoop *l_first, *l_next;
};

typedef void (*RMIter__begin_cb)(void *);
typedef void *(*RMIter__step_cb)(void *);

/* Iterator Structure */
/* NOTE: some of these vars are not used,
 * so they have been commented to save stack space since this struct is used all over */
typedef struct RMIter {
	/* keep union first */
	union {
		struct RMIter__elem_of_mesh elem_of_mesh;

		struct RMIter__edge_of_vert edge_of_vert;
		struct RMIter__face_of_vert face_of_vert;
		struct RMIter__loop_of_vert loop_of_vert;
		struct RMIter__loop_of_edge loop_of_edge;
		struct RMIter__loop_of_loop loop_of_loop;
		struct RMIter__face_of_edge face_of_edge;
		struct RMIter__vert_of_edge vert_of_edge;
		struct RMIter__vert_of_face vert_of_face;
		struct RMIter__edge_of_face edge_of_face;
		struct RMIter__loop_of_face loop_of_face;
	} data;

	RMIter__begin_cb begin;
	RMIter__step_cb step;

	int count; /* NOTE: only some iterators set this, don't rely on it. */
	char itype;
} RMIter;

/**
 * This runs in linear time for \a index, avoid using it when accessing mesh array elements such as
 * vert/edge/face of mesh. \note Use #RM_vert_at_index / #RM_edge_at_index / #RM_face_at_index for
 * mesh arrays.
 */
void *RM_iter_at_index(struct RMesh *mesh, char itype, void *data, int index);

/**
 * Sometimes its convenitent to get the iterator as an array to avoid multiple calls to #RM_iter_at_index.
 */
int RM_iter_as_array(struct RMesh *mesh, char itype, void *data, void **array, int len);

/**
 * Allocates a new array, has the advantage that you don't need to know the size ahead of time.
 *
 * Takes advantage of less common iterator usage to avoid counting twice,
 * which you might end up doing when #RM_iter_as_array is used.
 *
 * Called needs to free the array.
 */
void *RM_iter_as_arrayN(struct RMesh *mesh, char itype, void *data, int *r_len, void **stack_array, int stack_array_size);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
int RM_iter_mesh_bitmap_from_filter(char itype, struct RMesh *mesh, rose::MutableBitSpan bitmap, bool (*test_fn)(struct RMElem *, void *user_data), void *user_data);
/**
 * Needed when we want to check faces, but return a loop aligned array.
 */
int RM_iter_mesh_bitmap_from_filter_tessface(struct RMesh *bm, rose::MutableBitSpan bitmap, bool (*test_fn)(struct RMFace *, void *user_data), void *user_data);
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Elem Iter Flag Count
 *
 * Counts how many flagged / unflagged items are found in this element.
 */
int RM_iter_elem_count_flag(char itype, void *data, char hflag, bool value);
/**
 * Utility function.
 */
int RM_iter_mesh_count(char itype, struct RMesh *bm);
/**
 * \brief Mesh Iter Flag Count
 *
 * Counts how many flagged / unflagged items are found in this mesh.
 */
int RM_iter_mesh_count_flag(char itype, struct RMesh *bm, char hflag, bool value);

/* private for rmesh_iterators_inline.c */

#define RMITER_CB_DEF(name) \
	struct RMIter__##name; \
	void rmiter__##name##_begin(struct RMIter__##name *iter); \
	void *rmiter__##name##_step(struct RMIter__##name *iter)

RMITER_CB_DEF(elem_of_mesh);
RMITER_CB_DEF(edge_of_vert);
RMITER_CB_DEF(face_of_vert);
RMITER_CB_DEF(loop_of_vert);
RMITER_CB_DEF(loop_of_edge);
RMITER_CB_DEF(loop_of_loop);
RMITER_CB_DEF(face_of_edge);
RMITER_CB_DEF(vert_of_edge);
RMITER_CB_DEF(vert_of_face);
RMITER_CB_DEF(edge_of_face);
RMITER_CB_DEF(loop_of_face);

#undef RMITER_CB_DEF

#include "intern/rm_iterators_inline.h"

#ifdef __cplusplus
}
#endif

#endif // RM_ITERATORS_H
