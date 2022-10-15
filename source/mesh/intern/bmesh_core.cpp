#include "bmesh_core.h"
#include "bmesh_structure.h"
#include "bmesh_inline.h"
#include "bmesh_query.h"
#include "bmesh_construct.h"

#include "lib/lib_mempool.h"
#include "lib/lib_math.h"

#include "kernel/rke_customdata.h"

BMVert *BM_vert_create ( BMesh *bm ,
			 const float co [ 3 ] ,
			 const BMVert *v_example ,
			 const byte create_flag ) {
	BMVert *v = ( BMVert * ) LIB_mempool_alloc ( bm->VertPool );

	LIB_assert ( ( v_example == NULL ) || ( v_example->Head.ElemType == BM_VERT ) );

	/* --- assign all members --- */
	v->Head.Data = NULL;

	BM_elem_index_set ( v , -1 );

	v->Head.ElemType = BM_VERT;
	v->Head.ElemFlag = 0;
	v->Head.ApiFlag = 0;

	// 'v->Normal' is handled by BM_elem_attrs_copy
	if ( co ) {
		copy_v3_v3 ( v->Coord , co );
	} else {
		zero_v3 ( v->Coord );
	}

	v->Edge = NULL;
	/* --- done --- */

	// Disallow this flag for verts - its meaningless.
	LIB_assert ( ( create_flag & BM_CREATE_NO_DOUBLE ) == 0 );

	/* may add to middle of the pool */
	bm->ElemIndexDirty |= BM_VERT;
	bm->ElemTableDirty |= BM_VERT;

	bm->TotVert++;

	if ( !( create_flag & BM_CREATE_SKIP_CD ) ) {
		if ( v_example ) {
			int *keyi;

			/* handles 'v->Normal' too */
			BM_elem_attrs_copy ( bm , bm , v_example , v );

			/* exception: don't copy the original shapekey index */
			keyi = ( int * ) CustomData_bmesh_get ( &bm->VertData , v->Head.Data , CD_SHAPE_KEYINDEX );
			if ( keyi ) {
				*keyi = ORIGINDEX_NONE;
			}
		} else {
			CustomData_bmesh_set_default ( &bm->VertData , &v->Head.Data );
			zero_v3 ( v->Normal );
		}
	} else {
		if ( v_example ) {
			copy_v3_v3 ( v->Normal , v_example->Normal );
		} else {
			zero_v3 ( v->Normal );
		}
	}

	BM_CHECK_ELEMENT ( v );

	return v;
}

BMEdge *BM_edge_create ( BMesh *bm , BMVert *v1 , BMVert *v2 , const BMEdge *e_example , const byte create_flag ) {
	BMEdge *e;

	LIB_assert ( v1 != v2 );
	LIB_assert ( v1->Head.ElemType == BM_VERT && v2->Head.ElemType == BM_VERT );
	LIB_assert ( ( e_example == NULL ) || ( e_example->Head.ElemType == BM_EDGE ) );

	if ( ( create_flag & BM_CREATE_NO_DOUBLE ) && ( e = BM_edge_exists ( v1 , v2 ) ) ) {
		return e;
	}

	e = ( BMEdge * ) LIB_mempool_alloc ( bm->EdgePool );

	/* --- assign all members --- */
	e->Head.Data = NULL;

	BM_elem_index_set ( e , -1 ); // set_ok_invalid

	e->Head.ElemType = BM_EDGE;
	e->Head.ElemFlag = BM_ELEM_DRAW;
	e->Head.ApiFlag = 0;

	e->Vert1 = v1;
	e->Vert2 = v2;
	e->Loop = NULL;

	memset ( &e->DiskLink1 , 0 , sizeof ( BMDiskLink [ 2 ] ) );
	/* --- done --- */

	bmesh_disk_edge_append ( e , e->Vert1 );
	bmesh_disk_edge_append ( e , e->Vert2 );

	/* may add to middle of the pool */
	bm->ElemIndexDirty |= BM_EDGE;
	bm->ElemTableDirty |= BM_EDGE;

	bm->TotEdge++;

	if ( !( create_flag & BM_CREATE_SKIP_CD ) ) {
		if ( e_example ) {
			BM_elem_attrs_copy ( bm , bm , e_example , e );
		} else {
			CustomData_bmesh_set_default ( &bm->EdgeData , &e->Head.Data );
		}
	}

	BM_CHECK_ELEMENT ( e );

	return e;
}

static BMLoop *bm_loop_create ( BMesh *bm ,
				BMVert *v ,
				BMEdge *e ,
				BMFace *f ,
				const BMLoop *l_example ,
				const byte create_flag ) {
	BMLoop *l = NULL;

	l = ( BMLoop * ) LIB_mempool_alloc ( bm->LoopPool );

	LIB_assert ( ( l_example == NULL ) || ( l_example->Head.ElemType == BM_LOOP ) );

#ifndef NDEBUG
	if ( l_example ) {
		/* ensure passing a loop is either sharing the same vertex, or entirely disconnected
		 * use to catch mistake passing in loop offset-by-one. */
		LIB_assert ( ( v == l_example->Vert ) || !ELEM ( v , l_example->Prev->Vert , l_example->Next->Vert ) );
	}
#endif

	/* --- assign all members --- */
	l->Head.Data = NULL;

	BM_elem_index_set ( l , -1 ); /* set_ok_invalid */

	l->Head.ElemType = BM_LOOP;
	l->Head.ElemFlag = 0;
	l->Head.ApiFlag = 0;

	l->Vert = v;
	l->Edge = e;
	l->Face = f;

	l->RadialNext = NULL;
	l->RadialPrev = NULL;
	l->Next = NULL;
	l->Prev = NULL;
	/* --- done --- */

	/* may add to middle of the pool */
	bm->ElemIndexDirty |= BM_LOOP;

	bm->TotLoop++;

	if ( !( create_flag & BM_CREATE_SKIP_CD ) ) {
		if ( l_example ) {
			/* no need to copy attrs, just handle customdata */
			// BM_elem_attrs_copy(bm, bm, l_example, l);
			CustomData_bmesh_free_block_data ( &bm->LoopData , l->Head.Data );
			CustomData_bmesh_copy_data ( &bm->LoopData , &bm->LoopData , l_example->Head.Data , &l->Head.Data );
		} else {
			CustomData_bmesh_set_default ( &bm->LoopData , &l->Head.Data );
		}
	}

	return l;
}

static BMLoop *bm_face_boundary_add ( BMesh *bm , BMFace *f , BMVert *startv , BMEdge *starte , const byte create_flag ) {
	BMLoop *l = bm_loop_create ( bm , startv , starte , f , NULL , create_flag );

	bmesh_radial_loop_append ( starte , l );

	f->LoopFirst = l;

	return l;
}

BMFace *BM_face_copy ( BMesh *bm_dst , BMesh *bm_src , BMFace *f , const bool copy_verts , const bool copy_edges ) {
	BMVert **verts = ( BMVert ** ) alloca ( sizeof ( *verts ) * f->Len );
	BMEdge **edges = ( BMEdge ** ) alloca ( sizeof ( *edges ) * f->Len );
	BMLoop *l_iter;
	BMLoop *l_first;
	BMLoop *l_copy;
	BMFace *f_copy;
	int i;

	LIB_assert ( ( bm_dst == bm_src ) || ( copy_verts && copy_edges ) );

	l_iter = l_first = BM_FACE_FIRST_LOOP ( f );
	i = 0;
	do {
		if ( copy_verts ) {
			verts [ i ] = BM_vert_create ( bm_dst , l_iter->Vert->Coord , l_iter->Vert , BM_CREATE_NOP );
		} else {
			verts [ i ] = l_iter->Vert;
		}
		i++;
	} while ( ( l_iter = l_iter->Next ) != l_first );

	l_iter = l_first = BM_FACE_FIRST_LOOP ( f );
	i = 0;
	do {
		if ( copy_edges ) {
			BMVert *v1 , *v2;

			if ( l_iter->Edge->Vert1 == verts [ i ] ) {
				v1 = verts [ i ];
				v2 = verts [ ( i + 1 ) % f->Len ];
			} else {
				v2 = verts [ i ];
				v1 = verts [ ( i + 1 ) % f->Len ];
			}

			edges [ i ] = BM_edge_create ( bm_dst , v1 , v2 , l_iter->Edge , BM_CREATE_NOP );
		} else {
			edges [ i ] = l_iter->Edge;
		}
		i++;
	} while ( ( l_iter = l_iter->Next ) != l_first );

	f_copy = BM_face_create ( bm_dst , verts , edges , f->Len , NULL , BM_CREATE_SKIP_CD );

	BM_elem_attrs_copy ( bm_src , bm_dst , f , f_copy );

	l_iter = l_first = BM_FACE_FIRST_LOOP ( f );
	l_copy = BM_FACE_FIRST_LOOP ( f_copy );
	do {
		BM_elem_attrs_copy ( bm_src , bm_dst , l_iter , l_copy );
		l_copy = l_copy->Next;
	} while ( ( l_iter = l_iter->Next ) != l_first );

	return f_copy;
}

ROSE_INLINE BMFace *bm_face_create__internal ( BMesh *bm ) {
	BMFace *f;

	f = ( BMFace * ) LIB_mempool_alloc ( bm->FacePool );

	/* --- assign all members --- */
	f->Head.Data = NULL;
	BM_elem_index_set ( f , -1 ); /* set_ok_invalid */

	f->Head.ElemType = BM_FACE;
	f->Head.ElemFlag = 0;
	f->Head.ApiFlag = 0;

	f->LoopFirst = NULL;
	f->Len = 0;
	/* caller must initialize */
	// zero_v3(f->Normal);
	f->MatNum = 0;
	/* --- done --- */

	/* may add to middle of the pool */
	bm->ElemIndexDirty |= BM_FACE;
	bm->ElemTableDirty |= BM_FACE;

	bm->TotFace++;

	return f;
}

BMFace *BM_face_create ( BMesh *bm ,
			 BMVert **verts ,
			 BMEdge **edges ,
			 const int len ,
			 const BMFace *f_example ,
			 const byte create_flag ) {
	BMFace *f = NULL;
	BMLoop *l , *startl , *lastl;
	int i;

	LIB_assert ( ( f_example == NULL ) || ( f_example->Head.ElemType == BM_FACE ) );
	LIB_assert ( !( create_flag & 1 ) );

	if ( len == 0 ) {
		/* just return NULL for now */
		return NULL;
	}

	if ( create_flag & BM_CREATE_NO_DOUBLE ) {
		/* Check if face already exists */
		f = BM_face_exists ( verts , len );
		if ( f != NULL ) {
			return f;
		}
	}

	f = bm_face_create__internal ( bm );

	startl = lastl = bm_face_boundary_add ( bm , f , verts [ 0 ] , edges [ 0 ] , create_flag );

	for ( i = 1; i < len; i++ ) {
		l = bm_loop_create ( bm , verts [ i ] , edges [ i ] , f , NULL /* edges[i]->l */ , create_flag );

		bmesh_radial_loop_append ( edges [ i ] , l );

		l->Prev = lastl;
		lastl->Next = l;
		lastl = l;
	}

	startl->Prev = lastl;
	lastl->Next = startl;

	f->Len = len;

	if ( !( create_flag & BM_CREATE_SKIP_CD ) ) {
		if ( f_example ) {
			BM_elem_attrs_copy ( bm , bm , f_example , f );
		} else {
			CustomData_bmesh_set_default ( &bm->FaceData , &f->Head.Data );
			zero_v3 ( f->Normal );
		}
	} else {
		if ( f_example ) {
			copy_v3_v3 ( f->Normal , f_example->Normal );
		} else {
			zero_v3 ( f->Normal );
		}
	}

	BM_CHECK_ELEMENT ( f );

	return f;
}

BMFace *BM_face_create_verts ( BMesh *bm ,
			       BMVert **vert_arr ,
			       const int len ,
			       const BMFace *f_example ,
			       const byte create_flag ,
			       const bool create_edges ) {
	BMEdge **edge_arr = ( BMEdge ** ) alloca ( sizeof ( *edge_arr ) * len );

	if ( create_edges ) {
		BM_edges_from_verts_ensure ( bm , edge_arr , vert_arr , len );
	} else {
		if ( BM_edges_from_verts ( edge_arr , vert_arr , len ) == false ) {
			return NULL;
		}
	}

	return BM_face_create ( bm , vert_arr , edge_arr , len , f_example , create_flag );
}

#ifndef NDEBUG

int bmesh_elem_check ( void *element , const char htype ) {
	BMHeader *head = ( BMHeader * ) element;
	enum {
		IS_NULL = ( 1 << 0 ) ,
		IS_WRONG_TYPE = ( 1 << 1 ) ,

		IS_VERT_WRONG_EDGE_TYPE = ( 1 << 2 ) ,

		IS_EDGE_NULL_DISK_LINK = ( 1 << 3 ) ,
		IS_EDGE_WRONG_LOOP_TYPE = ( 1 << 4 ) ,
		IS_EDGE_WRONG_FACE_TYPE = ( 1 << 5 ) ,
		IS_EDGE_NULL_RADIAL_LINK = ( 1 << 6 ) ,
		IS_EDGE_ZERO_FACE_LENGTH = ( 1 << 7 ) ,

		IS_LOOP_WRONG_FACE_TYPE = ( 1 << 8 ) ,
		IS_LOOP_WRONG_EDGE_TYPE = ( 1 << 9 ) ,
		IS_LOOP_WRONG_VERT_TYPE = ( 1 << 10 ) ,
		IS_LOOP_VERT_NOT_IN_EDGE = ( 1 << 11 ) ,
		IS_LOOP_NULL_CYCLE_LINK = ( 1 << 12 ) ,
		IS_LOOP_ZERO_FACE_LENGTH = ( 1 << 13 ) ,
		IS_LOOP_WRONG_FACE_LENGTH = ( 1 << 14 ) ,
		IS_LOOP_WRONG_RADIAL_LENGTH = ( 1 << 15 ) ,

		IS_FACE_NULL_LOOP = ( 1 << 16 ) ,
		IS_FACE_WRONG_LOOP_FACE = ( 1 << 17 ) ,
		IS_FACE_NULL_EDGE = ( 1 << 18 ) ,
		IS_FACE_NULL_VERT = ( 1 << 19 ) ,
		IS_FACE_LOOP_VERT_NOT_IN_EDGE = ( 1 << 20 ) ,
		IS_FACE_LOOP_WRONG_RADIAL_LENGTH = ( 1 << 21 ) ,
		IS_FACE_LOOP_WRONG_DISK_LENGTH = ( 1 << 22 ) ,
		IS_FACE_LOOP_DUPE_LOOP = ( 1 << 23 ) ,
		IS_FACE_LOOP_DUPE_VERT = ( 1 << 24 ) ,
		IS_FACE_LOOP_DUPE_EDGE = ( 1 << 25 ) ,
		IS_FACE_WRONG_LENGTH = ( 1 << 26 ) ,
	};

	int err = 0;

	if ( !element ) {
		return IS_NULL;
	}

	if ( head->ElemType != htype ) {
		return IS_WRONG_TYPE;
	}

	switch ( htype ) {
		case BM_VERT: {
			BMVert *v = ( BMVert * ) element;
			if ( v->Edge && v->Edge->Head.ElemType != BM_EDGE ) {
				err |= IS_VERT_WRONG_EDGE_TYPE;
			}
			break;
		}
		case BM_EDGE: {
			BMEdge *e = ( BMEdge * ) element;
			if ( e->DiskLink1.Prev == NULL || e->DiskLink2.Prev == NULL ||
			     e->DiskLink1.Next == NULL || e->DiskLink2.Next == NULL ) {
				err |= IS_EDGE_NULL_DISK_LINK;
			}

			if ( e->Loop && e->Loop->Head.ElemType != BM_LOOP ) {
				err |= IS_EDGE_WRONG_LOOP_TYPE;
			}
			if ( e->Loop && e->Loop->Face->Head.ElemType != BM_FACE ) {
				err |= IS_EDGE_WRONG_FACE_TYPE;
			}
			if ( e->Loop && ( e->Loop->RadialNext == NULL || e->Loop->RadialPrev == NULL ) ) {
				err |= IS_EDGE_NULL_RADIAL_LINK;
			}
			if ( e->Loop && e->Loop->Face->Len <= 0 ) {
				err |= IS_EDGE_ZERO_FACE_LENGTH;
			}
			break;
		}
		case BM_LOOP: {
			BMLoop *l = ( BMLoop * ) element , *l2;
			int i;

			if ( l->Face->Head.ElemType != BM_FACE ) {
				err |= IS_LOOP_WRONG_FACE_TYPE;
			}
			if ( l->Edge->Head.ElemType != BM_EDGE ) {
				err |= IS_LOOP_WRONG_EDGE_TYPE;
			}
			if ( l->Vert->Head.ElemType != BM_VERT ) {
				err |= IS_LOOP_WRONG_VERT_TYPE;
			}
			if ( !BM_vert_in_edge ( l->Edge , l->Vert ) ) {
				fprintf ( stderr ,
					  "%s: fatal bmesh error (vert not in edge)! (bmesh internal error)\n" ,
					  __func__ );
				err |= IS_LOOP_VERT_NOT_IN_EDGE;
			}

			if ( l->RadialNext == NULL || l->RadialPrev == NULL ) {
				err |= IS_LOOP_NULL_CYCLE_LINK;
			}
			if ( l->Face->Len <= 0 ) {
				err |= IS_LOOP_ZERO_FACE_LENGTH;
			}

			/* validate boundary loop -- invalid for hole loops, of course,
			 * but we won't be allowing those for a while yet */
			l2 = l;
			i = 0;
			do {
				if ( i >= BM_NGON_MAX ) {
					break;
				}

				i++;
			} while ( ( l2 = l2->Next ) != l );

			if ( i != l->Face->Len || l2 != l ) {
				err |= IS_LOOP_WRONG_FACE_LENGTH;
			}

			if ( !bmesh_radial_validate ( bmesh_radial_length ( l ) , l ) ) {
				err |= IS_LOOP_WRONG_RADIAL_LENGTH;
			}

			break;
		}
		case BM_FACE: {
			BMFace *f = ( BMFace * ) element;
			BMLoop *l_iter;
			BMLoop *l_first;
			int len = 0;

			if ( !f->LoopFirst )
			{
				err |= IS_FACE_NULL_LOOP;
			}
			l_iter = l_first = BM_FACE_FIRST_LOOP ( f );
			do {
				if ( l_iter->Face != f ) {
					fprintf ( stderr ,
						  "%s: loop inside one face points to another! (bmesh internal error)\n" ,
						  __func__ );
					err |= IS_FACE_WRONG_LOOP_FACE;
				}

				if ( !l_iter->Edge ) {
					err |= IS_FACE_NULL_EDGE;
				}
				if ( !l_iter->Vert ) {
					err |= IS_FACE_NULL_VERT;
				}
				if ( l_iter->Edge && l_iter->Vert ) {
					if ( !BM_vert_in_edge ( l_iter->Edge , l_iter->Vert ) ||
					     !BM_vert_in_edge ( l_iter->Edge , l_iter->Next->Vert ) ) {
						err |= IS_FACE_LOOP_VERT_NOT_IN_EDGE;
					}

					if ( !bmesh_radial_validate ( bmesh_radial_length ( l_iter ) , l_iter ) ) {
						err |= IS_FACE_LOOP_WRONG_RADIAL_LENGTH;
					}

					if ( bmesh_disk_count_at_most ( l_iter->Vert , 2 ) < 2 ) {
						err |= IS_FACE_LOOP_WRONG_DISK_LENGTH;
					}
				}

				/* check for duplicates */
				if ( BM_ELEM_API_FLAG_TEST ( l_iter , _FLAG_ELEM_CHECK ) ) {
					err |= IS_FACE_LOOP_DUPE_LOOP;
				}
				BM_ELEM_API_FLAG_ENABLE ( l_iter , _FLAG_ELEM_CHECK );
				if ( l_iter->Vert ) {
					if ( BM_ELEM_API_FLAG_TEST ( l_iter->Vert , _FLAG_ELEM_CHECK ) ) {
						err |= IS_FACE_LOOP_DUPE_VERT;
					}
					BM_ELEM_API_FLAG_ENABLE ( l_iter->Vert , _FLAG_ELEM_CHECK );
				}
				if ( l_iter->Edge ) {
					if ( BM_ELEM_API_FLAG_TEST ( l_iter->Edge , _FLAG_ELEM_CHECK ) ) {
						err |= IS_FACE_LOOP_DUPE_EDGE;
					}
					BM_ELEM_API_FLAG_ENABLE ( l_iter->Edge , _FLAG_ELEM_CHECK );
				}

				len++;
			} while ( ( l_iter = l_iter->Next ) != l_first );

			/* cleanup duplicates flag */
			l_iter = l_first = BM_FACE_FIRST_LOOP ( f );
			do {
				BM_ELEM_API_FLAG_DISABLE ( l_iter , _FLAG_ELEM_CHECK );
				if ( l_iter->Vert ) {
					BM_ELEM_API_FLAG_DISABLE ( l_iter->Vert , _FLAG_ELEM_CHECK );
				}
				if ( l_iter->Edge ) {
					BM_ELEM_API_FLAG_DISABLE ( l_iter->Edge , _FLAG_ELEM_CHECK );
				}
			} while ( ( l_iter = l_iter->Next ) != l_first );

			if ( len != f->Len ) {
				err |= IS_FACE_WRONG_LENGTH;
			}
			break;
		}
		default: {
			LIB_assert_unreachable ( );
		}break;
	}

	LIB_assert ( err == 0 );

	return err;
}

#endif // NDEBUG

/**
* low level function, only frees the vert,
* doesn't change or adjust surrounding geometry
*/
static void bm_kill_only_vert ( BMesh *bm , BMVert *v ) {
	bm->TotVert--;
	bm->ElemIndexDirty |= BM_VERT;
	bm->ElemTableDirty |= BM_VERT;

	if ( v->Head.Data ) {
		CustomData_bmesh_free_block ( &bm->VertData , &v->Head.Data );
	}

	LIB_mempool_free ( bm->VertPool , v );
}

/**
* low level function, only frees the edge,
* doesn't change or adjust surrounding geometry
*/
static void bm_kill_only_edge ( BMesh *bm , BMEdge *e ) {
	bm->TotEdge--;
	bm->ElemIndexDirty |= BM_EDGE;
	bm->ElemTableDirty |= BM_EDGE;

	if ( e->Head.Data ) {
		CustomData_bmesh_free_block ( &bm->EdgeData , &e->Head.Data );
	}

	LIB_mempool_free ( bm->EdgePool , e );
}

/**
* low level function, only frees the face,
* doesn't change or adjust surrounding geometry
*/
static void bm_kill_only_face ( BMesh *bm , BMFace *f ) {
	bm->TotFace--;
	bm->ElemIndexDirty |= BM_FACE;
	bm->ElemTableDirty |= BM_FACE;

	if ( f->Head.Data ) {
		CustomData_bmesh_free_block ( &bm->FaceData , &f->Head.Data );
	}

	LIB_mempool_free ( bm->FacePool , f );
}

/**
* low level function, only frees the loop,
* doesn't change or adjust surrounding geometry
*/
static void bm_kill_only_loop ( BMesh *bm , BMLoop *l ) {
	bm->TotLoop--;
	bm->ElemIndexDirty |= BM_LOOP;

	LIB_mempool_free ( bm->LoopPool , l );
}

void BM_face_edges_kill ( BMesh *bm , BMFace *f ) {
	BMEdge **edges = ( BMEdge ** ) alloca ( sizeof ( *edges ) * f->Len );
	BMLoop *l_iter;
	BMLoop *l_first;
	int i = 0;

	l_iter = l_first = BM_FACE_FIRST_LOOP ( f );
	do {
		edges [ i++ ] = l_iter->Edge;
	} while ( ( l_iter = l_iter->Next ) != l_first );

	for ( i = 0; i < f->Len; i++ ) {
		BM_edge_kill ( bm , edges [ i ] );
	}
}

void BM_face_verts_kill ( BMesh *bm , BMFace *f ) {
	BMVert **verts = ( BMVert ** ) alloca ( sizeof ( *verts ) * f->Len );
	BMLoop *l_iter;
	BMLoop *l_first;
	int i = 0;

	l_iter = l_first = BM_FACE_FIRST_LOOP ( f );
	do {
		verts [ i++ ] = l_iter->Vert;
	} while ( ( l_iter = l_iter->Next ) != l_first );

	for ( i = 0; i < f->Len; i++ ) {
		BM_vert_kill ( bm , verts [ i ] );
	}
}

void BM_face_kill ( BMesh *bm , BMFace *f ) {
#ifdef NDEBUG
	/* check length since we may be removing degenerate faces */
	if ( f->Len >= 3 ) {
		BM_CHECK_ELEMENT ( f );
	}
#endif

	if ( f->LoopFirst ) {
		BMLoop *l_iter , *l_next , *l_first;

		l_iter = l_first = f->LoopFirst;

		do {
			l_next = l_iter->Next;

			bmesh_radial_loop_remove ( l_iter->Edge , l_iter );
			bm_kill_only_loop ( bm , l_iter );

		} while ( ( l_iter = l_next ) != l_first );

	}

	bm_kill_only_face ( bm , f );
}

void BM_face_kill_loose ( BMesh *bm , BMFace *f ) {
	BM_CHECK_ELEMENT ( f );

	if ( f->LoopFirst ) {
		BMLoop *l_iter , *l_next , *l_first;

		l_iter = l_first = f->LoopFirst;

		do {
			BMEdge *e;
			l_next = l_iter->Next;

			e = l_iter->Edge;
			bmesh_radial_loop_remove ( e , l_iter );
			bm_kill_only_loop ( bm , l_iter );

			if ( e->Loop == NULL ) {
				BMVert *v1 = e->Vert1 , *v2 = e->Vert2;

				bmesh_disk_edge_remove ( e , e->Vert1 );
				bmesh_disk_edge_remove ( e , e->Vert2 );
				bm_kill_only_edge ( bm , e );

				if ( v1->Edge == NULL ) {
					bm_kill_only_vert ( bm , v1 );
				}
				if ( v2->Edge == NULL ) {
					bm_kill_only_vert ( bm , v2 );
				}
			}
		} while ( ( l_iter = l_next ) != l_first );

	}

	bm_kill_only_face ( bm , f );
}

void BM_edge_kill ( BMesh *bm , BMEdge *e ) {
	while ( e->Loop ) {
		BM_face_kill ( bm , e->Loop->Face );
	}

	bmesh_disk_edge_remove ( e , e->Vert1 );
	bmesh_disk_edge_remove ( e , e->Vert2 );

	bm_kill_only_edge ( bm , e );
}

void BM_vert_kill ( BMesh *bm , BMVert *v ) {
	while ( v->Edge ) {
		BM_edge_kill ( bm , v->Edge );
	}

	bm_kill_only_vert ( bm , v );
}

/* -------------------------------------------------------------------- */
/** \name Private Disk & Radial Cycle Functions
* \{ */

static int UNUSED_FUNCTION ( bm_loop_length )( BMLoop *l ) {
	BMLoop *l_first = l;
	int i = 0;

	do {
		i++;
	} while ( ( l = l->Next ) != l_first );

	return i;
}

void bmesh_kernel_loop_reverse ( BMesh *bm ,
				 BMFace *f ,
				 const int ,
				 const bool use_loop_mdisp_flip ) {
	BMLoop *l_first = f->LoopFirst;

	// track previous cycles radial state
	BMEdge *e_prev = l_first->Prev->Edge;
	BMLoop *l_prev_radial_next = l_first->Prev->RadialNext;
	BMLoop *l_prev_radial_prev = l_first->Prev->RadialPrev;
	bool is_prev_boundary = l_prev_radial_next == l_prev_radial_next->RadialNext;

	BMLoop *l_iter = l_first;
	do {
		BMEdge *e_iter = l_iter->Edge;
		BMLoop *l_iter_radial_next = l_iter->RadialNext;
		BMLoop *l_iter_radial_prev = l_iter->RadialPrev;
		bool is_iter_boundary = l_iter_radial_next == l_iter_radial_next->RadialNext;

#if 0
		bmesh_radial_loop_remove ( e_iter , l_iter );
		bmesh_radial_loop_append ( e_prev , l_iter );
#else
		// inline loop reversal
		if ( is_prev_boundary ) {
			// boundary
			l_iter->RadialNext = l_iter;
			l_iter->RadialPrev = l_iter;
		} else {
			// non-boundary, replace radial links
			l_iter->RadialNext = l_prev_radial_next;
			l_iter->RadialPrev = l_prev_radial_prev;
			l_prev_radial_next->RadialPrev = l_iter;
			l_prev_radial_prev->RadialNext = l_iter;
		}

		if ( e_iter->Loop == l_iter ) {
			e_iter->Loop = l_iter->Next;
		}
		l_iter->Edge = e_prev;
#endif

		SWAP ( BMLoop * , l_iter->Next , l_iter->Prev );

		e_prev = e_iter;
		l_prev_radial_next = l_iter_radial_next;
		l_prev_radial_prev = l_iter_radial_prev;
		is_prev_boundary = is_iter_boundary;

		/* step to next (now swapped) */
	} while ( ( l_iter = l_iter->Prev ) != l_first );

#ifndef NDEBUG
	// validate radial
	int i;
	for ( i = 0 , l_iter = l_first; i < f->Len; i++ , l_iter = l_iter->Next ) {
		BM_CHECK_ELEMENT ( l_iter );
		BM_CHECK_ELEMENT ( l_iter->Edge );
		BM_CHECK_ELEMENT ( l_iter->Vert );
		BM_CHECK_ELEMENT ( l_iter->Face );
	}

	BM_CHECK_ELEMENT ( f );
#endif

	// Loop indices are no more valid!
	bm->ElemIndexDirty |= BM_LOOP;
}

static void bm_elements_systag_enable ( void *veles , int tot , const char api_flag ) {
	BMHeader **eles = ( BMHeader ** ) veles;
	int i;

	for ( i = 0; i < tot; i++ ) {
		BM_ELEM_API_FLAG_ENABLE ( ( BMElem * ) eles [ i ] , api_flag );
	}
}

static void bm_elements_systag_disable ( void *veles , int tot , const char api_flag ) {
	BMHeader **eles = ( BMHeader ** ) veles;
	int i;

	for ( i = 0; i < tot; i++ ) {
		BM_ELEM_API_FLAG_DISABLE ( ( BMElem * ) eles [ i ] , api_flag );
	}
}

static int bm_loop_systag_count_radial ( BMLoop *l , const char api_flag ) {
	BMLoop *l_iter = l;
	int i = 0;
	do {
		i += BM_ELEM_API_FLAG_TEST ( l_iter->Face , api_flag ) ? 1 : 0;
	} while ( ( l_iter = l_iter->RadialNext ) != l );

	return i;
}

static int UNUSED_FUNCTION ( bm_vert_systag_count_disk )( BMVert *v , const char api_flag ) {
	BMEdge *e = v->Edge;
	int i = 0;

	if ( !e ) {
		return 0;
	}

	do {
		i += BM_ELEM_API_FLAG_TEST ( e , api_flag ) ? 1 : 0;
	} while ( ( e = bmesh_disk_edge_next ( e , v ) ) != v->Edge );

	return i;
}

static bool bm_vert_is_manifold_flagged ( BMVert *v , const char api_flag ) {
	BMEdge *e = v->Edge;

	if ( !e ) {
		return false;
	}

	do {
		BMLoop *l = e->Loop;

		if ( !l ) {
			return false;
		}

		if ( BM_edge_is_boundary ( l->Edge ) ) {
			return false;
		}

		do {
			if ( !BM_ELEM_API_FLAG_TEST ( l->Face , api_flag ) ) {
				return false;
			}
		} while ( ( l = l->RadialNext ) != e->Loop );
	} while ( ( e = bmesh_disk_edge_next ( e , v ) ) != v->Edge );

	return true;
}

/** \{ */

/* -------------------------------------------------------------------- */
/** \name Mid-level Topology Manipulation Functions
* \{ */



/** \{ */
