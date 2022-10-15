#pragma once

#include "lib/lib_utildefines.h"
#include "lib/lib_typedef.h"
#include "lib/lib_compiler.h"

#include "bmesh_private.h"

/**
* \brief Iterator Step
*
* Calls an iterators step function to return the next element.
*/
ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 ) ROSE_INLINE
void *BM_iter_step ( BMIter *iter ) {
	return iter->Step ( iter );
}

/**
* \brief Iterator Init
*
* Takes a bmesh iterator structure and fills 
* it with the appropriate function pointers based 
* upon its type.
*/
ATTR_NONNULL ( 1 ) ROSE_INLINE
bool BM_iter_init ( BMIter *iter , BMesh *bm , const char itype , void *data ) {
	iter->IterType = itype;

	// Inlining optimizes out this switch when called with the defined type.
	switch ( ( BMIterType ) itype ) {
		case BM_VERTS_OF_MESH: {
			LIB_assert ( bm != NULL );
			LIB_assert ( data == NULL );
			iter->Begin = ( BMIter__begin_cb ) bmiter__elem_of_mesh_begin;
			iter->Step = ( BMIter__step_cb ) bmiter__elem_of_mesh_step;
			iter->Data.elem_of_mesh.pooliter.Pool = bm->VertPool;
		}break;
		case BM_EDGES_OF_MESH: {
			LIB_assert ( bm != NULL );
			LIB_assert ( data == NULL );
			iter->Begin = ( BMIter__begin_cb ) bmiter__elem_of_mesh_begin;
			iter->Step = ( BMIter__step_cb ) bmiter__elem_of_mesh_step;
			iter->Data.elem_of_mesh.pooliter.Pool = bm->EdgePool;
		}break;
		case BM_FACES_OF_MESH: {
			LIB_assert ( bm != NULL );
			LIB_assert ( data == NULL );
			iter->Begin = ( BMIter__begin_cb ) bmiter__elem_of_mesh_begin;
			iter->Step = ( BMIter__step_cb ) bmiter__elem_of_mesh_step;
			iter->Data.elem_of_mesh.pooliter.Pool = bm->FacePool;
		}break;
		case BM_EDGES_OF_VERT: {
			LIB_assert ( data != NULL );
			LIB_assert ( ( ( BMElem * ) data )->Head.ElemType == BM_VERT );
			iter->Begin = ( BMIter__begin_cb ) bmiter__edge_of_vert_begin;
			iter->Step = ( BMIter__step_cb ) bmiter__edge_of_vert_step;
			iter->Data.edge_of_vert.VertData = ( BMVert * ) data;
		}break;
		case BM_FACES_OF_VERT: {
			LIB_assert ( data != NULL );
			LIB_assert ( ( ( BMElem * ) data )->Head.ElemType == BM_VERT );
			iter->Begin = ( BMIter__begin_cb ) bmiter__face_of_vert_begin;
			iter->Step = ( BMIter__step_cb ) bmiter__face_of_vert_step;
			iter->Data.face_of_vert.VertData = ( BMVert * ) data;
		}break;
		case BM_LOOPS_OF_VERT: {
			LIB_assert ( data != NULL );
			LIB_assert ( ( ( BMElem * ) data )->Head.ElemType == BM_VERT );
			iter->Begin = ( BMIter__begin_cb ) bmiter__loop_of_vert_begin;
			iter->Step = ( BMIter__step_cb ) bmiter__loop_of_vert_step;
			iter->Data.loop_of_vert.VertData = ( BMVert * ) data;
		}break;
		case BM_VERTS_OF_EDGE: {
			LIB_assert ( data != NULL );
			LIB_assert ( ( ( BMElem * ) data )->Head.ElemType == BM_EDGE );
			iter->Begin = ( BMIter__begin_cb ) bmiter__vert_of_edge_begin;
			iter->Step = ( BMIter__step_cb ) bmiter__vert_of_edge_step;
			iter->Data.vert_of_edge.EdgeData = ( BMEdge * ) data;
		}break;
		case BM_FACES_OF_EDGE: {
			LIB_assert ( data != NULL );
			LIB_assert ( ( ( BMElem * ) data )->Head.ElemType == BM_EDGE );
			iter->Begin = ( BMIter__begin_cb ) bmiter__face_of_edge_begin;
			iter->Step = ( BMIter__step_cb ) bmiter__face_of_edge_step;
			iter->Data.face_of_edge.EdgeData = ( BMEdge * ) data;
		}break;
		case BM_VERTS_OF_FACE: {
			LIB_assert ( data != NULL );
			LIB_assert ( ( ( BMElem * ) data )->Head.ElemType == BM_FACE );
			iter->Begin = ( BMIter__begin_cb ) bmiter__vert_of_face_begin;
			iter->Step = ( BMIter__step_cb ) bmiter__vert_of_face_step;
			iter->Data.vert_of_face.FaceData = ( BMFace * ) data;
		}break;
		case BM_EDGES_OF_FACE: {
			LIB_assert ( data != NULL );
			LIB_assert ( ( ( BMElem * ) data )->Head.ElemType == BM_FACE );
			iter->Begin = ( BMIter__begin_cb ) bmiter__edge_of_face_begin;
			iter->Step = ( BMIter__step_cb ) bmiter__edge_of_face_step;
			iter->Data.edge_of_face.FaceData = ( BMFace * ) data;
		}break;
		case BM_LOOPS_OF_FACE: {
			LIB_assert ( data != NULL );
			LIB_assert ( ( ( BMElem * ) data )->Head.ElemType == BM_FACE );
			iter->Begin = ( BMIter__begin_cb ) bmiter__loop_of_face_begin;
			iter->Step = ( BMIter__step_cb ) bmiter__loop_of_face_step;
			iter->Data.loop_of_face.FaceData = ( BMFace * ) data;
		}break;
		case BM_LOOPS_OF_LOOP: {
			LIB_assert ( data != NULL );
			LIB_assert ( ( ( BMElem * ) data )->Head.ElemType == BM_LOOP );
			iter->Begin = ( BMIter__begin_cb ) bmiter__loop_of_loop_begin;
			iter->Step = ( BMIter__step_cb ) bmiter__loop_of_loop_step;
			iter->Data.loop_of_loop.LoopData = ( BMLoop * ) data;
		}break;
		case BM_LOOPS_OF_EDGE: {
			LIB_assert ( data != NULL );
			LIB_assert ( ( ( BMElem * ) data )->Head.ElemType == BM_EDGE );
			iter->Begin = ( BMIter__begin_cb ) bmiter__loop_of_edge_begin;
			iter->Step = ( BMIter__step_cb ) bmiter__loop_of_edge_step;
			iter->Data.loop_of_edge.EdgeData = ( BMEdge * ) data;
		}break;
		default: {
			// Should never happen.
			LIB_assert_unreachable ( );
		}return false;
	}

	iter->Begin ( iter );

	return true;
}

/**
* \brief Iterator New
*
* Takes a bmesh iterator structure and fills 
* it with the appropriate function pointers based 
* upon its type and then calls BMeshIter_step() 
* to return the first element of the iterator.
*/
ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 ) ROSE_INLINE
void *BM_iter_new ( BMIter *iter , BMesh *bm , const byte itype , void *data ) {
	if ( LIKELY ( BM_iter_init ( iter , bm , itype , data ) ) ) {
		return BM_iter_step ( iter );
	}
	return NULL;
}
