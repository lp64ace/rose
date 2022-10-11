#include "bmesh_structure.h"
#include "bmesh_inline.h"

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
