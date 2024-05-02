#pragma once

#include "LIB_assert.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RMHead {
	/** All mesh elements can own customdata. */
	void *data;
	
	/** A cached index for this element in the element array, check for dirty flags before using. */
	int index;
	
	/** The type of this element, see below. */
	short type;
	/** Flags for this element, see below. */
	short flag;
} RMHead;

/** We don't want to blow this structure too much keep it under 16bytes. */
ROSE_STATIC_ASSERT(sizeof(RMHead) <= 16, "RMHead size has increased!");

/** #RMHead->type */
enum {
	/**
	 * Stored as a short anyway so we have 2bytes to waste here! 
	 * Might as well make it a bitflag so that we can mask them later on without using different enums.
	 */
	RM_VERT = (1 << 0),
	RM_EDGE = (1 << 1),
	RM_LOOP = (1 << 2),
	RM_FACE = (1 << 3),
};

/** #RMHead->flag */
enum {
	RM_ELEM_SELECT = (1 << 0),
	RM_ELEM_HIDDEN = (1 << 1),
	
	/** Runtime tags, reset before use. */
	RM_ELEM_TAG0 = (1 << 2),
	RM_ELEM_TAG1 = (1 << 2),
};

typedef struct RMVert {
	RMHead head;
	
	float co[3];
	/** Vertex normal. */
	float no[3];
	
	/** Pointer to any edge using this vertex. */
	struct RMEdge *e;
} RMVert;

typedef struct RMDiskLink {
	struct RMEdge *prev, *next;
} RMDiskLink;

typedef struct RMEdge {
	RMHead head;
	
	/** 
	 * Vertices unordered, althrough the order may matter in some cases,
	 * nothing should be assumed and the order of the vertices should not 
	 * change unless there is a good reason to do so.
	 */
	struct RMVert *v1, *v2;
	
	/** The loop this edge is part of. */
	struct RMLoop *loop;
	
	RMDiskLink v1_disk_link, v2_disk_link;
} RMEdge;

typedef struct RMLoop {
	RMHead head;
	
	struct RMVert *v;
	struct RMEdge *e;
	
	/**
	 * The face this loop is part of, this should be common for all the RMLoop elements in this circle.
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
   * - Every loop in this radial list has the same value for #RMLoop.e.
   *
   * - The value for #RMLoop.v might not match the radial next/previous
   *   as this depends on the face-winding.
   *   You can be sure #RMLoop.v will either #RMEdge.v1 or #RMEdge.v2 of #RMLoop.e,
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
  struct RMLoop *radial_next, *radial_prev;

  /**
   * Other loops that are part of this face.
   *
   * This is typically used for accessing all vertices/edges in a faces.
   *
   * - This is a circular list, so there are no first/last storage of the "cycle" data.
   *   Instead #RMFace.l_first points to any one of the loops that are part of this face.
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
   * l_iter = l_first = RM_FACE_FIRST_LOOP(f);
   * do {
   *   operate_on_vert(l_iter->v);
   *   operate_on_edge(l_iter->e);
   * } while ((l_iter = l_iter->next) != l_first);
   * \endcode
   */
	struct RMLoop *prev, *next;
} RMLoop;

typedef struct RMFace {
	RMHead head;
	
	/** Any of the loop in this face, we can iterate this list to get all the vertices in this face. */
	struct RMLoop *l_first;
	
	/** The length of the face, the number of vertices of this face. */
	int len;
	/** The normal of this face, see how normals are calculated. */
	float no[3];
	
	/**
	 * The material index for this face, typically ranged from 0 to the number of materials used by a mesh.
	 * This is not enforced though since some tools build the faces first and then the materials for a mesh.
	 * The value of this should always be checked before accessed, in case this is out of range this should 
	 * be treated as -1 or 0 (depending on whether we need a material in order to render this or not).
	 */
	int matnr;
} RMFace;

#define RM_FACE_FIRST_LOOP(f) (((RMFace *)f)->l_first)

typedef struct RMesh {
	/** Number of elements in this mesh. */
	int totvert;
	int totedge;
	int totloop;
	int totface;
	
	/** Number of selected elements in this mesh. */
	int totvertsel;
	int totedgesel;
	int totfacesel;
	
	/** Indicates that the indices should be recalculated. */
	short elem_index_dirty;
	/** Indicates whether the element tables can be used or not. */
	short elem_table_dirty;
	
	struct LIB_mempool *vpool, *epool, *lpool, *fpool;
	
	/**
	 * Element tables (optional), they map elements from #vpool, #epool, #fpool into tables 
	 * for easier element lookup, in order to find elements fast by their index.
	 *
	 * check #elem_table_dirty before accessing these.
	 */
	struct RMVert **vtable;
	struct RMEdge **etable;
	struct RMLoop **ltable;
} RMesh;

#ifdef __cplusplus
}
#endif