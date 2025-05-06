#ifndef RM_TYPE_H
#define RM_TYPE_H

#include "DNA_customdata_types.h"

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Topology
 * \{ */

typedef struct RMHeader {
	void *data;
	
	/**
	 * The index assigned to this specific element.
	 * - Use #RM_elem_index_get/set macros to access, uninitialized to -1 so we can easily tell its not set.
	 * - Used for edge/vert/face/loop, check for the #RMesh->elem_index_dirty for valid index values.
	 */
	int index;
	
	char htype;
	char hflag;
	/**
	 * Internal use only!
	 * \note We are very picky about not bloating this struct, 
	 * but in this case its padded up to 16 bytes anyway, so adding a flag here gives no increase in size.
	 */
	char api_flag;
} RMHeader;

/** #RMHeader->htype (char) */
enum {
	RM_VERT = (1 << 0),
	RM_EDGE = (1 << 1),
	RM_LOOP = (1 << 2),
	RM_FACE = (1 << 3),
};

#define RM_ALL (RM_VERT | RM_EDGE | RM_LOOP | RM_FACE)
#define RM_ALL_NOLOOP (RM_VERT | RM_EDGE | RM_FACE)

/** #RMHeader->hflag (char) */
enum {
	RM_ELEM_HIDDEN = 1 << 1,
	RM_ELEM_SMOOTH = 1 << 2,
	RM_ELEM_DRAW = 1 << 3,
	
	RM_ELEM_TAG = 1 << 4,
	RM_ELEM_TAG_ALT = 1 << 5,
	RM_ELEM_INTERNAL_TAG = 1 << 6,
};

ROSE_STATIC_ASSERT(sizeof(RMHeader) <= 16, "RMHeader size has grown!");

typedef struct RMVert {
	RMHeader head;
	
	float co[3];
	float no[3];
	
	/**
	 * Pointer to (any) edge using this vertex (for disk cycles).
	 * \note Some higher level functions may set this to different edges that use this vertex, 
	 * which is a bit of an abuse of internal #RMhesh data but also works OK.
	 * (use with care!).
	 */
	struct RMEdge *e;
} RMVert;

typedef struct RMVert_OFlag {
	RMVert base;
	
	struct RMFlagLayer *oflags;
} RMVert_OFlag;

typedef struct RMDiskLink {
	struct RMEdge *prev, *next;
} RMDiskLink;

typedef struct RMEdge {
	RMHeader head;
	
	/**
	 * Vertices (unordered).
	 *
	 * Although the order can be used at times.
	 * Operations that create/subdivide edges shouldn't flip the order 
	 * unlesss there is a good reason to do so.
	 */
	struct RMVert *v1, *v2;
	/**
	 * The list of loops around the edge.
	 */
	struct RMLoop *l;
	/**
	 * Disk Cycle Pointers.
	 *
	 * Relative data: d1 indicates the next/prev edge around vertex v1,
	 * d2 does the same for v2.
	 */
	struct RMDiskLink v1_disk_link, v2_disk_link;
} RMEdge;

typedef struct RMEdge_OFlag {
	RMEdge base;
	
	struct RMFlagLayer *oflags;
} RMEdge_OFlag;

typedef struct RMLoop {
	RMHeader head;
	
	/**
	 * The vertex this loop points to.
	 * The vertex must be unique within that cycle.
	 */
	struct RMVert *v;
	/**
	 * The edge this loop uses.
	 * The vertices (#RMLoop.v & RMLoop.next.v) always cotain vertices from (#RMEdge.v1, #RMEdge.v2).
	 * Although no assumptions can be made about the order, as this isn't meaningul for mesh topology.
	 *
	 * - This edge must be unique within the cycle *defined by #RMLoop.next & #RMLoop.prev links).
	 */
	struct RMEdge *e;
	/**
	 * The face this loop is part of.
	 * This face must be shared by all within the cycle, 
	 * used as a back-pointer so loops can know the face they defined.
	 */
	struct RMFace *f;
	
	/**
	* Other loops connected to this edge.
	*
	* This is typically use for accessing an edges faces,
	* however this is done by stepping over it's loops.
	*
	* - This is a circular list, so there are no first/last storage of the "radial" data.
	*   Instead #BMEdge.l points to any one of the loops that use it.
	*
	* - Since the list is circular, the particular loop referenced doesn't matter,
	*   as all other loops can be accessed from it.
	*
	* - Every loop in this radial list has the same value for #BMLoop.e.
	*
	* - The value for #BMLoop.v might not match the radial next/previous
	*   as this depends on the face-winding.
	*   You can be sure #BMLoop.v will either #BMEdge.v1 or #BMEdge.v2 of #BMLoop.e,
	*
	* - Unlike face-winding (which defines if the direction the face points),
	*   next and previous are insignificant. The list could be reversed for example,
	*   without any impact on the topology.
	*
	* This is an example of looping over an edges faces using #BMLoop.radial_next.
	*
	* \code{.c}
	* RMLoop *l_iter = edge->l;
	* do {
	*   operate_on_face(l_iter->f);
	* } while ((l_iter = l_iter->radial_next) != edge->l);
	* \endcode
	*/
	struct RMLoop *radial_prev, *radial_next;
	/**
	* Other loops that are part of this face.
	*
	* This is typically used for accessing all vertices/edges in a faces.
	*
	* - This is a circular list, so there are no first/last storage of the "cycle" data.
	*   Instead #BMFace.l_first points to any one of the loops that are part of this face.
	*
	* - Since the list is circular, the particular loop referenced doesn't matter,
	*   as all other loops can be accessed from it.
	*
	* - Every loop in this "cycle" list has the same value for #BMLoop.f.
	*
	* - The direction of this list defines the face winding.
	*   Reversing the list flips the face.
	*
	* This is an example loop over all vertices and edges of a face.
	*
	* \code{.c}
	* RMLoop *l_first, *l_iter;
	* l_iter = l_first = BM_FACE_FIRST_LOOP(f);
	* do {
	*   operate_on_vert(l_iter->v);
	*   operate_on_edge(l_iter->e);
	* } while ((l_iter = l_iter->next) != l_first);
	* \endcode
	*/
	struct RMLoop *prev, *next;
} RMLoop;

/* can cast RMFace/RMEdge/RMVert, but NOT RMLoop, since these don't have a flag layer */
typedef struct RMElemF {
	RMHeader head;
} RMElemF;

/* can cast anything to this, including RMLoop */
typedef struct RMElem {
	RMHeader head;
} RMElem;


typedef struct RMFace {
	RMHeader head;
	
	/**
	* A face is defined as a linked list of vertices (loop).
	*/
	struct RMLoop *l_first;
	
	/**
	* Number of vertices in the face.
	* 
	* - The length of #RMFace.l_first circular linked list.
	*/
	int len;
	
	/**
	* The normal of the face, see #RM_face_calc_normal.
	*/
	float no[3];
	
	/**
	* When using to index into a material array it's range should be checked first,
	* values exceeding the range should be ignored or treated as zero
	* (if a material slot needs to be used - when drawing for e.g.)
	*/
	short mat_nr;
} RMFace;

#define RM_FACE_FIRST_LOOP(p) ((p)->l_first)
#define RM_DISK_EDGE_NEXT(e, v) (ROSE_assert(RM_vert_in_edge(e, v)), (((&e->v1_disk_link)[v == e->v2]).next))
#define RM_DISK_EDGE_PREV(e, v) (ROSE_assert(RM_vert_in_edge(e, v)), (((&e->v1_disk_link)[v == e->v2]).prev))

/**
 * size to use for stack arrays when dealing with NGons,
 * alloc after this limit is reached.
 * this value is rather arbitrary
 */
#define RM_DEFAULT_NGON_STACK_SIZE 32
/**
 * size to use for stack arrays dealing with connected mesh data
 * verts of faces, edges of vert - etc.
 * often used with #RM_iter_as_arrayN()
 */
#define RM_DEFAULT_ITER_STACK_SIZE 16

/* avoid inf loop, this value is arbitrary but should not error on valid cases! */
#define RM_LOOP_RADIAL_MAX 10000
#define RM_NGON_MAX 100000

typedef struct RMFace_OFlag {
	RMFace base;
	
	struct RMFlagLayer *oflags;
} RMFace_OFlag;

typedef struct RMesh {
	int totvert, totedge, totloop, totface;

	/**
	 * Flag index arrays as being dirty so we can check if they are clean and avoid looping over the
	 * entire vert/edge/face/loop array in those cases. valid flags are: `(RM_VERT | RM_EDGE |
	 * RM_FACE | RM_LOOP)`.
	 */
	char elem_index_dirty;

	/**
	 * Flag array table as being dirty so we know when its safe to use it,
	 * or when it needs to be re-created. valid flags are: `(RM_VERT | RM_EDGE |
	 * RM_FACE | RM_LOOP)`.
	 */
	char elem_table_dirty;

	/** Pools that hold the data of the mesh elements. */
	struct MemPool *vpool, *epool, *lpool, *fpool;

	/**
	 * MemPool lookup tables (optional) index tables, to map indices to elements via
	 * #RM_mesh_elem_table_ensure and associated functions. Don't touch this or read it directly.
	 * Use #RM_mesh_elem_table_ensure(), #RM_vert/edge/face_at_index().
	 */
	RMVert **vtable;
	RMEdge **etable;
	RMFace **ftable;

	/* size of allocated tables */
	int vtable_tot;
	int etable_tot;
	int ftable_tot;

	CustomData vdata, edata, ldata, pdata;
} RMesh;

/* args for _Generic */
#define _RM_GENERIC_TYPE_ELEM_NONCONST void *, RMVert *, RMEdge *, RMLoop *, RMFace *, RMVert_OFlag *, RMEdge_OFlag *, RMFace_OFlag *, RMElem *, RMElemF *, RMHeader *
#define _RM_GENERIC_TYPE_ELEM_CONST const void *, const RMVert *, const RMEdge *, const RMLoop *, const RMFace *, const RMVert_OFlag *, const RMEdge_OFlag *, const RMFace_OFlag *, const RMElem *, const RMElemF *, const RMHeader *

#define RM_CHECK_TYPE_ELEM_CONST(ele) CHECK_TYPE_ANY(ele, _RM_GENERIC_TYPES_CONST)
#define RM_CHECK_TYPE_ELEM_NONCONST(ele) CHECK_TYPE_ANY(ele, _RM_GENERIC_TYPE_ELEM_NONCONST)

#define RM_CHECK_TYPE_ELEM(ele) CHECK_TYPE_ANY(ele, _RM_GENERIC_TYPE_ELEM_NONCONST, _RM_GENERIC_TYPE_ELEM_CONST)

/* vert */
#define _RM_GENERIC_TYPE_VERT_NONCONST RMVert *, RMVert_OFlag *
#define _RM_GENERIC_TYPE_VERT_CONST const RMVert *, const RMVert_OFlag *
#define RM_CHECK_TYPE_VERT_CONST(ele) CHECK_TYPE_ANY(ele, _RM_GENERIC_TYPE_VERT_CONST)
#define RM_CHECK_TYPE_VERT_NONCONST(ele) CHECK_TYPE_ANY(ele, _RM_GENERIC_TYPE_ELEM_NONCONST)
#define RM_CHECK_TYPE_VERT(ele) CHECK_TYPE_ANY(ele, _RM_GENERIC_TYPE_VERT_NONCONST, _RM_GENERIC_TYPE_VERT_CONST)
/* edge */
#define _RM_GENERIC_TYPE_EDGE_NONCONST RMEdge *, RMEdge_OFlag *
#define _RM_GENERIC_TYPE_EDGE_CONST const RMEdge *, const RMEdge_OFlag *
#define RM_CHECK_TYPE_EDGE_CONST(ele) CHECK_TYPE_ANY(ele, _RM_GENERIC_TYPE_EDGE_CONST)
#define RM_CHECK_TYPE_EDGE_NONCONST(ele) CHECK_TYPE_ANY(ele, _RM_GENERIC_TYPE_ELEM_NONCONST)
#define RM_CHECK_TYPE_EDGE(ele) CHECK_TYPE_ANY(ele, _RM_GENERIC_TYPE_EDGE_NONCONST, _RM_GENERIC_TYPE_EDGE_CONST)
/* face */
#define _RM_GENERIC_TYPE_FACE_NONCONST RMFace *, RMFace_OFlag *
#define _RM_GENERIC_TYPE_FACE_CONST const RMFace *, const RMFace_OFlag *
#define RM_CHECK_TYPE_FACE_CONST(ele) CHECK_TYPE_ANY(ele, _RM_GENERIC_TYPE_FACE_CONST)
#define RM_CHECK_TYPE_FACE_NONCONST(ele) CHECK_TYPE_ANY(ele, _RM_GENERIC_TYPE_ELEM_NONCONST)
#define RM_CHECK_TYPE_FACE(ele) CHECK_TYPE_ANY(ele, _RM_GENERIC_TYPE_FACE_NONCONST, _RM_GENERIC_TYPE_FACE_CONST)

/* Assignment from a void* to a typed pointer is not allowed in C++,
 * casting the LHS to void works fine though.
 */
#ifdef __cplusplus
#	define RM_CHECK_TYPE_ELEM_ASSIGN(ele) (RM_CHECK_TYPE_ELEM(ele)), *((void **)&ele)
#else
#	define RM_CHECK_TYPE_ELEM_ASSIGN(ele) (RM_CHECK_TYPE_ELEM(ele)), ele
#endif


/** \} */

/* -------------------------------------------------------------------- */
/** \name Defines
 * \{ */

#define RM_ELEM_CD_SET_INT(ele, offset, f) \
	{ \
		ROSE_assert(offset != -1); \
		*((int *)((char *)(ele)->head.data + (offset))) = (f); \
	} \
	(void)0

#define RM_ELEM_CD_GET_INT(ele, offset) (ROSE_assert(offset != -1), *((int *)((char *)(ele)->head.data + (offset))))

#define RM_ELEM_CD_SET_BOOL(ele, offset, f) \
	{ \
		ROSE_assert(offset != -1); \
		*((bool *)((char *)(ele)->head.data + (offset))) = (f); \
	} \
	(void)0

#define RM_ELEM_CD_GET_BOOL(ele, offset) (ROSE_assert(offset != -1), *((bool *)((char *)(ele)->head.data + (offset))))
#define RM_ELEM_CD_GET_BOOL_P(ele, offset) (ROSE_assert(offset != -1), (bool *)((char *)(ele)->head.data + (offset)))
#define RM_ELEM_CD_GET_VOID_P(ele, offset) (ROSE_assert(offset != -1), (void *)((char *)(ele)->head.data + (offset)))

#define RM_ELEM_CD_SET_FLOAT(ele, offset, f) \
	{ \
		CHECK_TYPE_NONCONST(ele); \
		ROSE_assert(offset != -1); \
		*((float *)((char *)(ele)->head.data + (offset))) = (f); \
	} \
	(void)0

#define RM_ELEM_CD_GET_FLOAT(ele, offset) (ROSE_assert(offset != -1), *((float *)((char *)(ele)->head.data + (offset))))

#define RM_ELEM_CD_GET_FLOAT_P(ele, offset) (ROSE_assert(offset != -1), (float *)((char *)(ele)->head.data + (offset)))
#define RM_ELEM_CD_GET_FLOAT2_P(ele, offset) (ROSE_assert(offset != -1), (float(*)[2])((char *)(ele)->head.data + (offset)))
#define RM_ELEM_CD_GET_FLOAT3_P(ele, offset) (ROSE_assert(offset != -1), (float(*)[3])((char *)(ele)->head.data + (offset)))

#define RM_ELEM_CD_SET_FLOAT2(ele, offset, f) \
	{ \
		ROSE_assert(offset != -1); \
		((float *)((char *)(ele)->head.data + (offset)))[0] = (f)[0]; \
		((float *)((char *)(ele)->head.data + (offset)))[1] = (f)[1]; \
	} \
	(void)0

#define RM_ELEM_CD_SET_FLOAT3(ele, offset, f) \
	{ \
		ROSE_assert(offset != -1); \
		((float *)((char *)(ele)->head.data + (offset)))[0] = (f)[0]; \
		((float *)((char *)(ele)->head.data + (offset)))[1] = (f)[1]; \
		((float *)((char *)(ele)->head.data + (offset)))[2] = (f)[2]; \
	} \
	(void)0

#define RM_ELEM_CD_GET_FLOAT_AS_UCHAR(ele, offset) (ROSE_assert(offset != -1), (uchar)(RM_ELEM_CD_GET_FLOAT(ele, offset) * 255.0f))

/** \} */

#ifdef __cplusplus
}
#endif

#endif // RM_TYPE_H
