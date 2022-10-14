#include "bmesh_construct.h"
#include "bmesh_query.h"
#include "bmesh_private.h"

#include "kernel/rke_customdata.h"

#include "lib/lib_math.h"

bool BM_verts_from_edges ( BMVert **vert_arr , BMEdge **edge_arr , int len ) {
	int i , i_prev = len - 1;
	for ( i = 0; i < len; i++ ) {
		vert_arr [ i ] = BM_edge_share_vert ( edge_arr [ i_prev ] , edge_arr [ i ] );
		if ( vert_arr [ i ] == NULL ) {
			return false;
		}
		i_prev = i;
	}
	return true;
}

bool BM_edges_from_verts ( BMEdge **edge_arr , BMVert **vert_arr , int len ) {
	int i , i_prev = len - 1;
	for ( i = 0; i < len; i++ ) {
		edge_arr [ i_prev ] = BM_edge_exists ( vert_arr [ i_prev ] , vert_arr [ i ] );
		if ( edge_arr [ i_prev ] == NULL ) {
			return false;
		}
		i_prev = i;
	}
	return true;
}

void BM_edges_from_verts_ensure ( BMesh *bm , BMEdge **edge_arr , BMVert **vert_arr , const int len ) {
	int i , i_prev = len - 1;
	for ( i = 0; i < len; i++ ) {
		edge_arr [ i_prev ] = BM_edge_create ( bm , vert_arr [ i_prev ] , vert_arr [ i ] , NULL , BM_CREATE_NO_DOUBLE );
		i_prev = i;
	}
}

static void bm_vert_attrs_copy (
	BMesh *bm_src , BMesh *bm_dst , const BMVert *v_src , BMVert *v_dst , eCustomDataMask mask_exclude ) {
	if ( ( bm_src == bm_dst ) && ( v_src == v_dst ) ) {
		LIB_assert_msg ( 0 , "BMVert: source and target match" );
		return;
	}
	if ( ( mask_exclude & CD_MASK_NORMAL ) == 0 ) {
		copy_v3_v3 ( v_dst->Normal , v_src->Normal );
	}
	CustomData_bmesh_free_block_data_exclude_by_type ( &bm_dst->VertData , v_dst->Head.Data , mask_exclude );
	CustomData_bmesh_copy_data_exclude_by_type ( &bm_src->VertData , &bm_dst->VertData , v_src->Head.Data , &v_dst->Head.Data , mask_exclude );
}

static void bm_edge_attrs_copy (
	BMesh *bm_src , BMesh *bm_dst , const BMEdge *e_src , BMEdge *e_dst , eCustomDataMask mask_exclude ) {
	if ( ( bm_src == bm_dst ) && ( e_src == e_dst ) ) {
		LIB_assert_msg ( 0 , "BMEdge: source and target match" );
		return;
	}
	CustomData_bmesh_free_block_data_exclude_by_type ( &bm_dst->EdgeData , e_dst->Head.Data , mask_exclude );
	CustomData_bmesh_copy_data_exclude_by_type ( &bm_src->EdgeData , &bm_dst->EdgeData , e_src->Head.Data , &e_dst->Head.Data , mask_exclude );
}

static void bm_loop_attrs_copy (
	BMesh *bm_src , BMesh *bm_dst , const BMLoop *l_src , BMLoop *l_dst , eCustomDataMask mask_exclude ) {
	if ( ( bm_src == bm_dst ) && ( l_src == l_dst ) ) {
		LIB_assert_msg ( 0 , "BMLoop: source and target match" );
		return;
	}
	CustomData_bmesh_free_block_data_exclude_by_type ( &bm_dst->LoopData , l_dst->Head.Data , mask_exclude );
	CustomData_bmesh_copy_data_exclude_by_type ( &bm_src->LoopData , &bm_dst->LoopData , l_src->Head.Data , &l_dst->Head.Data , mask_exclude );
}

static void bm_face_attrs_copy (
	BMesh *bm_src , BMesh *bm_dst , const BMFace *f_src , BMFace *f_dst , eCustomDataMask mask_exclude ) {
	if ( ( bm_src == bm_dst ) && ( f_src == f_dst ) ) {
		LIB_assert_msg ( 0 , "BMFace: source and target match" );
		return;
	}
	if ( ( mask_exclude & CD_MASK_NORMAL ) == 0 ) {
		copy_v3_v3 ( f_dst->Normal , f_src->Normal );
	}
	CustomData_bmesh_free_block_data_exclude_by_type ( &bm_dst->FaceData , f_dst->Head.Data , mask_exclude );
	CustomData_bmesh_copy_data_exclude_by_type ( &bm_src->FaceData , &bm_dst->FaceData , f_src->Head.Data , &f_dst->Head.Data , mask_exclude );
	f_dst->MatNum = f_src->MatNum;
}

void BM_elem_attrs_copy_ex ( BMesh *bm_src ,
			     BMesh *bm_dst ,
			     const void *ele_src_v ,
			     void *ele_dst_v ,
			     const char hflag_mask ,
			     const uint64_t cd_mask_exclude ) {
	/* TODO: Special handling for hide flags? */
	/* TODO: swap src/dst args, everywhere else in bmesh does other way round. */

	const BMHeader *ele_src = ( const BMHeader * ) ele_src_v;
	BMHeader *ele_dst = ( BMHeader * ) ele_dst_v;

	LIB_assert ( ele_src->ElemType == ele_dst->ElemType );
	LIB_assert ( ele_src != ele_dst );

	/* Now we copy flags */
	if ( hflag_mask == 0 ) {
		ele_dst->ElemType = ele_src->ElemType;
	} else if ( hflag_mask == 0xff ) {
		/* pass */
	} else {
		ele_dst->ElemFlag = ( ( ele_dst->ElemFlag & hflag_mask ) | ( ele_src->ElemFlag & ~hflag_mask ) );
	}

	/* Copy specific attributes */
	switch ( ele_dst->ElemType ) {
		case BM_VERT: {
			bm_vert_attrs_copy (
				bm_src , bm_dst , ( const BMVert * ) ele_src , ( BMVert * ) ele_dst , cd_mask_exclude );
		}break;
		case BM_EDGE: {
			bm_edge_attrs_copy (
				bm_src , bm_dst , ( const BMEdge * ) ele_src , ( BMEdge * ) ele_dst , cd_mask_exclude );
		}break;
		case BM_LOOP: {
			bm_loop_attrs_copy (
				bm_src , bm_dst , ( const BMLoop * ) ele_src , ( BMLoop * ) ele_dst , cd_mask_exclude );
		}break;
		case BM_FACE: {
			bm_face_attrs_copy (
				bm_src , bm_dst , ( const BMFace * ) ele_src , ( BMFace * ) ele_dst , cd_mask_exclude );
		}break;
		default: {
			LIB_assert_unreachable ( );
		}break;
	}
}

void BM_elem_attrs_copy ( BMesh *bm_src , BMesh *bm_dst , const void *ele_src , void *ele_dst ) {
	/* BMESH_TODO, default 'use_flags' to false */
	BM_elem_attrs_copy_ex ( bm_src , bm_dst , ele_src , ele_dst , 0x0 , 0x0 );
}

void BM_elem_select_copy ( BMesh *bm_dst , void *ele_dst_v , const void *ele_src_v ) {
	BMHeader *ele_dst = ( BMHeader * ) ele_dst_v;
	const BMHeader *ele_src = ( const BMHeader * ) ele_src_v;

	LIB_assert ( ele_src->ElemType == ele_dst->ElemType );
}
