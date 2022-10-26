#include "bmesh_polygon.h"
#include "bmesh_inline.h"

#include "lib/lib_math.h"

static float bm_face_calc_poly_normal ( const BMFace *f , float n [ 3 ] ) {
	BMLoop *l_first = BM_FACE_FIRST_LOOP ( f );
	BMLoop *l_iter = l_first;
	const float *v_prev = l_first->Prev->Vert->Coord;
	const float *v_curr = l_first->Vert->Coord;

	zero_v3 ( n );

	// Newell's Method
	do {
		add_newell_cross_v3_v3v3 ( n , v_prev , v_curr );

		l_iter = l_iter->Next;
		v_prev = v_curr;
		v_curr = l_iter->Vert->Coord;

	} while ( l_iter != l_first );

	return normalize_v3 ( n );
}

static float bm_face_calc_poly_normal_vertex_cos ( const BMFace *f ,
						   float r_no [ 3 ] ,
						   float const ( *vertexCos ) [ 3 ] ) {
	BMLoop *l_first = BM_FACE_FIRST_LOOP ( f );
	BMLoop *l_iter = l_first;
	const float *v_prev = vertexCos [ BM_elem_index_get ( l_first->Prev->Vert ) ];
	const float *v_curr = vertexCos [ BM_elem_index_get ( l_first->Vert ) ];

	zero_v3 ( r_no );

	/* Newell's Method */
	do {
		add_newell_cross_v3_v3v3 ( r_no , v_prev , v_curr );

		l_iter = l_iter->Next;
		v_prev = v_curr;
		v_curr = vertexCos [ BM_elem_index_get ( l_iter->Vert ) ];
	} while ( l_iter != l_first );

	return normalize_v3 ( r_no );
}

float BM_face_calc_normal ( const BMFace *f , float r_no [ 3 ] ) {
	BMLoop *l;

	// Common cases first.
	switch ( f->Len ) {
		case 4: {
			const float *co1 = ( l = BM_FACE_FIRST_LOOP ( f ) )->Vert->Coord;
			const float *co2 = ( l = l->Next )->Vert->Coord;
			const float *co3 = ( l = l->Next )->Vert->Coord;
			const float *co4 = ( l->Next )->Vert->Coord;

			return normal_quad_v3 ( r_no , co1 , co2 , co3 , co4 );
		}
		case 3: {
			const float *co1 = ( l = BM_FACE_FIRST_LOOP ( f ) )->Vert->Coord;
			const float *co2 = ( l = l->Next )->Vert->Coord;
			const float *co3 = ( l->Next )->Vert->Coord;

			return normal_tri_v3 ( r_no , co1 , co2 , co3 );
		}
	}

	return bm_face_calc_poly_normal ( f , r_no );
}

void BM_face_normal_update ( BMFace *f ) {
	BM_face_calc_normal ( f , f->Normal );
}

float BM_face_calc_normal_vcos ( const BMesh *bm ,
				 const BMFace *f ,
				 float r_no [ 3 ] ,
				 float const ( *vertexCos ) [ 3 ] ) {
	BMLoop *l;

	// must have valid index data
	LIB_assert ( ( bm->ElemIndexDirty & BM_VERT ) == 0 );
	( void ) bm;

	/* common cases first */
	switch ( f->Len ) {
		case 4: {
			const float *co1 = vertexCos [ BM_elem_index_get ( ( l = BM_FACE_FIRST_LOOP ( f ) )->Vert ) ];
			const float *co2 = vertexCos [ BM_elem_index_get ( ( l = l->Next )->Vert ) ];
			const float *co3 = vertexCos [ BM_elem_index_get ( ( l = l->Next )->Vert ) ];
			const float *co4 = vertexCos [ BM_elem_index_get ( ( l->Next )->Vert ) ];

			return normal_quad_v3 ( r_no , co1 , co2 , co3 , co4 );
		}
		case 3: {
			const float *co1 = vertexCos [ BM_elem_index_get ( ( l = BM_FACE_FIRST_LOOP ( f ) )->Vert ) ];
			const float *co2 = vertexCos [ BM_elem_index_get ( ( l = l->Next )->Vert ) ];
			const float *co3 = vertexCos [ BM_elem_index_get ( ( l->Next )->Vert ) ];

			return normal_tri_v3 ( r_no , co1 , co2 , co3 );
		}
		default: {
			return bm_face_calc_poly_normal_vertex_cos ( f , r_no , vertexCos );
		}
	}
}
