#include "bmesh_iterators.h"
#include "bmesh_inline.h"
#include "bmesh_structure.h"

#include "lib/lib_bitmap.h"

const byte bm_iter_itype_htype_map [ BM_ITYPE_MAX ] = {
	'\0',
	BM_VERT, /* BM_VERTS_OF_MESH */
	BM_EDGE, /* BM_EDGES_OF_MESH */
	BM_FACE, /* BM_FACES_OF_MESH */
	BM_EDGE, /* BM_EDGES_OF_VERT */
	BM_FACE, /* BM_FACES_OF_VERT */
	BM_LOOP, /* BM_LOOPS_OF_VERT */
	BM_VERT, /* BM_VERTS_OF_EDGE */
	BM_FACE, /* BM_FACES_OF_EDGE */
	BM_VERT, /* BM_VERTS_OF_FACE */
	BM_EDGE, /* BM_EDGES_OF_FACE */
	BM_LOOP, /* BM_LOOPS_OF_FACE */
	BM_LOOP, /* BM_LOOPS_OF_LOOP */
	BM_LOOP, /* BM_LOOPS_OF_EDGE */
};

int BM_iter_mesh_count ( byte itype , BMesh *bm ) {
	int count = 0;
	
	switch ( itype ) {
		case BM_VERTS_OF_MESH: {
			count = bm->TotVert;
		}break;
		case BM_EDGES_OF_MESH: {
			count = bm->TotEdge;
		}break;
		case BM_FACES_OF_MESH: {
			count = bm->TotFace;
		}break;
		default: {
			LIB_assert_unreachable ( );
		}break;
	}

	return count;
}

void *BM_iter_at_index ( BMesh *bm , byte itype , void *data , int index ) {
	BMIter iter;

	void *val;

	if ( index < 0 ) {
		LIB_assert_unreachable ( );
		return NULL;
	}

	val = BM_iter_new ( &iter , bm , itype , data );

	for ( int i = 0; i < index; i++ ) {
		val = BM_iter_step ( &iter );
	}

	return val;
}

int BM_iter_as_array ( BMesh *bm , byte itype , void *data , void **array , const int len ) {
	int i = 0;

	/* sanity check */
	if ( len > 0 ) {
		BMIter iter;

		for ( void *ele = BM_iter_new ( &iter , bm , itype , data ); ele; ele = BM_iter_step ( &iter ) ) {
			array [ i ] = ele;
			i++;
			if ( i == len ) {
				return len;
			}
		}
	}

	return i;
}

void *BM_iter_as_arrayN ( BMesh *bm ,
			  byte itype ,
			  void *data ,
			  int *r_len ,
			  /* optional args to avoid an alloc (normally stack array) */
			  void **stack_array ,
			  int stack_array_size ) {
	BMIter iter;

	LIB_assert ( stack_array_size == 0 || ( stack_array_size && stack_array ) );

	/* We can't rely on #BMIter.count being set. */
	switch ( itype ) {
		case BM_VERTS_OF_MESH: {
			iter.Count = bm->TotVert;
		}break;
		case BM_EDGES_OF_MESH: {
			iter.Count = bm->TotEdge;
		}break;
		case BM_FACES_OF_MESH: {
			iter.Count = bm->TotFace;
		}break;
	}

	if ( BM_iter_init ( &iter , bm , itype , data ) && iter.Count > 0 ) {
		BMElem *ele;
		BMElem **array = iter.Count > stack_array_size ?
			( BMElem ** ) MEM_mallocN ( sizeof ( ele ) * iter.Count , __func__ ) :
			( BMElem ** ) stack_array;
		int i = 0;

		*r_len = iter.Count; /* set before iterating */

		while ( ( ele = ( BMElem * ) BM_iter_step ( &iter ) ) ) {
			array [ i++ ] = ele;
		}
		return array;
	}

	*r_len = 0;
	return NULL;
}

int BM_iter_mesh_bitmap_from_filter ( byte itype ,
				      BMesh *bm ,
				      LIB_bitmap *bitmap ,
				      bool ( *test_fn )( BMElem * , void *user_data ) ,
				      void *user_data ) {
	BMIter iter;
	BMElem *ele;
	int i;
	int bitmap_enabled = 0;

	BM_ITER_MESH_INDEX ( ele , &iter , bm , itype , i ) {
		if ( test_fn ( ele , user_data ) ) {
			LIB_BITMAP_ENABLE ( bitmap , i );
			bitmap_enabled++;
		} else {
			LIB_BITMAP_DISABLE ( bitmap , i );
		}
	}

	return bitmap_enabled;
}

int BM_iter_mesh_bitmap_from_filter_tessface ( BMesh *bm ,
					       LIB_bitmap *bitmap ,
					       bool ( *test_fn )( BMFace * , void *user_data ) ,
					       void *user_data ) {
	BMIter iter;
	BMFace *f;
	int i;
	int j = 0;
	int bitmap_enabled = 0;

	BM_ITER_MESH_INDEX ( f , &iter , bm , BM_FACES_OF_MESH , i ) {
		if ( test_fn ( f , user_data ) ) {
			for ( int tri = 2; tri < f->Len; tri++ ) {
				LIB_BITMAP_ENABLE ( bitmap , j );
				bitmap_enabled++;
				j++;
			}
		} else {
			for ( int tri = 2; tri < f->Len; tri++ ) {
				LIB_BITMAP_ENABLE ( bitmap , j );
				j++;
			}
		}
	}

	return bitmap_enabled;
}

int BM_iter_elem_count_flag ( byte itype , void *data , const char hflag , const bool value ) {
	BMIter iter;
	BMElem *ele;
	int count = 0;

	BM_ITER_ELEM ( ele , &iter , data , itype ) {
		if ( BM_elem_flag_test_bool ( ele , hflag ) == value ) {
			count++;
		}
	}

	return count;
}

int BM_iter_mesh_count_flag ( byte itype , BMesh *bm , const char hflag , const bool value ) {
	BMIter iter;
	BMElem *ele;
	int count = 0;

	BM_ITER_MESH ( ele , &iter , bm , itype ) {
		if ( BM_elem_flag_test_bool ( ele , hflag ) == value ) {
			count++;
		}
	}

	return count;
}

/* -------------------------------------------------------------------- */
/** \name Vert of mesh callbacks.
* \{ */

void bmiter__elem_of_mesh_begin ( struct BMIter__elem_of_mesh *iter ) {
	LIB_mempool_iternew ( iter->pooliter.Pool , &iter->pooliter );
}

void *bmiter__elem_of_mesh_step ( struct BMIter__elem_of_mesh *iter ) {
	return LIB_mempool_iterstep ( &iter->pooliter );
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Edge of vert callbacks.
* \{ */

void bmiter__edge_of_vert_begin ( struct BMIter__edge_of_vert *iter ) {
	if ( iter->VertData->Edge ) {
		iter->EdgeFirst = iter->VertData->Edge;
		iter->EdgeNext = iter->VertData->Edge;
	} else {
		iter->EdgeFirst = NULL;
		iter->EdgeNext = NULL;
	}
}

void *bmiter__edge_of_vert_step ( struct BMIter__edge_of_vert *iter ) {
	BMEdge *e_curr = iter->EdgeNext;

	if ( iter->EdgeNext ) {
		LIB_assert ( ELEM ( iter->VertData , iter->EdgeNext->Vert1 , iter->EdgeNext->Vert2 ) );
		iter->EdgeNext = bmesh_disk_edge_next ( iter->EdgeNext , iter->VertData );
		if ( iter->EdgeNext == iter->EdgeFirst ) {
			iter->EdgeNext = NULL;
		}
	}

	return e_curr;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Face of vert callbacks.
* \{ */

void bmiter__face_of_vert_begin ( struct BMIter__face_of_vert *iter ) {
	( ( BMIter * ) iter )->Count = bmesh_disk_facevert_count ( iter->VertData );
	if ( ( ( BMIter * ) iter )->Count ) {
		iter->LoopFirst = bmesh_disk_faceloop_find_first ( iter->VertData->Edge , iter->VertData );
		iter->EdgeFirst = iter->LoopFirst->Edge;
		iter->EdgeNext = iter->EdgeFirst;
		iter->LoopNext = iter->LoopFirst;
	} else {
		iter->LoopFirst = iter->LoopNext = NULL;
		iter->EdgeFirst = iter->EdgeNext = NULL;
	}
}

void *bmiter__face_of_vert_step ( struct BMIter__face_of_vert *iter ) {
	BMLoop *l_curr = iter->LoopNext;

	if ( ( ( BMIter * ) iter )->Count && iter->LoopNext ) {
		( ( BMIter * ) iter )->Count--;
		iter->LoopNext = bmesh_radial_faceloop_find_next ( iter->LoopNext , iter->VertData );
		if ( iter->LoopNext == iter->LoopFirst ) {
			iter->EdgeNext = bmesh_disk_faceedge_find_next ( iter->EdgeNext , iter->VertData );
			iter->LoopFirst = bmesh_radial_faceloop_find_first ( iter->EdgeNext->Loop , iter->VertData );
			iter->LoopNext = iter->LoopFirst;
		}
	}

	if ( !( ( BMIter * ) iter )->Count ) {
		iter->LoopNext = NULL;
	}

	return l_curr ? l_curr->Face : NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Loop of vert callbacks.
* \{ */

void bmiter__loop_of_vert_begin ( struct BMIter__loop_of_vert *iter ) {
	( ( BMIter * ) iter )->Count = bmesh_disk_facevert_count ( iter->VertData );
	if ( ( ( BMIter * ) iter )->Count ) {
		iter->LoopFirst = bmesh_disk_faceloop_find_first ( iter->VertData->Edge , iter->VertData );
		iter->EdgeFirst = iter->LoopFirst->Edge;
		iter->EdgeNext = iter->EdgeFirst;
		iter->LoopNext = iter->LoopFirst;
	} else {
		iter->LoopFirst = iter->LoopNext = NULL;
		iter->EdgeFirst = iter->EdgeNext = NULL;
	}
}
void *bmiter__loop_of_vert_step ( struct BMIter__loop_of_vert *iter ) {
	BMLoop *l_curr = iter->LoopNext;

	if ( ( ( BMIter * ) iter )->Count ) {
		( ( BMIter * ) iter )->Count--;
		iter->LoopNext = bmesh_radial_faceloop_find_next ( iter->LoopNext , iter->VertData );
		if ( iter->LoopNext == iter->LoopFirst ) {
			iter->EdgeNext = bmesh_disk_faceedge_find_next ( iter->EdgeNext , iter->VertData );
			iter->LoopFirst = bmesh_radial_faceloop_find_first ( iter->EdgeNext->Loop , iter->VertData );
			iter->LoopNext = iter->LoopFirst;
		}
	}

	if ( !( ( BMIter * ) iter )->Count ) {
		iter->LoopNext = NULL;
	}

	/* NULL on finish */
	return l_curr;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Loop of edge callbacks.
* \{ */

void bmiter__loop_of_edge_begin ( struct BMIter__loop_of_edge *iter ) {
	iter->LoopFirst = iter->LoopNext = iter->EdgeData->Loop;
}

void *bmiter__loop_of_edge_step ( struct BMIter__loop_of_edge *iter ) {
	BMLoop *l_curr = iter->LoopNext;

	if ( iter->LoopNext ) {
		iter->LoopNext = iter->LoopNext->RadialNext;
		if ( iter->LoopNext == iter->LoopFirst ) {
			iter->LoopNext = NULL;
		}
	}

	/* NULL on finish */
	return l_curr;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Loop of loop callbacks.
* \{ */

void bmiter__loop_of_loop_begin ( struct BMIter__loop_of_loop *iter ) {
	iter->LoopFirst = iter->LoopData;
	iter->LoopNext = iter->LoopFirst->RadialNext;

	if ( iter->LoopNext == iter->LoopFirst ) {
		iter->LoopNext = NULL;
	}
}

void *bmiter__loop_of_loop_step ( struct BMIter__loop_of_loop *iter ) {
	BMLoop *l_curr = iter->LoopNext;

	if ( iter->LoopNext ) {
		iter->LoopNext = iter->LoopNext->RadialNext;
		if ( iter->LoopNext == iter->LoopFirst ) {
			iter->LoopNext = NULL;
		}
	}

	/* NULL on finish */
	return l_curr;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Face of edge callbacks.
* \{ */

void bmiter__face_of_edge_begin ( struct BMIter__face_of_edge *iter ) {
	iter->LoopFirst = iter->LoopNext = iter->EdgeData->Loop;
}

void *bmiter__face_of_edge_step ( struct BMIter__face_of_edge *iter ) {
	BMLoop *current = iter->LoopNext;

	if ( iter->LoopNext ) {
		iter->LoopNext = iter->LoopNext->RadialNext;
		if ( iter->LoopNext == iter->LoopFirst ) {
			iter->LoopNext = NULL;
		}
	}

	return current ? current->Face : NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Verts of edge callbacks.
* \{ */

void bmiter__vert_of_edge_begin ( struct BMIter__vert_of_edge *iter ) {
	( ( BMIter * ) iter )->Count = 0;
}

void *bmiter__vert_of_edge_step ( struct BMIter__vert_of_edge *iter ) {
	switch ( ( ( BMIter * ) iter )->Count++ ) {
		case 0: return iter->EdgeData->Vert1;
		case 1: return iter->EdgeData->Vert2;
	}
	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Verts of face callbacks.
* \{ */

void bmiter__vert_of_face_begin ( struct BMIter__vert_of_face *iter ) {
	iter->LoopFirst = iter->LoopNext = BM_FACE_FIRST_LOOP ( iter->FaceData );
}

void *bmiter__vert_of_face_step ( struct BMIter__vert_of_face *iter ) {
	BMLoop *l_curr = iter->LoopNext;

	if ( iter->LoopNext ) {
		iter->LoopNext = iter->LoopNext->Next;
		if ( iter->LoopNext == iter->LoopFirst ) {
			iter->LoopNext = NULL;
		}
	}

	return l_curr ? l_curr->Vert : NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Edge of face callbacks.
* \{ */

void bmiter__edge_of_face_begin ( struct BMIter__edge_of_face *iter ) {
	iter->LoopFirst = iter->LoopNext = BM_FACE_FIRST_LOOP ( iter->FaceData );
}

void *bmiter__edge_of_face_step ( struct BMIter__edge_of_face *iter ) {
	BMLoop *l_curr = iter->LoopNext;

	if ( iter->LoopNext ) {
		iter->LoopNext = iter->LoopNext->Next;
		if ( iter->LoopNext == iter->LoopFirst ) {
			iter->LoopNext = NULL;
		}
	}

	return l_curr ? l_curr->Edge : NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Loop of face callbacks.
* \{ */

void bmiter__loop_of_face_begin ( struct BMIter__loop_of_face *iter ) {
	iter->LoopFirst = iter->LoopNext = BM_FACE_FIRST_LOOP ( iter->FaceData );
}

void *bmiter__loop_of_face_step ( struct BMIter__loop_of_face *iter ) {
	BMLoop *l_curr = iter->LoopNext;

	if ( iter->LoopNext ) {
		iter->LoopNext = iter->LoopNext->Next;
		if ( iter->LoopNext == iter->LoopFirst ) {
			iter->LoopNext = NULL;
		}
	}

	return l_curr;
}

/** \} */
