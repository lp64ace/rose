#include "bmesh_query.h"
#include "bmesh_inline.h"
#include "bmesh_iterators.h"
#include "bmesh_structure.h"
#include "bmesh_construct.h"

#include "lib/lib_alloc.h"
#include "lib/lib_math.h"

#include "kernel/rke_customdata.h"

#include <float.h>

BMLoop *BM_face_other_edge_loop ( BMFace *f , BMEdge *e , BMVert *v ) {
	BMLoop *l = BM_face_edge_share_loop ( f , e );
	LIB_assert ( l != NULL );
	return BM_loop_other_edge_loop ( l , v );
}

BMLoop *BM_loop_other_edge_loop ( BMLoop *l , BMVert *v ) {
	LIB_assert ( BM_vert_in_edge ( l->Edge , v ) );
	return l->Vert == v ? l->Prev : l->Next;
}

BMLoop *BM_face_other_vert_loop ( BMFace *f , BMVert *v_prev , BMVert *v ) {
	BMLoop *l_iter = BM_face_vert_share_loop ( f , v );

	LIB_assert ( BM_edge_exists ( v_prev , v ) != NULL );

	if ( l_iter ) {
		if ( l_iter->Prev->Vert == v_prev ) {
			return l_iter->Next;
		}
		if ( l_iter->Next->Vert == v_prev ) {
			return l_iter->Prev;
		}
	}
	LIB_assert_unreachable ( );
	return NULL;
}

BMLoop *BM_loop_other_vert_loop ( BMLoop *l , BMVert *v ) {
	BMEdge *e = l->Edge;
	BMVert *v_prev = BM_edge_other_vert ( e , v );
	if ( l->Vert == v ) {
		if ( l->Prev->Vert == v_prev ) {
			return l->Next;
		}
		LIB_assert ( l->Next->Vert == v_prev );

		return l->Prev;
	}
	LIB_assert ( l->Vert == v_prev );

	if ( l->Prev->Vert == v ) {
		return l->Prev->Prev;
	}
	LIB_assert ( l->Next->Vert == v );
	return l->Next->Next;
}

BMLoop *BM_loop_other_vert_loop_by_edge ( BMLoop *l , BMEdge *e ) {
	LIB_assert ( BM_vert_in_edge ( e , l->Vert ) );
	if ( l->Edge == e ) {
		return l->Next;
	}
	if ( l->Prev->Edge == e ) {
		return l->Prev;
	}

	LIB_assert ( 0 );
	return NULL;
}

bool BM_vert_pair_share_face_check ( BMVert *v_a , BMVert *v_b ) {
	if ( v_a->Edge && v_b->Edge ) {
		BMIter iter;
		BMFace *f;

		BM_ITER_ELEM ( f , &iter , v_a , BM_FACES_OF_VERT ) {
			if ( BM_vert_in_face ( v_b , f ) ) {
				return true;
			}
		}
	}

	return false;
}

bool BM_vert_pair_share_face_check_cb ( BMVert *v_a , BMVert *v_b , bool ( *test_fn )( BMFace * , void *user_data ) , void *user_data ) {
	if ( v_a->Edge && v_b->Edge ) {
		BMIter iter;
		BMFace *f;

		BM_ITER_ELEM ( f , &iter , v_a , BM_FACES_OF_VERT ) {
			if ( test_fn ( f , user_data ) ) {
				if ( BM_vert_in_face ( v_b , f ) ) {
					return true;
				}
			}
		}
	}

	return false;
}

BMFace *BM_vert_pair_shared_face_cb ( BMVert *v_a , BMVert *v_b , const bool allow_adjacent ,
				      bool ( *callback )( BMFace * , BMLoop * , BMLoop * , void *userdata ) ,
				      void *user_data , BMLoop **r_l_a , BMLoop **r_l_b ) {
	if ( v_a->Edge && v_b->Edge ) {
		BMIter iter;
		BMLoop *l_a , *l_b;

		BM_ITER_ELEM ( l_a , &iter , v_a , BM_LOOPS_OF_VERT ) {
			BMFace *f = l_a->Face;
			l_b = BM_face_vert_share_loop ( f , v_b );
			if ( l_b && ( allow_adjacent || !BM_loop_is_adjacent ( l_a , l_b ) ) &&
			     callback ( f , l_a , l_b , user_data ) ) {
				*r_l_a = l_a;
				*r_l_b = l_b;

				return f;
			}
		}
	}

	return NULL;
}

BMFace *BM_vert_pair_share_face_by_len ( BMVert *v_a , BMVert *v_b , BMLoop **r_l_a , BMLoop **r_l_b , const bool allow_adjacent ) {
	BMLoop *l_cur_a = NULL , *l_cur_b = NULL;
	BMFace *f_cur = NULL;

	if ( v_a->Edge && v_b->Edge ) {
		BMIter iter;
		BMLoop *l_a , *l_b;

		BM_ITER_ELEM ( l_a , &iter , v_a , BM_LOOPS_OF_VERT ) {
			if ( ( f_cur == NULL ) || ( l_a->Face->Len < f_cur->Len ) ) {
				l_b = BM_face_vert_share_loop ( l_a->Face , v_b );
				if ( l_b && ( allow_adjacent || !BM_loop_is_adjacent ( l_a , l_b ) ) ) {
					f_cur = l_a->Face;
					l_cur_a = l_a;
					l_cur_b = l_b;
				}
			}
		}
	}

	*r_l_a = l_cur_a;
	*r_l_b = l_cur_b;

	return f_cur;
}

BMFace *BM_edge_pair_share_face_by_len ( BMEdge *e_a , BMEdge *e_b , BMLoop **r_l_a , BMLoop **r_l_b , const bool allow_adjacent ) {
	BMLoop *l_cur_a = NULL , *l_cur_b = NULL;
	BMFace *f_cur = NULL;

	if ( e_a->Loop && e_b->Loop ) {
		BMIter iter;
		BMLoop *l_a , *l_b;

		BM_ITER_ELEM ( l_a , &iter , e_a , BM_LOOPS_OF_EDGE ) {
			if ( ( f_cur == NULL ) || ( l_a->Face->Len < f_cur->Len ) ) {
				l_b = BM_face_edge_share_loop ( l_a->Face , e_b );
				if ( l_b && ( allow_adjacent || !BM_loop_is_adjacent ( l_a , l_b ) ) ) {
					f_cur = l_a->Face;
					l_cur_a = l_a;
					l_cur_b = l_b;
				}
			}
		}
	}

	*r_l_a = l_cur_a;
	*r_l_b = l_cur_b;

	return f_cur;
}

BMLoop *BM_vert_find_first_loop ( BMVert *v ) {
	return v->Edge ? bmesh_disk_faceloop_find_first ( v->Edge , v ) : NULL;
}
BMLoop *BM_vert_find_first_loop_visible ( BMVert *v ) {
	return v->Edge ? bmesh_disk_faceloop_find_first_visible ( v->Edge , v ) : NULL;
}

bool BM_vert_in_face ( BMVert *v , BMFace *f ) {
	BMLoop *l_iter , *l_first;

	{
		l_iter = l_first = f->LoopFirst;
		do {
			if ( l_iter->Vert == v ) {
				return true;
			}
		} while ( ( l_iter = l_iter->Next ) != l_first );
	}

	return false;
}

int BM_verts_in_face_count ( BMVert **varr , int len , BMFace *f ) {
	BMLoop *l_iter , *l_first;

	int i , count = 0;

	for ( i = 0; i < len; i++ ) {
		BM_ELEM_API_FLAG_ENABLE ( varr [ i ] , _FLAG_OVERLAP );
	}

	{
		l_iter = l_first = f->LoopFirst;

		do {
			if ( BM_ELEM_API_FLAG_TEST ( l_iter->Vert , _FLAG_OVERLAP ) ) {
				count++;
			}

		} while ( ( l_iter = l_iter->Next ) != l_first );
	}

	for ( i = 0; i < len; i++ ) {
		BM_ELEM_API_FLAG_DISABLE ( varr [ i ] , _FLAG_OVERLAP );
	}

	return count;
}

bool BM_verts_in_face ( BMVert **varr , int len , BMFace *f ) {
	BMLoop *l_iter , *l_first;

#ifdef USE_BMESH_HOLES
	BMLoopList *lst;
#endif

	int i;
	bool ok = true;

	/* simple check, we know can't succeed */
	if ( f->Len < len ) {
		return false;
	}

	for ( i = 0; i < len; i++ ) {
		BM_ELEM_API_FLAG_ENABLE ( varr [ i ] , _FLAG_OVERLAP );
	}

	{
		l_iter = l_first = f->LoopFirst;

		do {
			if ( BM_ELEM_API_FLAG_TEST ( l_iter->Vert , _FLAG_OVERLAP ) ) {
				/* pass */
			} else {
				ok = false;
				break;
			}

		} while ( ( l_iter = l_iter->Next ) != l_first );
	}

	for ( i = 0; i < len; i++ ) {
		BM_ELEM_API_FLAG_DISABLE ( varr [ i ] , _FLAG_OVERLAP );
	}

	return ok;
}

bool BM_edge_in_face ( const BMEdge *e , const BMFace *f ) {
	if ( e->Loop ) {
		const BMLoop *l_iter , *l_first;

		l_iter = l_first = e->Loop;
		do {
			if ( l_iter->Face == f ) {
				return true;
			}
		} while ( ( l_iter = l_iter->RadialNext ) != l_first );
	}

	return false;
}

BMLoop *BM_edge_other_loop ( BMEdge *e , BMLoop *l ) {
	BMLoop *l_other;

	LIB_assert ( e->Loop && e->Loop->RadialNext != e->Loop );
	LIB_assert ( BM_vert_in_edge ( e , l->Vert ) );

	l_other = ( l->Edge == e ) ? l : l->Prev;
	l_other = l_other->RadialNext;
	LIB_assert ( l_other->Edge == e );

	if ( l_other->Vert == l->Vert ) {
		/* pass */
	} else if ( l_other->Next->Vert == l->Vert ) {
		l_other = l_other->Next;
	} else {
		LIB_assert_unreachable ( );
	}

	return l_other;
}

BMLoop *BM_vert_step_fan_loop ( BMLoop *l , BMEdge **e_step ) {
	BMEdge *e_prev = *e_step;
	BMEdge *e_next;
	if ( l->Edge == e_prev ) {
		e_next = l->Prev->Edge;
	} else if ( l->Prev->Edge == e_prev ) {
		e_next = l->Edge;
	} else {
		LIB_assert_unreachable ( );
		return NULL;
	}

	if ( BM_edge_is_manifold ( e_next ) ) {
		return BM_edge_other_loop ( ( *e_step = e_next ) , l );
	}
	return NULL;
}

BMEdge *BM_vert_other_disk_edge ( BMVert *v , BMEdge *e_first ) {
	BMLoop *l_a;
	int tot = 0;
	int i;

	LIB_assert ( BM_vert_in_edge ( e_first , v ) );

	l_a = e_first->Loop;
	do {
		l_a = BM_loop_other_vert_loop ( l_a , v );
		l_a = BM_vert_in_edge ( l_a->Edge , v ) ? l_a : l_a->Prev;
		if ( BM_edge_is_manifold ( l_a->Edge ) ) {
			l_a = l_a->RadialNext;
		} else {
			return NULL;
		}

		tot++;
	} while ( l_a != e_first->Loop );

	// We know the total, now loop half way
	tot /= 2;
	i = 0;

	l_a = e_first->Loop;
	do {
		if ( i == tot ) {
			l_a = BM_vert_in_edge ( l_a->Edge , v ) ? l_a : l_a->Prev;
			return l_a->Edge;
		}

		l_a = BM_loop_other_vert_loop ( l_a , v );
		l_a = BM_vert_in_edge ( l_a->Edge , v ) ? l_a : l_a->Prev;
		if ( BM_edge_is_manifold ( l_a->Edge ) ) {
			l_a = l_a->RadialNext;
		}
		// This won't have changed from the previous loop

		i++;
	} while ( l_a != e_first->Loop );

	return NULL;
}

float BM_edge_calc_length ( const BMEdge *e ) {
	return len_v3v3 ( e->Vert1->Coord , e->Vert2->Coord );
}

float BM_edge_calc_length_squared ( const BMEdge *e ) {
	return len_squared_v3v3 ( e->Vert1->Coord , e->Vert2->Coord );
}

bool BM_edge_face_pair ( BMEdge *e , BMFace **r_fa , BMFace **r_fb ) {
	BMLoop *la , *lb;

	if ( ( la = e->Loop ) && ( lb = la->RadialNext ) && ( la != lb ) && ( lb->RadialNext == la ) ) {
		*r_fa = la->Face;
		*r_fb = lb->Face;
		return true;
	}

	*r_fa = NULL;
	*r_fb = NULL;
	return false;
}

bool BM_edge_loop_pair ( BMEdge *e , BMLoop **r_la , BMLoop **r_lb ) {
	BMLoop *la , *lb;

	if ( ( la = e->Loop ) && ( lb = la->RadialNext ) && ( la != lb ) && ( lb->RadialNext == la ) ) {
		*r_la = la;
		*r_lb = lb;
		return true;
	}

	*r_la = NULL;
	*r_lb = NULL;
	return false;
}

bool BM_vert_is_edge_pair ( const BMVert *v ) {
	const BMEdge *e = v->Edge;
	if ( e ) {
		BMEdge *e_other = BM_DISK_EDGE_NEXT ( e , v );
		return ( ( e_other != e ) && ( BM_DISK_EDGE_NEXT ( e_other , v ) == e ) );
	}
	return false;
}

bool BM_vert_is_edge_pair_manifold ( const BMVert *v ) {
	const BMEdge *e = v->Edge;
	if ( e ) {
		BMEdge *e_other = BM_DISK_EDGE_NEXT ( e , v );
		if ( ( ( e_other != e ) && ( BM_DISK_EDGE_NEXT ( e_other , v ) == e ) ) ) {
			return BM_edge_is_manifold ( e ) && BM_edge_is_manifold ( e_other );
		}
	}
	return false;
}

bool BM_vert_edge_pair ( BMVert *v , BMEdge **r_e_a , BMEdge **r_e_b ) {
	BMEdge *e_a = v->Edge;
	if ( e_a ) {
		BMEdge *e_b = BM_DISK_EDGE_NEXT ( e_a , v );
		if ( ( e_b != e_a ) && ( BM_DISK_EDGE_NEXT ( e_b , v ) == e_a ) ) {
			*r_e_a = e_a;
			*r_e_b = e_b;
			return true;
		}
	}

	*r_e_a = NULL;
	*r_e_b = NULL;
	return false;
}

int BM_vert_edge_count ( const BMVert *v ) {
	return bmesh_disk_count ( v );
}

int BM_vert_edge_count_at_most ( const BMVert *v , const int count_max ) {
	return bmesh_disk_count_at_most ( v , count_max );
}

int BM_vert_edge_count_nonwire ( const BMVert *v ) {
	int count = 0;
	BMIter eiter;
	BMEdge *edge;
	BM_ITER_ELEM ( edge , &eiter , ( BMVert * ) v , BM_EDGES_OF_VERT ) {
		if ( edge->Loop ) {
			count++;
		}
	}
	return count;
}

int BM_edge_face_count ( const BMEdge *e ) {
	int count = 0;

	if ( e->Loop ) {
		BMLoop *l_iter , *l_first;

		l_iter = l_first = e->Loop;
		do {
			count++;
		} while ( ( l_iter = l_iter->RadialNext ) != l_first );
	}

	return count;
}

int BM_edge_face_count_at_most ( const BMEdge *e , const int count_max ) {
	int count = 0;

	if ( e->Loop ) {
		BMLoop *l_iter , *l_first;

		l_iter = l_first = e->Loop;
		do {
			count++;
			if ( count == count_max ) {
				break;
			}
		} while ( ( l_iter = l_iter->RadialNext ) != l_first );
	}

	return count;
}

int BM_vert_face_count ( const BMVert *v ) {
	return bmesh_disk_facevert_count ( v );
}

int BM_vert_face_count_at_most ( const BMVert *v , int count_max ) {
	return bmesh_disk_facevert_count_at_most ( v , count_max );
}

bool BM_vert_face_check ( const BMVert *v ) {
	if ( v->Edge != NULL ) {
		const BMEdge *e_iter , *e_first;
		e_first = e_iter = v->Edge;
		do {
			if ( e_iter->Loop != NULL ) {
				return true;
			}
		} while ( ( e_iter = bmesh_disk_edge_next ( e_iter , v ) ) != e_first );
	}
	return false;
}

bool BM_vert_is_wire ( const BMVert *v ) {
	if ( v->Edge ) {
		BMEdge *e_first , *e_iter;

		e_first = e_iter = v->Edge;
		do {
			if ( e_iter->Loop ) {
				return false;
			}
		} while ( ( e_iter = bmesh_disk_edge_next ( e_iter , v ) ) != e_first );

		return true;
	}
	return false;
}

bool BM_vert_is_manifold ( const BMVert *v ) {
	BMEdge *e_iter , *e_first , *e_prev;
	BMLoop *l_iter , *l_first;
	int loop_num = 0 , loop_num_region = 0 , boundary_num = 0;

	if ( v->Edge == NULL ) {
		/* loose vert */
		return false;
	}

	/* count edges while looking for non-manifold edges */
	e_first = e_iter = v->Edge;
	/* may be null */
	l_first = e_iter->Loop;
	do {
		/* loose edge or edge shared by more than two faces,
		 * edges with 1 face user are OK, otherwise we could
		 * use BM_edge_is_manifold() here */
		if ( e_iter->Loop == NULL || ( e_iter->Loop != e_iter->Loop->RadialNext->RadialNext ) ) {
			return false;
		}

		/* count radial loops */
		if ( e_iter->Loop->Vert == v ) {
			loop_num += 1;
		}

		if ( !BM_edge_is_boundary ( e_iter ) ) {
			/* non boundary check opposite loop */
			if ( e_iter->Loop->RadialNext->Vert == v ) {
				loop_num += 1;
			}
		} else {
			/* start at the boundary */
			l_first = e_iter->Loop;
			boundary_num += 1;
			/* >2 boundaries can't be manifold */
			if ( boundary_num == 3 ) {
				return false;
			}
		}
	} while ( ( e_iter = bmesh_disk_edge_next ( e_iter , v ) ) != e_first );

	e_first = l_first->Edge;
	l_first = ( l_first->Vert == v ) ? l_first : l_first->Next;
	LIB_assert ( l_first->Vert == v );

	l_iter = l_first;
	e_prev = e_first;

	do {
		loop_num_region += 1;
	} while ( ( ( l_iter = BM_vert_step_fan_loop ( l_iter , &e_prev ) ) != l_first ) && ( l_iter != NULL ) );

	return ( loop_num == loop_num_region );
}

#define LOOP_VISIT _FLAG_WALK
#define EDGE_VISIT _FLAG_WALK

static int bm_loop_region_count__recursive ( BMEdge *e , BMVert *v ) {
	BMLoop *l_iter , *l_first;
	int count = 0;

	LIB_assert ( !BM_ELEM_API_FLAG_TEST ( e , EDGE_VISIT ) );
	BM_ELEM_API_FLAG_ENABLE ( e , EDGE_VISIT );

	l_iter = l_first = e->Loop;
	do {
		if ( l_iter->Vert == v ) {
			BMEdge *e_other = l_iter->Prev->Edge;
			if ( !BM_ELEM_API_FLAG_TEST ( l_iter , LOOP_VISIT ) ) {
				BM_ELEM_API_FLAG_ENABLE ( l_iter , LOOP_VISIT );
				count += 1;
			}
			if ( !BM_ELEM_API_FLAG_TEST ( e_other , EDGE_VISIT ) ) {
				count += bm_loop_region_count__recursive ( e_other , v );
			}
		} else if ( l_iter->Next->Vert == v ) {
			BMEdge *e_other = l_iter->Next->Edge;
			if ( !BM_ELEM_API_FLAG_TEST ( l_iter->Next , LOOP_VISIT ) ) {
				BM_ELEM_API_FLAG_ENABLE ( l_iter->Next , LOOP_VISIT );
				count += 1;
			}
			if ( !BM_ELEM_API_FLAG_TEST ( e_other , EDGE_VISIT ) ) {
				count += bm_loop_region_count__recursive ( e_other , v );
			}
		} else {
			LIB_assert_unreachable ( );
		}
	} while ( ( l_iter = l_iter->RadialNext ) != l_first );

	return count;
}

static int bm_loop_region_count__clear ( BMLoop *l ) {
	int count = 0;
	BMEdge *e_iter , *e_first;

	/* clear flags */
	e_iter = e_first = l->Edge;
	do {
		BM_ELEM_API_FLAG_DISABLE ( e_iter , EDGE_VISIT );
		if ( e_iter->Loop ) {
			BMLoop *l_iter , *l_first;
			l_iter = l_first = e_iter->Loop;
			do {
				if ( l_iter->Vert == l->Vert ) {
					BM_ELEM_API_FLAG_DISABLE ( l_iter , LOOP_VISIT );
					count += 1;
				}
			} while ( ( l_iter = l_iter->RadialNext ) != l_first );
		}
	} while ( ( e_iter = BM_DISK_EDGE_NEXT ( e_iter , l->Vert ) ) != e_first );

	return count;
}

int BM_loop_region_loops_count_at_most ( BMLoop *l , int *r_loop_total ) {
	const int count = bm_loop_region_count__recursive ( l->Edge , l->Vert );
	const int count_total = bm_loop_region_count__clear ( l );
	if ( r_loop_total ) {
		*r_loop_total = count_total;
	}
	return count;
}

#undef LOOP_VISIT
#undef EDGE_VISIT

int BM_loop_region_loops_count ( BMLoop *l ) {
	return BM_loop_region_loops_count_at_most ( l , NULL );
}

bool BM_vert_is_manifold_region ( const BMVert *v ) {
	BMLoop *l_first = BM_vert_find_first_loop ( ( BMVert * ) v );
	if ( l_first ) {
		int count , count_total;
		count = BM_loop_region_loops_count_at_most ( l_first , &count_total );
		return ( count == count_total );
	}
	return true;
}

bool BM_edge_is_convex ( const BMEdge *e ) {
	if ( BM_edge_is_manifold ( e ) ) {
		BMLoop *l1 = e->Loop;
		BMLoop *l2 = e->Loop->RadialNext;
		if ( !equals_v3v3 ( l1->Face->Normal , l2->Face->Normal ) ) {
			float cross [ 3 ];
			float l_dir [ 3 ];
			cross_v3_v3v3 ( cross , l1->Face->Normal , l2->Face->Normal );
			// we assume contiguous normals, otherwise the result isn't meaningful.
			sub_v3_v3v3 ( l_dir , l1->Next->Vert->Coord , l1->Vert->Coord );
			return ( dot_v3v3 ( l_dir , cross ) > 0.0f );
		}
	}
	return true;
}

bool BM_edge_is_contiguous_loop_cd ( const BMEdge *e , const int cd_loop_type , const int cd_loop_offset ) {
	LIB_assert ( cd_loop_offset != -1 );

	if ( e->Loop && e->Loop->RadialNext != e->Loop ) {
		const BMLoop *l_base_v1 = e->Loop;
		const BMLoop *l_base_v2 = e->Loop->Next;
		const void *l_base_cd_v1 = BM_ELEM_CD_GET_VOID_P ( l_base_v1 , cd_loop_offset );
		const void *l_base_cd_v2 = BM_ELEM_CD_GET_VOID_P ( l_base_v2 , cd_loop_offset );
		const BMLoop *l_iter = e->Loop->RadialNext;
		do {
			const BMLoop *l_iter_v1;
			const BMLoop *l_iter_v2;
			const void *l_iter_cd_v1;
			const void *l_iter_cd_v2;

			if ( l_iter->Vert == l_base_v1->Vert ) {
				l_iter_v1 = l_iter;
				l_iter_v2 = l_iter->Next;
			} else {
				l_iter_v1 = l_iter->Next;
				l_iter_v2 = l_iter;
			}
			LIB_assert ( ( l_iter_v1->Vert == l_base_v1->Vert ) && ( l_iter_v2->Vert == l_base_v2->Vert ) );

			l_iter_cd_v1 = BM_ELEM_CD_GET_VOID_P ( l_iter_v1 , cd_loop_offset );
			l_iter_cd_v2 = BM_ELEM_CD_GET_VOID_P ( l_iter_v2 , cd_loop_offset );

			if ( ( CustomData_data_equals ( cd_loop_type , l_base_cd_v1 , l_iter_cd_v1 ) == 0 ) ||
			     ( CustomData_data_equals ( cd_loop_type , l_base_cd_v2 , l_iter_cd_v2 ) == 0 ) ) {
				return false;
			}

		} while ( ( l_iter = l_iter->RadialNext ) != e->Loop );
	}
	return true;
}

bool BM_vert_is_boundary ( const BMVert *v ) {
	if ( v->Edge ) {
		BMEdge *e_first , *e_iter;

		e_first = e_iter = v->Edge;
		do {
			if ( BM_edge_is_boundary ( e_iter ) ) {
				return true;
			}
		} while ( ( e_iter = bmesh_disk_edge_next ( e_iter , v ) ) != e_first );

		return false;
	}
	return false;
}

int BM_face_share_face_count ( BMFace *f_a , BMFace *f_b ) {
	BMIter iter1 , iter2;
	BMEdge *e;
	BMFace *f;
	int count = 0;

	BM_ITER_ELEM ( e , &iter1 , f_a , BM_EDGES_OF_FACE ) {
		BM_ITER_ELEM ( f , &iter2 , e , BM_FACES_OF_EDGE ) {
			if ( f != f_a && f != f_b && BM_face_share_edge_check ( f , f_b ) ) {
				count++;
			}
		}
	}

	return count;
}

bool BM_face_share_face_check ( BMFace *f_a , BMFace *f_b ) {
	BMIter iter1 , iter2;
	BMEdge *e;
	BMFace *f;

	BM_ITER_ELEM ( e , &iter1 , f_a , BM_EDGES_OF_FACE ) {
		BM_ITER_ELEM ( f , &iter2 , e , BM_FACES_OF_EDGE ) {
			if ( f != f_a && f != f_b && BM_face_share_edge_check ( f , f_b ) ) {
				return true;
			}
		}
	}

	return false;
}

int BM_face_share_edge_count ( BMFace *f_a , BMFace *f_b ) {
	BMLoop *l_iter;
	BMLoop *l_first;
	int count = 0;

	l_iter = l_first = BM_FACE_FIRST_LOOP ( f_a );
	do {
		if ( BM_edge_in_face ( l_iter->Edge , f_b ) ) {
			count++;
		}
	} while ( ( l_iter = l_iter->Next ) != l_first );

	return count;
}

bool BM_face_share_edge_check ( BMFace *f1 , BMFace *f2 ) {
	BMLoop *l_iter;
	BMLoop *l_first;

	l_iter = l_first = BM_FACE_FIRST_LOOP ( f1 );
	do {
		if ( BM_edge_in_face ( l_iter->Edge , f2 ) ) {
			return true;
		}
	} while ( ( l_iter = l_iter->Next ) != l_first );

	return false;
}

int BM_face_share_vert_count ( BMFace *f_a , BMFace *f_b ) {
	BMLoop *l_iter;
	BMLoop *l_first;
	int count = 0;

	l_iter = l_first = BM_FACE_FIRST_LOOP ( f_a );
	do {
		if ( BM_vert_in_face ( l_iter->Vert , f_b ) ) {
			count++;
		}
	} while ( ( l_iter = l_iter->Next ) != l_first );

	return count;
}

bool BM_face_share_vert_check ( BMFace *f_a , BMFace *f_b ) {
	BMLoop *l_iter;
	BMLoop *l_first;

	l_iter = l_first = BM_FACE_FIRST_LOOP ( f_a );
	do {
		if ( BM_vert_in_face ( l_iter->Vert , f_b ) ) {
			return true;
		}
	} while ( ( l_iter = l_iter->Next ) != l_first );

	return false;
}

bool BM_loop_share_edge_check ( BMLoop *l_a , BMLoop *l_b ) {
	LIB_assert ( l_a->Vert == l_b->Vert );
	return ( ELEM ( l_a->Edge , l_b->Edge , l_b->Prev->Edge ) ||
		 ELEM ( l_b->Edge , l_a->Edge , l_a->Prev->Edge ) );
}

bool BM_edge_share_face_check ( BMEdge *e1 , BMEdge *e2 ) {
	BMLoop *l;
	BMFace *f;

	if ( e1->Loop && e2->Loop ) {
		l = e1->Loop;
		do {
			f = l->Face;
			if ( BM_edge_in_face ( e2 , f ) ) {
				return true;
			}
			l = l->RadialNext;
		} while ( l != e1->Loop );
	}
	return false;
}

bool BM_edge_share_quad_check ( BMEdge *e1 , BMEdge *e2 ) {
	BMLoop *l;
	BMFace *f;

	if ( e1->Loop && e2->Loop ) {
		l = e1->Loop;
		do {
			f = l->Face;
			if ( f->Len == 4 ) {
				if ( BM_edge_in_face ( e2 , f ) ) {
					return true;
				}
			}
			l = l->RadialNext;
		} while ( l != e1->Loop );
	}
	return false;
}

bool BM_edge_share_vert_check ( BMEdge *e1 , BMEdge *e2 ) {
	return ( e1->Vert1 == e2->Vert1 || e1->Vert1 == e2->Vert2 || e1->Vert2 == e2->Vert1 || e1->Vert2 == e2->Vert2 );
}

BMVert *BM_edge_share_vert ( BMEdge *e1 , BMEdge *e2 ) {
	LIB_assert ( e1 != e2 );
	if ( BM_vert_in_edge ( e2 , e1->Vert1 ) ) {
		return e1->Vert1;
	}
	if ( BM_vert_in_edge ( e2 , e1->Vert2 ) ) {
		return e1->Vert2;
	}
	return NULL;
}

BMLoop *BM_edge_vert_share_loop ( BMLoop *l , BMVert *v ) {
	LIB_assert ( BM_vert_in_edge ( l->Edge , v ) );
	if ( l->Vert == v ) {
		return l;
	}
	return l->Next;
}

BMLoop *BM_face_vert_share_loop ( BMFace *f , BMVert *v ) {
	BMLoop *l_first;
	BMLoop *l_iter;

	l_iter = l_first = BM_FACE_FIRST_LOOP ( f );
	do {
		if ( l_iter->Vert == v ) {
			return l_iter;
		}
	} while ( ( l_iter = l_iter->Next ) != l_first );

	return NULL;
}

BMLoop *BM_face_edge_share_loop ( BMFace *f , BMEdge *e ) {
	BMLoop *l_first;
	BMLoop *l_iter;

	l_iter = l_first = e->Loop;
	do {
		if ( l_iter->Face == f ) {
			return l_iter;
		}
	} while ( ( l_iter = l_iter->RadialNext ) != l_first );

	return NULL;
}

void BM_edge_ordered_verts_ex ( const BMEdge *edge ,
				BMVert **r_v1 ,
				BMVert **r_v2 ,
				const BMLoop *edge_loop ) {
	LIB_assert ( edge_loop->Edge == edge );
	( void ) edge; /* quiet warning in release build */
	*r_v1 = edge_loop->Vert;
	*r_v2 = edge_loop->Next->Vert;
}

void BM_edge_ordered_verts ( const BMEdge *edge , BMVert **r_v1 , BMVert **r_v2 ) {
	BM_edge_ordered_verts_ex ( edge , r_v1 , r_v2 , edge->Loop );
}

BMLoop *BM_loop_find_prev_nodouble ( BMLoop *l , BMLoop *l_stop , const float eps_sq ) {
	BMLoop *l_step = l->Prev;

	LIB_assert ( !ELEM ( l_stop , NULL , l ) );

	while ( UNLIKELY ( len_squared_v3v3 ( l->Vert->Coord , l_step->Vert->Coord ) < eps_sq ) ) {
		l_step = l_step->Prev;
		LIB_assert ( l_step != l );
		if ( UNLIKELY ( l_step == l_stop ) ) {
			return NULL;
		}
	}

	return l_step;
}

BMLoop *BM_loop_find_next_nodouble ( BMLoop *l , BMLoop *l_stop , const float eps_sq ) {
	BMLoop *l_step = l->Next;

	LIB_assert ( !ELEM ( l_stop , NULL , l ) );

	while ( UNLIKELY ( len_squared_v3v3 ( l->Vert->Coord , l_step->Vert->Coord ) < eps_sq ) ) {
		l_step = l_step->Next;
		LIB_assert ( l_step != l );
		if ( UNLIKELY ( l_step == l_stop ) ) {
			return NULL;
		}
	}

	return l_step;
}

bool BM_loop_is_convex ( const BMLoop *l ) {
	float e_dir_prev [ 3 ];
	float e_dir_next [ 3 ];
	float l_no [ 3 ];

	sub_v3_v3v3 ( e_dir_prev , l->Prev->Vert->Coord , l->Vert->Coord );
	sub_v3_v3v3 ( e_dir_next , l->Next->Vert->Coord , l->Vert->Coord );
	cross_v3_v3v3 ( l_no , e_dir_next , e_dir_prev );
	return dot_v3v3 ( l_no , l->Face->Normal ) > 0.0f;
}

float BM_loop_calc_face_angle ( const BMLoop *l ) {
	return angle_v3v3v3 ( l->Prev->Vert->Coord , l->Vert->Coord , l->Next->Vert->Coord );
}

float BM_loop_calc_face_normal_safe_ex ( const BMLoop *l , const float epsilon_sq , float r_normal [ 3 ] ) {
	/* NOTE: we cannot use result of normal_tri_v3 here to detect colinear vectors
	 * (vertex on a straight line) from zero value,
	 * because it does not normalize both vectors before making cross-product.
	 * Instead of adding two costly normalize computations,
	 * just check ourselves for colinear case. */
	 /* NOTE: FEPSILON might need some finer tweaking at some point?
	  * Seems to be working OK for now though. */
	float v1 [ 3 ] , v2 [ 3 ] , v_tmp [ 3 ];
	sub_v3_v3v3 ( v1 , l->Prev->Vert->Coord , l->Vert->Coord );
	sub_v3_v3v3 ( v2 , l->Next->Vert->Coord , l->Vert->Coord );

	const float fac = ( ( v2 [ 0 ] == 0.0f ) ?
			    ( ( v2 [ 1 ] == 0.0f ) ? ( ( v2 [ 2 ] == 0.0f ) ? 0.0f : v1 [ 2 ] / v2 [ 2 ] ) :
			      v1 [ 1 ] / v2 [ 1 ] ) :
			    v1 [ 0 ] / v2 [ 0 ] );

	mul_v3_v3fl ( v_tmp , v2 , fac );
	sub_v3_v3 ( v_tmp , v1 );
	if ( fac != 0.0f && !is_zero_v3 ( v1 ) && len_squared_v3 ( v_tmp ) > epsilon_sq ) {
		/* Not co-linear, we can compute cross-product and normalize it into normal. */
		cross_v3_v3v3 ( r_normal , v1 , v2 );
		return normalize_v3 ( r_normal );
	}
	copy_v3_v3 ( r_normal , l->Face->Normal );
	return 0.0f;
}

float BM_loop_calc_face_normal_safe_vcos_ex ( const BMLoop *l ,
					      const float normal_fallback [ 3 ] ,
					      float const ( *vertexCos ) [ 3 ] ,
					      const float epsilon_sq ,
					      float r_normal [ 3 ] ) {
	const int i_prev = BM_elem_index_get ( l->Prev->Vert );
	const int i_next = BM_elem_index_get ( l->Next->Vert );
	const int i = BM_elem_index_get ( l->Vert );

	float v1 [ 3 ] , v2 [ 3 ] , v_tmp [ 3 ];
	sub_v3_v3v3 ( v1 , vertexCos [ i_prev ] , vertexCos [ i ] );
	sub_v3_v3v3 ( v2 , vertexCos [ i_next ] , vertexCos [ i ] );

	const float fac = ( ( v2 [ 0 ] == 0.0f ) ?
			    ( ( v2 [ 1 ] == 0.0f ) ? ( ( v2 [ 2 ] == 0.0f ) ? 0.0f : v1 [ 2 ] / v2 [ 2 ] ) :
			      v1 [ 1 ] / v2 [ 1 ] ) :
			    v1 [ 0 ] / v2 [ 0 ] );

	mul_v3_v3fl ( v_tmp , v2 , fac );
	sub_v3_v3 ( v_tmp , v1 );
	if ( fac != 0.0f && !is_zero_v3 ( v1 ) && len_squared_v3 ( v_tmp ) > epsilon_sq ) {
		/* Not co-linear, we can compute cross-product and normalize it into normal. */
		cross_v3_v3v3 ( r_normal , v1 , v2 );
		return normalize_v3 ( r_normal );
	}
	copy_v3_v3 ( r_normal , normal_fallback );
	return 0.0f;
}

float BM_loop_calc_face_normal_safe ( const BMLoop *l , float r_normal [ 3 ] ) {
	return BM_loop_calc_face_normal_safe_ex ( l , 1e-5f , r_normal );
}

float BM_loop_calc_face_normal_safe_vcos ( const BMLoop *l ,
					   const float normal_fallback [ 3 ] ,
					   float const ( *vertexCos ) [ 3 ] ,
					   float r_normal [ 3 ] )

{
	return BM_loop_calc_face_normal_safe_vcos_ex ( l , normal_fallback , vertexCos , 1e-5f , r_normal );
}

float BM_loop_calc_face_normal ( const BMLoop *l , float r_normal [ 3 ] ) {
	float v1 [ 3 ] , v2 [ 3 ];
	sub_v3_v3v3 ( v1 , l->Prev->Vert->Coord , l->Vert->Coord );
	sub_v3_v3v3 ( v2 , l->Next->Vert->Coord , l->Vert->Coord );

	cross_v3_v3v3 ( r_normal , v1 , v2 );
	const float len = normalize_v3 ( r_normal );
	if ( UNLIKELY ( len == 0.0f ) ) {
		copy_v3_v3 ( r_normal , l->Face->Normal );
	}
	return len;
}

void BM_loop_calc_face_direction ( const BMLoop *l , float r_dir [ 3 ] ) {
	float v_prev [ 3 ];
	float v_next [ 3 ];

	sub_v3_v3v3 ( v_prev , l->Vert->Coord , l->Prev->Vert->Coord );
	sub_v3_v3v3 ( v_next , l->Next->Vert->Coord , l->Vert->Coord );

	normalize_v3 ( v_prev );
	normalize_v3 ( v_next );

	add_v3_v3v3 ( r_dir , v_prev , v_next );
	normalize_v3 ( r_dir );
}

void BM_loop_calc_face_tangent ( const BMLoop *l , float r_tangent [ 3 ] ) {
	float v_prev [ 3 ];
	float v_next [ 3 ];
	float dir [ 3 ];

	sub_v3_v3v3 ( v_prev , l->Prev->Vert->Coord , l->Vert->Coord );
	sub_v3_v3v3 ( v_next , l->Vert->Coord , l->Next->Vert->Coord );

	normalize_v3 ( v_prev );
	normalize_v3 ( v_next );
	add_v3_v3v3 ( dir , v_prev , v_next );

	if ( compare_v3v3 ( v_prev , v_next , FLT_EPSILON * 10.0f ) == false ) {
		float nor [ 3 ]; /* for this purpose doesn't need to be normalized */
		cross_v3_v3v3 ( nor , v_prev , v_next );
		/* concave face check */
		if ( UNLIKELY ( dot_v3v3 ( nor , l->Face->Normal ) < 0.0f ) ) {
			negate_v3 ( nor );
		}
		cross_v3_v3v3 ( r_tangent , dir , nor );
	} else {
		/* prev/next are the same - compare with face normal since we don't have one */
		cross_v3_v3v3 ( r_tangent , dir , l->Face->Normal );
	}

	normalize_v3 ( r_tangent );
}

float BM_edge_calc_face_angle_ex ( const BMEdge *e , const float fallback ) {
	if ( BM_edge_is_manifold ( e ) ) {
		const BMLoop *l1 = e->Loop;
		const BMLoop *l2 = e->Loop->RadialNext;
		return angle_normalized_v3v3 ( l1->Face->Normal , l2->Face->Normal );
	}
	return fallback;
}
float BM_edge_calc_face_angle ( const BMEdge *e ) {
	return BM_edge_calc_face_angle_ex ( e , DEG2RADF ( 90.0f ) );
}

float BM_edge_calc_face_angle_with_imat3_ex ( const BMEdge *e ,
					      const float imat3 [ 3 ][ 3 ] ,
					      const float fallback ) {
	if ( BM_edge_is_manifold ( e ) ) {
		const BMLoop *l1 = e->Loop;
		const BMLoop *l2 = e->Loop->RadialNext;
		float no1 [ 3 ] , no2 [ 3 ];
		copy_v3_v3 ( no1 , l1->Face->Normal );
		copy_v3_v3 ( no2 , l2->Face->Normal );

		mul_transposed_m3_v3 ( imat3 , no1 );
		mul_transposed_m3_v3 ( imat3 , no2 );

		normalize_v3 ( no1 );
		normalize_v3 ( no2 );

		return angle_normalized_v3v3 ( no1 , no2 );
	}
	return fallback;
}
float BM_edge_calc_face_angle_with_imat3 ( const BMEdge *e , const float imat3 [ 3 ][ 3 ] ) {
	return BM_edge_calc_face_angle_with_imat3_ex ( e , imat3 , DEG2RADF ( 90.0f ) );
}

float BM_edge_calc_face_angle_signed_ex ( const BMEdge *e , const float fallback ) {
	if ( BM_edge_is_manifold ( e ) ) {
		BMLoop *l1 = e->Loop;
		BMLoop *l2 = e->Loop->RadialNext;
		const float angle = angle_normalized_v3v3 ( l1->Face->Normal , l2->Face->Normal );
		return BM_edge_is_convex ( e ) ? angle : -angle;
	}
	return fallback;
}
float BM_edge_calc_face_angle_signed ( const BMEdge *e ) {
	return BM_edge_calc_face_angle_signed_ex ( e , DEG2RADF ( 90.0f ) );
}

void BM_edge_calc_face_tangent ( const BMEdge *e , const BMLoop *e_loop , float r_tangent [ 3 ] ) {
	float tvec [ 3 ];
	BMVert *v1 , *v2;
	BM_edge_ordered_verts_ex ( e , &v1 , &v2 , e_loop );

	sub_v3_v3v3 ( tvec , v1->Coord , v2->Coord ); /* use for temp storage */
	/* NOTE: we could average the tangents of both loops,
	 * for non flat ngons it will give a better direction */
	cross_v3_v3v3 ( r_tangent , tvec , e_loop->Face->Normal );
	normalize_v3 ( r_tangent );
}

float BM_vert_calc_edge_angle_ex ( const BMVert *v , const float fallback ) {
	BMEdge *e1 , *e2;

	/* Saves `BM_vert_edge_count(v)` and edge iterator,
	 * get the edges and count them both at once. */

	if ( ( e1 = v->Edge ) && ( e2 = bmesh_disk_edge_next ( e1 , v ) ) && ( e1 != e2 ) &&
	     /* make sure we come full circle and only have 2 connected edges */
	     ( e1 == bmesh_disk_edge_next ( e2 , v ) ) ) {
		BMVert *v1 = BM_edge_other_vert ( e1 , v );
		BMVert *v2 = BM_edge_other_vert ( e2 , v );

		return ( float ) M_PI - angle_v3v3v3 ( v1->Coord , v->Coord , v2->Coord );
	}
	return fallback;
}

float BM_vert_calc_edge_angle ( const BMVert *v ) {
	return BM_vert_calc_edge_angle_ex ( v , DEG2RADF ( 90.0f ) );
}

float BM_vert_calc_shell_factor ( const BMVert *v ) {
	BMIter iter;
	BMLoop *l;
	float accum_shell = 0.0f;
	float accum_angle = 0.0f;

	BM_ITER_ELEM ( l , &iter , ( BMVert * ) v , BM_LOOPS_OF_VERT ) {
		const float face_angle = BM_loop_calc_face_angle ( l );
		accum_shell += shell_v3v3_normalized_to_dist ( v->Normal , l->Face->Normal ) * face_angle;
		accum_angle += face_angle;
	}

	if ( accum_angle != 0.0f ) {
		return accum_shell / accum_angle;
	}
	return 1.0f;
}
float BM_vert_calc_shell_factor_ex ( const BMVert *v , const float no [ 3 ] , const char hflag ) {
	BMIter iter;
	const BMLoop *l;
	float accum_shell = 0.0f;
	float accum_angle = 0.0f;
	int tot_sel = 0 , tot = 0;

	BM_ITER_ELEM ( l , &iter , ( BMVert * ) v , BM_LOOPS_OF_VERT ) {
		if ( BM_elem_flag_test ( l->Face , hflag ) ) { /* <-- main difference to BM_vert_calc_shell_factor! */
			const float face_angle = BM_loop_calc_face_angle ( l );
			accum_shell += shell_v3v3_normalized_to_dist ( no , l->Face->Normal ) * face_angle;
			accum_angle += face_angle;
			tot_sel++;
		}
		tot++;
	}

	if ( accum_angle != 0.0f ) {
		return accum_shell / accum_angle;
	}
	/* other main difference from BM_vert_calc_shell_factor! */
	if ( tot != 0 && tot_sel == 0 ) {
		/* none selected, so use all */
		return BM_vert_calc_shell_factor ( v );
	}
	return 1.0f;
}

float BM_vert_calc_median_tagged_edge_length ( const BMVert *v ) {
	BMIter iter;
	BMEdge *e;
	int tot;
	float length = 0.0f;

	BM_ITER_ELEM_INDEX ( e , &iter , ( BMVert * ) v , BM_EDGES_OF_VERT , tot ) {
		const BMVert *v_other = BM_edge_other_vert ( e , v );
		if ( BM_elem_flag_test ( v_other , BM_ELEM_TAG ) ) {
			length += BM_edge_calc_length ( e );
		}
	}

	if ( tot ) {
		return length / ( float ) tot;
	}
	return 0.0f;
}

BMLoop *BM_face_find_shortest_loop ( BMFace *f ) {
	BMLoop *shortest_loop = NULL;
	float shortest_len = FLT_MAX;

	BMLoop *l_iter;
	BMLoop *l_first;

	l_iter = l_first = BM_FACE_FIRST_LOOP ( f );

	do {
		const float len_sq = len_squared_v3v3 ( l_iter->Vert->Coord , l_iter->Next->Vert->Coord );
		if ( len_sq <= shortest_len ) {
			shortest_loop = l_iter;
			shortest_len = len_sq;
		}
	} while ( ( l_iter = l_iter->Next ) != l_first );

	return shortest_loop;
}

BMLoop *BM_face_find_longest_loop ( BMFace *f ) {
	BMLoop *longest_loop = NULL;
	float len_max_sq = 0.0f;

	BMLoop *l_iter;
	BMLoop *l_first;

	l_iter = l_first = BM_FACE_FIRST_LOOP ( f );

	do {
		const float len_sq = len_squared_v3v3 ( l_iter->Vert->Coord , l_iter->Next->Vert->Coord );
		if ( len_sq >= len_max_sq ) {
			longest_loop = l_iter;
			len_max_sq = len_sq;
		}
	} while ( ( l_iter = l_iter->Next ) != l_first );

	return longest_loop;
}

/**
 * Returns the edge existing between \a v_a and \a v_b, or NULL if there isn't one.
 *
 * \note multiple edges may exist between any two vertices, and therefore
 * this function only returns the first one found.
 */
#if 0
BMEdge *BM_edge_exists ( BMVert *v_a , BMVert *v_b ) {
	BMIter iter;
	BMEdge *e;

	LIB_assert ( v_a != v_b );
	LIB_assert ( v_a->Head.ElemType == BM_VERT && v_b->Head.ElemType == BM_VERT );

	BM_ITER_ELEM ( e , &iter , v_a , BM_EDGES_OF_VERT ) {
		if ( e->Vert1 == v_b || e->Vert2 == v_b ) {
			return e;
		}
	}

	return NULL;
}
#else
BMEdge *BM_edge_exists ( BMVert *v_a , BMVert *v_b ) {
	/* speedup by looping over both edges verts
	 * where one vert may connect to many edges but not the other. */

	BMEdge *e_a , *e_b;

	LIB_assert ( v_a != v_b );
	LIB_assert ( v_a->Head.ElemType == BM_VERT && v_b->Head.ElemType == BM_VERT );

	if ( ( e_a = v_a->Edge ) && ( e_b = v_b->Edge ) ) {
		BMEdge *e_a_iter = e_a , *e_b_iter = e_b;

		do {
			if ( BM_vert_in_edge ( e_a_iter , v_b ) ) {
				return e_a_iter;
			}
			if ( BM_vert_in_edge ( e_b_iter , v_a ) ) {
				return e_b_iter;
			}
		} while ( ( ( e_a_iter = bmesh_disk_edge_next ( e_a_iter , v_a ) ) != e_a ) &&
			  ( ( e_b_iter = bmesh_disk_edge_next ( e_b_iter , v_b ) ) != e_b ) );
	}

	return NULL;
}
#endif

BMEdge *BM_edge_find_double ( BMEdge *e ) {
	BMVert *v = e->Vert1;
	BMVert *v_other = e->Vert2;

	BMEdge *e_iter;

	e_iter = e;
	while ( ( e_iter = bmesh_disk_edge_next ( e_iter , v ) ) != e ) {
		if ( UNLIKELY ( BM_vert_in_edge ( e_iter , v_other ) ) ) {
			return e_iter;
		}
	}

	return NULL;
}

BMLoop *BM_edge_find_first_loop_visible ( BMEdge *e ) {
	if ( e->Loop != NULL ) {
		BMLoop *l_iter , *l_first;
		l_iter = l_first = e->Loop;
		do {
			if ( !BM_elem_flag_test ( l_iter->Face , BM_ELEM_HIDDEN ) ) {
				return l_iter;
			}
		} while ( ( l_iter = l_iter->RadialNext ) != l_first );
	}
	return NULL;
}

BMFace *BM_face_exists ( BMVert **varr , int len ) {
	if ( varr [ 0 ]->Edge ) {
		BMEdge *e_iter , *e_first;
		e_iter = e_first = varr [ 0 ]->Edge;

		/* would normally use BM_LOOPS_OF_VERT, but this runs so often,
		 * its faster to iterate on the data directly */
		do {
			if ( e_iter->Loop ) {
				BMLoop *l_iter_radial , *l_first_radial;
				l_iter_radial = l_first_radial = e_iter->Loop;

				do {
					if ( ( l_iter_radial->Vert == varr [ 0 ] ) && ( l_iter_radial->Face->Len == len ) ) {
						/* the fist 2 verts match, now check the remaining (len - 2) faces do too
						 * winding isn't known, so check in both directions */
						int i_walk = 2;

						if ( l_iter_radial->Next->Vert == varr [ 1 ] ) {
							BMLoop *l_walk = l_iter_radial->Next->Next;
							do {
								if ( l_walk->Vert != varr [ i_walk ] ) {
									break;
								}
							} while ( ( void ) ( l_walk = l_walk->Next ) , ++i_walk != len );
						} else if ( l_iter_radial->Prev->Vert == varr [ 1 ] ) {
							BMLoop *l_walk = l_iter_radial->Prev->Prev;
							do {
								if ( l_walk->Vert != varr [ i_walk ] ) {
									break;
								}
							} while ( ( void ) ( l_walk = l_walk->Prev ) , ++i_walk != len );
						}

						if ( i_walk == len ) {
							return l_iter_radial->Face;
						}
					}
				} while ( ( l_iter_radial = l_iter_radial->RadialNext ) != l_first_radial );
			}
		} while ( ( e_iter = BM_DISK_EDGE_NEXT ( e_iter , varr [ 0 ] ) ) != e_first );
	}

	return NULL;
}

BMFace *BM_face_find_double ( BMFace *f ) {
	BMLoop *l_first = BM_FACE_FIRST_LOOP ( f );
	for ( BMLoop *l_iter = l_first->RadialNext; l_first != l_iter; l_iter = l_iter->RadialNext ) {
		if ( l_iter->Face->Len == l_first->Face->Len ) {
			if ( l_iter->Vert == l_first->Vert ) {
				BMLoop *l_a = l_first , *l_b = l_iter , *l_b_init = l_iter;
				do {
					if ( l_a->Edge != l_b->Edge ) {
						break;
					}
				} while ( ( ( void ) ( l_a = l_a->Next ) , ( l_b = l_b->Next ) ) != l_b_init );
				if ( l_b == l_b_init ) {
					return l_iter->Face;
				}
			} else {
				BMLoop *l_a = l_first , *l_b = l_iter , *l_b_init = l_iter;
				do {
					if ( l_a->Edge != l_b->Edge ) {
						break;
					}
				} while ( ( ( void ) ( l_a = l_a->Prev ) , ( l_b = l_b->Next ) ) != l_b_init );
				if ( l_b == l_b_init ) {
					return l_iter->Face;
				}
			}
		}
	}
	return NULL;
}

bool BM_face_exists_multi ( BMVert **varr , BMEdge **earr , int len ) {
	BMFace *f;
	BMEdge *e;
	BMVert *v;
	bool ok;
	int tot_tag;

	BMIter fiter;
	BMIter viter;

	int i;

	for ( i = 0; i < len; i++ ) {
		/* save some time by looping over edge faces rather than vert faces
		 * will still loop over some faces twice but not as many */
		BM_ITER_ELEM ( f , &fiter , earr [ i ] , BM_FACES_OF_EDGE ) {
			BM_elem_flag_disable ( f , BM_ELEM_INTERNAL_TAG );
			BM_ITER_ELEM ( v , &viter , f , BM_VERTS_OF_FACE ) {
				BM_elem_flag_disable ( v , BM_ELEM_INTERNAL_TAG );
			}
		}

		/* clear all edge tags */
		BM_ITER_ELEM ( e , &fiter , varr [ i ] , BM_EDGES_OF_VERT ) {
			BM_elem_flag_disable ( e , BM_ELEM_INTERNAL_TAG );
		}
	}

	/* now tag all verts and edges in the boundary array as true so
	 * we can know if a face-vert is from our array */
	for ( i = 0; i < len; i++ ) {
		BM_elem_flag_enable ( varr [ i ] , BM_ELEM_INTERNAL_TAG );
		BM_elem_flag_enable ( earr [ i ] , BM_ELEM_INTERNAL_TAG );
	}

	/* so! boundary is tagged, everything else cleared */

	/* 1) tag all faces connected to edges - if all their verts are boundary */
	tot_tag = 0;
	for ( i = 0; i < len; i++ ) {
		BM_ITER_ELEM ( f , &fiter , earr [ i ] , BM_FACES_OF_EDGE ) {
			if ( !BM_elem_flag_test ( f , BM_ELEM_INTERNAL_TAG ) ) {
				ok = true;
				BM_ITER_ELEM ( v , &viter , f , BM_VERTS_OF_FACE ) {
					if ( !BM_elem_flag_test ( v , BM_ELEM_INTERNAL_TAG ) ) {
						ok = false;
						break;
					}
				}

				if ( ok ) {
					/* we only use boundary verts */
					BM_elem_flag_enable ( f , BM_ELEM_INTERNAL_TAG );
					tot_tag++;
				}
			} else {
				/* we already found! */
			}
		}
	}

	if ( tot_tag == 0 ) {
		/* no faces use only boundary verts, quit early */
		ok = false;
		goto finally;
	}

	/* 2) loop over non-boundary edges that use boundary verts,
	 *    check each have 2 tagged faces connected (faces that only use 'varr' verts) */
	ok = true;
	for ( i = 0; i < len; i++ ) {
		BM_ITER_ELEM ( e , &fiter , varr [ i ] , BM_EDGES_OF_VERT ) {

			if (/* non-boundary edge */
			     BM_elem_flag_test ( e , BM_ELEM_INTERNAL_TAG ) == false &&
			     /* ...using boundary verts */
			     BM_elem_flag_test ( e->Vert1 , BM_ELEM_INTERNAL_TAG ) &&
			     BM_elem_flag_test ( e->Vert2 , BM_ELEM_INTERNAL_TAG ) ) {
				int tot_face_tag = 0;
				BM_ITER_ELEM ( f , &fiter , e , BM_FACES_OF_EDGE ) {
					if ( BM_elem_flag_test ( f , BM_ELEM_INTERNAL_TAG ) ) {
						tot_face_tag++;
					}
				}

				if ( tot_face_tag != 2 ) {
					ok = false;
					break;
				}
			}
		}

		if ( ok == false ) {
			break;
		}
	}

	finally:
	/* Cleanup */
	for ( i = 0; i < len; i++ ) {
		BM_elem_flag_disable ( varr [ i ] , BM_ELEM_INTERNAL_TAG );
		BM_elem_flag_disable ( earr [ i ] , BM_ELEM_INTERNAL_TAG );
	}
	return ok;
}

bool BM_face_exists_multi_edge ( BMEdge **earr , int len ) {
	BMVert **varr = ( BMVert ** ) alloca ( sizeof ( *varr ) * len );

	// first check if verts have edges, if not we can bail out early
	if ( !BM_verts_from_edges ( varr , earr , len ) ) {
		LIB_assert_unreachable ( );
		return false;
	}

	return BM_face_exists_multi ( varr , earr , len );
}

bool BM_vert_is_all_edge_flag_test ( const BMVert *v , const char hflag , const bool respect_hide ) {
	if ( v->Edge ) {
		BMEdge *e_other;
		BMIter eiter;

		BM_ITER_ELEM ( e_other , &eiter , ( BMVert * ) v , BM_EDGES_OF_VERT ) {
			if ( !respect_hide || !BM_elem_flag_test ( e_other , BM_ELEM_HIDDEN ) ) {
				if ( !BM_elem_flag_test ( e_other , hflag ) ) {
					return false;
				}
			}
		}
	}

	return true;
}

bool BM_vert_is_all_face_flag_test ( const BMVert *v , const char hflag , const bool respect_hide ) {
	if ( v->Edge ) {
		BMEdge *f_other;
		BMIter fiter;

		BM_ITER_ELEM ( f_other , &fiter , ( BMVert * ) v , BM_FACES_OF_VERT ) {
			if ( !respect_hide || !BM_elem_flag_test ( f_other , BM_ELEM_HIDDEN ) ) {
				if ( !BM_elem_flag_test ( f_other , hflag ) ) {
					return false;
				}
			}
		}
	}

	return true;
}

bool BM_edge_is_all_face_flag_test ( const BMEdge *e , const char hflag , const bool respect_hide ) {
	if ( e->Loop ) {
		BMLoop *l_iter , *l_first;

		l_iter = l_first = e->Loop;
		do {
			if ( !respect_hide || !BM_elem_flag_test ( l_iter->Face , BM_ELEM_HIDDEN ) ) {
				if ( !BM_elem_flag_test ( l_iter->Face , hflag ) ) {
					return false;
				}
			}
		} while ( ( l_iter = l_iter->RadialNext ) != l_first );
	}

	return true;
}

bool BM_edge_is_any_face_flag_test ( const BMEdge *e , const char hflag ) {
	if ( e->Loop ) {
		BMLoop *l_iter , *l_first;

		l_iter = l_first = e->Loop;
		do {
			if ( BM_elem_flag_test ( l_iter->Face , hflag ) ) {
				return true;
			}
		} while ( ( l_iter = l_iter->RadialNext ) != l_first );
	}

	return false;
}

bool BM_edge_is_any_vert_flag_test ( const BMEdge *e , const char hflag ) {
	return ( BM_elem_flag_test ( e->Vert1 , hflag ) || BM_elem_flag_test ( e->Vert2 , hflag ) );
}

bool BM_face_is_any_vert_flag_test ( const BMFace *f , const char hflag ) {
	BMLoop *l_iter;
	BMLoop *l_first;

	l_iter = l_first = BM_FACE_FIRST_LOOP ( f );
	do {
		if ( BM_elem_flag_test ( l_iter->Vert , hflag ) ) {
			return true;
		}
	} while ( ( l_iter = l_iter->Next ) != l_first );
	return false;
}

bool BM_face_is_any_edge_flag_test ( const BMFace *f , const char hflag ) {
	BMLoop *l_iter;
	BMLoop *l_first;

	l_iter = l_first = BM_FACE_FIRST_LOOP ( f );
	do {
		if ( BM_elem_flag_test ( l_iter->Edge , hflag ) ) {
			return true;
		}
	} while ( ( l_iter = l_iter->Next ) != l_first );
	return false;
}

bool BM_edge_is_any_face_len_test ( const BMEdge *e , const int len ) {
	if ( e->Loop ) {
		BMLoop *l_iter , *l_first;

		l_iter = l_first = e->Loop;
		do {
			if ( l_iter->Face->Len == len ) {
				return true;
			}
		} while ( ( l_iter = l_iter->RadialNext ) != l_first );
	}

	return false;
}
