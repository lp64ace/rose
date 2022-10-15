#pragma once

#include "mesh/bmesh_class.h"

#include "lib/lib_compiler.h"

BMFace *BM_face_copy ( BMesh *bm_dst , BMesh *bm_src , BMFace *f , bool copy_verts , bool copy_edges );

#define BM_CREATE_NOP			0
// Faces & Edges only.
#define BM_CREATE_NO_DOUBLE		(1<<1)
/** Skip custom-data - for all element types data, 
* use if we immediately write custom-data into the element so this skips copying from 'example' 
* arguments or setting defaults, speeds up conversion when data is converted all at once.
*/
#define BM_CREATE_SKIP_CD		(1<<2)

// Main funtion for creating a new vertex
BMVert *BM_vert_create ( BMesh *bm , const float co [ 3 ] , const BMVert *v_example , byte create_flag );

/** Main function for creating a new edge.
* \note Duplicate edges are supported by the API however users should _never_ see them. 
* so unless you need a unique edge or know the edge won't exist, you should call with \a BM_CREATE_NO_DOUBLE. */
BMEdge *BM_edge_create ( BMesh *bm , BMVert *v1 , BMVert *v2 , const BMEdge *e_example , byte create_flag );

/** Main face creation function.
* 
* \param bm The mesh.
* \param verts A sorted array of verts size of len.
* \param edges A sorted array of edges size of len.
* \param len The length of the face.
* \param create_flag Options for creating the face. */
BMFace *BM_face_create ( BMesh *bm , BMVert **verts , BMEdge **edges , int len , const BMFace *f_example , byte create_flag );

// Wrapper for #BM_face_create when you don't have an edge array.
BMFace *BM_face_create_verts ( BMesh *bm , 
			       BMVert **vert_arr , int len , 
			       const BMFace *f_example , 
			       byte create_flag , bool create_edges );

// Kills all edges associated with \a f, along with any other faces containing those edges.
void BM_face_edges_kill ( BMesh *bm , BMFace *f );

// Kills all verts associated with \a f, along with any other faces containing those vertices
void BM_face_verts_kill ( BMesh *bm , BMFace *f );

// Kills \a f and its loops.
void BM_face_kill_loose ( BMesh *bm , BMFace *f );

// Kills \a e and all faces that use it.
void BM_edge_kill ( BMesh *bm , BMEdge *e );

// Kills \a v and all edges that use it.
void BM_vert_kill ( BMesh *bm , BMVert *v );
