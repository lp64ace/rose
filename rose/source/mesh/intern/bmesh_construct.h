#pragma once

#include "bmesh_core.h"

/**
* Fill in a vertex array from an edge array.
*
* \returns false if any verts aren't found.
*/
bool BM_verts_from_edges ( BMVert **vert_arr , BMEdge **edge_arr , int len );

/**
* Fill in an edge array from a vertex array (connected polygon loop).
*
* \returns false if any edges aren't found.
*/
bool BM_edges_from_verts ( BMEdge **edge_arr , BMVert **vert_arr , int len );

/**
* Fill in an edge array from a vertex array (connected polygon loop).
* Creating edges as-needed.
*/
void BM_edges_from_verts_ensure ( BMesh *bm , BMEdge **edge_arr , BMVert **vert_arr , int len );

/**
* Copies attributes, e.g. customdata, header flags, etc, from one element
* to another of the same type.
*/
void BM_elem_attrs_copy_ex ( BMesh *bm_src , BMesh *bm_dst , const void *ele_src_v , void *ele_dst_v , char hflag_mask , uint64_t cd_mask_exclude );
void BM_elem_attrs_copy ( BMesh *bm_src , BMesh *bm_dst , const void *ele_src_v , void *ele_dst_v );
void BM_elem_select_copy ( BMesh *bm_dst , void *ele_dst_v , const void *ele_src_v );
