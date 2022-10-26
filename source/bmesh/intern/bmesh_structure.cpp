#include "bmesh_structure.h"
#include "bmesh_inline.h"
#include "bmesh_private.h"

void bmesh_disk_edge_append ( BMEdge *e , BMVert *v ) {
	if ( !v->Edge ) {
		BMDiskLink *dl1 = bmesh_disk_edge_link_from_vert ( e , v );

		v->Edge = e;
		dl1->Next = dl1->Prev = e;
	} else {
		BMDiskLink *dl1 , *dl2 , *dl3;

		dl1 = bmesh_disk_edge_link_from_vert ( e , v );
		dl2 = bmesh_disk_edge_link_from_vert ( v->Edge , v );
		dl3 = dl2->Prev ? bmesh_disk_edge_link_from_vert ( dl2->Prev , v ) : NULL;

		dl1->Next = v->Edge;
		dl1->Prev = dl2->Prev;

		dl2->Prev = e;
		if ( dl3 ) {
			dl3->Next = e;
		}
	}
}

void bmesh_disk_edge_remove ( BMEdge *e , BMVert *v ) {
	BMDiskLink *dl1 , *dl2;

	dl1 = bmesh_disk_edge_link_from_vert ( e , v );
	if ( dl1->Prev ) {
		dl2 = bmesh_disk_edge_link_from_vert ( dl1->Prev , v );
		dl2->Next = dl1->Next;
	}

	if ( dl1->Next ) {
		dl2 = bmesh_disk_edge_link_from_vert ( dl1->Next , v );
		dl2->Prev = dl1->Prev;
	}

	if ( v->Edge == e ) {
		v->Edge = ( e != dl1->Next ) ? dl1->Next : NULL;
	}

	dl1->Next = dl1->Prev = NULL;
}

int bmesh_disk_facevert_count ( const BMVert *v ) {
	int count = 0;
	if ( v->Edge ) {
		BMEdge *e_first , *e_iter;

		/* first, loop around edge */
		e_first = e_iter = v->Edge;
		do {
			if ( e_iter->Loop ) {
				count += bmesh_radial_facevert_count ( e_iter->Loop , v );
			}
		} while ( ( e_iter = bmesh_disk_edge_next ( e_iter , v ) ) != e_first );
	}
	return count;
}

int bmesh_disk_facevert_count_at_most ( const BMVert *v , const int count_max ) {
	/* is there an edge on this vert at all */
	int count = 0;
	if ( v->Edge ) {
		BMEdge *e_first , *e_iter;

		/* first, loop around edge */
		e_first = e_iter = v->Edge;
		do {
			if ( e_iter->Loop ) {
				count += bmesh_radial_facevert_count_at_most ( e_iter->Loop , v , count_max - count );
				if ( count == count_max ) {
					break;
				}
			}
		} while ( ( e_iter = bmesh_disk_edge_next ( e_iter , v ) ) != e_first );
	}
	return count;
}

BMEdge *bmesh_disk_faceedge_find_first ( const BMEdge *e , const BMVert *v ) {
	const BMEdge *e_iter = e;
	do {
		if ( e_iter->Loop != NULL ) {
			return ( BMEdge * ) ( ( e_iter->Loop->Vert == v ) ? e_iter : e_iter->Loop->Next->Edge );
		}
	} while ( ( e_iter = bmesh_disk_edge_next ( e_iter , v ) ) != e );
	return NULL;
}

BMLoop *bmesh_disk_faceloop_find_first ( const BMEdge *e , const BMVert *v ) {
	const BMEdge *e_iter = e;
	do {
		if ( e_iter->Loop != NULL ) {
			return ( e_iter->Loop->Vert == v ) ? e_iter->Loop : e_iter->Loop->Next;
		}
	} while ( ( e_iter = bmesh_disk_edge_next ( e_iter , v ) ) != e );
	return NULL;
}

BMLoop *bmesh_disk_faceloop_find_first_visible ( const BMEdge *e , const BMVert *v ) {
	const BMEdge *e_iter = e;
	do {
		if ( !BM_elem_flag_test ( e_iter , BM_ELEM_HIDDEN ) ) {
			if ( e_iter->Loop != NULL ) {
				BMLoop *l_iter , *l_first;
				l_iter = l_first = e_iter->Loop;
				do {
					if ( !BM_elem_flag_test ( l_iter->Face , BM_ELEM_HIDDEN ) ) {
						return ( l_iter->Vert == v ) ? l_iter : l_iter->Next;
					}
				} while ( ( l_iter = l_iter->RadialNext ) != l_first );
			}
		}
	} while ( ( e_iter = bmesh_disk_edge_next ( e_iter , v ) ) != e );
	return NULL;
}

BMEdge *bmesh_disk_faceedge_find_next ( const BMEdge *e , const BMVert *v ) {
	BMEdge *e_find;
	e_find = bmesh_disk_edge_next ( e , v );
	do {
		if ( e_find->Loop && bmesh_radial_facevert_check ( e_find->Loop , v ) ) {
			return e_find;
		}
	} while ( ( e_find = bmesh_disk_edge_next ( e_find , v ) ) != e );
	return ( BMEdge * ) e;
}

BMLoop *bmesh_radial_faceloop_find_first ( const BMLoop *l , const BMVert *v ) {
	const BMLoop *l_iter;
	l_iter = l;
	do {
		if ( l_iter->Vert == v ) {
			return ( BMLoop * ) l_iter;
		}
	} while ( ( l_iter = l_iter->RadialNext ) != l );
	return NULL;
}

BMLoop *bmesh_radial_faceloop_find_next ( const BMLoop *l , const BMVert *v ) {
	BMLoop *l_iter;
	l_iter = l->RadialNext;
	do {
		if ( l_iter->Vert == v ) {
			return l_iter;
		}
	} while ( ( l_iter = l_iter->RadialNext ) != l );
	return ( BMLoop * ) l;
}

int bmesh_radial_facevert_count_at_most ( const BMLoop *l , const BMVert *v , const int count_max ) {
	const BMLoop *l_iter;
	int count = 0;
	l_iter = l;
	do {
		if ( l_iter->Vert == v ) {
			count++;
			if ( count == count_max ) {
				break;
			}
		}
	} while ( ( l_iter = l_iter->RadialNext ) != l );

	return count;
}

int bmesh_radial_facevert_count ( const BMLoop *l , const BMVert *v ) {
	const BMLoop *l_iter;
	int count = 0;
	l_iter = l;
	do {
		if ( l_iter->Vert == v ) {
			count++;
		}
	} while ( ( l_iter = l_iter->RadialNext ) != l );

	return count;
}

bool bmesh_radial_facevert_check ( const BMLoop *l , const BMVert *v ) {
	const BMLoop *l_iter;
	l_iter = l;
	do {
		if ( l_iter->Vert == v ) {
			return true;
		}
	} while ( ( l_iter = l_iter->RadialNext ) != l );

	return false;
}

int bmesh_radial_length ( const BMLoop *l ) {
	const BMLoop *l_iter = l;
	int i = 0;

	if ( !l ) {
		return 0;
	}

	do {
		if ( UNLIKELY ( !l_iter ) ) {
			/* Radial cycle is broken (not a circular loop). */
			LIB_assert_unreachable ( );
			return 0;
		}

		i++;
		if ( UNLIKELY ( i >= BM_LOOP_RADIAL_MAX ) ) {
			LIB_assert_unreachable ( );
			return -1;
		}
	} while ( ( l_iter = l_iter->RadialNext ) != l );

	return i;
}

int bmesh_disk_count ( const BMVert *v ) {
	int count = 0;
	if ( v->Edge ) {
		BMEdge *e_first , *e_iter;
		e_iter = e_first = v->Edge;
		do {
			count++;
		} while ( ( e_iter = bmesh_disk_edge_next ( e_iter , v ) ) != e_first );
	}
	return count;
}

int bmesh_disk_count_at_most ( const BMVert *v , const int count_max ) {
	int count = 0;
	if ( v->Edge ) {
		BMEdge *e_first , *e_iter;
		e_iter = e_first = v->Edge;
		do {
			count++;
			if ( count == count_max ) {
				break;
			}
		} while ( ( e_iter = bmesh_disk_edge_next ( e_iter , v ) ) != e_first );
	}
	return count;
}

bool bmesh_radial_validate ( int radlen , BMLoop *l ) {
	BMLoop *l_iter = l;
	int i = 0;

	if ( bmesh_radial_length ( l ) != radlen ) {
		return false;
	}

	do {
		if ( UNLIKELY ( !l_iter ) ) {
			LIB_assert_unreachable ( );
			return false;
		}

		if ( l_iter->Edge != l->Edge ) {
			return false;
		}
		if ( !ELEM ( l_iter->Vert , l->Edge->Vert1 , l->Edge->Vert2 ) ) {
			return false;
		}

		if ( UNLIKELY ( i > BM_LOOP_RADIAL_MAX ) ) {
			LIB_assert_unreachable ( );
			return false;
		}

		i++;
	} while ( ( l_iter = l_iter->RadialNext ) != l );

	return true;
}

void bmesh_radial_loop_append ( BMEdge *e , BMLoop *l ) {
	if ( e->Loop == NULL ) {
		e->Loop = l;
		l->RadialNext = l->RadialPrev = l;
	} else {
		l->RadialPrev = e->Loop;
		l->RadialNext = e->Loop->RadialNext;

		e->Loop->RadialNext->RadialPrev = l;
		e->Loop->RadialNext = l;

		e->Loop = l;
	}

	if ( UNLIKELY ( l->Edge && l->Edge != e ) ) {
		/* l is already in a radial cycle for a different edge */
		LIB_assert_unreachable ( );
	}

	l->Edge = e;
}

void bmesh_radial_loop_remove ( BMEdge *e , BMLoop *l ) {
	/* if e is non-NULL, l must be in the radial cycle of e */
	if ( UNLIKELY ( e != l->Edge ) ) {
		LIB_assert_unreachable ( );
	}

	if ( l->RadialNext != l ) {
		if ( l == e->Loop ) {
			e->Loop = l->RadialNext;
		}

		l->RadialNext->RadialPrev = l->RadialPrev;
		l->RadialPrev->RadialNext = l->RadialNext;
	} else {
		if ( l == e->Loop ) {
			e->Loop = NULL;
		} else {
			LIB_assert_unreachable ( );
		}
	}

	/* l is no longer in a radial cycle; empty the links
	 * to the cycle and the link back to an edge */
	l->RadialNext = l->RadialPrev = NULL;
	l->Edge = NULL;
}
