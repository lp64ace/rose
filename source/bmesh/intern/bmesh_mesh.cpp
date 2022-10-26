#include "bmesh_mesh.h"
#include "bmesh_iterators.h"
#include "bmesh_inline.h"

#include "kernel/rke_customdata.h"

const BMAllocTemplate bm_mesh_allocsize_default = { 512, 1024, 2048, 512 };
const BMAllocTemplate bm_mesh_chunksize_default = { 512, 1024, 2048, 512 };

const BMeshCreateParams bm_mesh_create_params_default = { 0 };

static void bm_mempool_init_ex ( const BMAllocTemplate *allocsize ,
				 const bool use_toolflags ,
				 LIB_mempool **r_vpool ,
				 LIB_mempool **r_epool ,
				 LIB_mempool **r_lpool ,
				 LIB_mempool **r_fpool ) {
	size_t vert_size , edge_size , loop_size , face_size;

	LIB_assert_msg ( !use_toolflags , "Toolflags are not implemented.\n" );

	vert_size = sizeof ( BMVert );
	edge_size = sizeof ( BMEdge );
	loop_size = sizeof ( BMLoop );
	face_size = sizeof ( BMFace );

	if ( r_vpool ) {
		*r_vpool = LIB_mempool_create (
			vert_size , allocsize->totvert , bm_mesh_chunksize_default.totvert , LIB_MEMPOOL_ALLOW_ITER );
	}
	if ( r_epool ) {
		*r_epool = LIB_mempool_create (
			edge_size , allocsize->totedge , bm_mesh_chunksize_default.totedge , LIB_MEMPOOL_ALLOW_ITER );
	}
	if ( r_lpool ) {
		*r_lpool = LIB_mempool_create (
			loop_size , allocsize->totloop , bm_mesh_chunksize_default.totloop , LIB_MEMPOOL_NOP );
	}
	if ( r_fpool ) {
		*r_fpool = LIB_mempool_create (
			face_size , allocsize->totface , bm_mesh_chunksize_default.totface , LIB_MEMPOOL_ALLOW_ITER );
	}
}

static void bm_mempool_init ( BMesh *bm , const BMAllocTemplate *allocsize , const bool use_toolflags ) {
	bm_mempool_init_ex ( allocsize , use_toolflags , &bm->VertPool , &bm->EdgePool , &bm->LoopPool , &bm->FacePool );
}

BMesh *BM_mesh_create ( const BMAllocTemplate *allocsize , const struct BMeshCreateParams *params ) {
	/* allocate the structure */
	BMesh *bm = static_cast< BMesh * >( MEM_callocN ( sizeof ( BMesh ) , __func__ ) );

	// allocate the memory pools for the mesh elements.
	bm_mempool_init ( bm ,
			  ( allocsize ) ? allocsize : &bm_mesh_allocsize_default ,
			  ( params ) ? params->use_toolflags : bm_mesh_create_params_default.use_toolflags );

	CustomData_reset ( &bm->VertData );
	CustomData_reset ( &bm->EdgeData );
	CustomData_reset ( &bm->LoopData );
	CustomData_reset ( &bm->FaceData );

	return bm;
}

void BM_mesh_free ( BMesh *bm ) {
	BM_mesh_data_free ( bm );

	MEM_freeN ( bm );
}

void BM_mesh_data_free ( BMesh *bm ) {
	BMVert *v;
	BMEdge *e;
	BMLoop *l;
	BMFace *f;

	BMIter iter;
	BMIter itersub;

	const bool is_ldata_free = CustomData_bmesh_has_free ( &bm->LoopData );
	const bool is_pdata_free = CustomData_bmesh_has_free ( &bm->FaceData );

	/* Check if we have to call free, if not we can avoid a lot of looping */
	if ( CustomData_bmesh_has_free ( &( bm->VertData ) ) ) {
		BM_ITER_MESH ( v , &iter , bm , BM_VERTS_OF_MESH ) {
			CustomData_bmesh_free_block ( &( bm->VertData ) , &( v->Head.Data ) );
		}
	}
	if ( CustomData_bmesh_has_free ( &( bm->EdgeData ) ) ) {
		BM_ITER_MESH ( e , &iter , bm , BM_EDGES_OF_MESH ) {
			CustomData_bmesh_free_block ( &( bm->EdgeData ) , &( e->Head.Data ) );
		}
	}

	if ( is_ldata_free || is_pdata_free ) {
		BM_ITER_MESH ( f , &iter , bm , BM_FACES_OF_MESH ) {
			if ( is_pdata_free ) {
				CustomData_bmesh_free_block ( &( bm->FaceData ) , &( f->Head.Data ) );
			}
			if ( is_ldata_free ) {
				BM_ITER_ELEM ( l , &itersub , f , BM_LOOPS_OF_FACE ) {
					CustomData_bmesh_free_block ( &( bm->LoopData ) , &( l->Head.Data ) );
				}
			}
		}
	}

	/* Free custom data pools, This should probably go in CustomData_free? */
	if ( bm->VertData.TotLayer ) {
		LIB_mempool_destroy ( bm->VertData.Pool );
	}
	if ( bm->EdgeData.TotLayer ) {
		LIB_mempool_destroy ( bm->EdgeData.Pool );
	}
	if ( bm->LoopData.TotLayer ) {
		LIB_mempool_destroy ( bm->LoopData.Pool );
	}
	if ( bm->FaceData.TotLayer ) {
		LIB_mempool_destroy ( bm->FaceData.Pool );
	}

	/* free custom data */
	CustomData_free ( &bm->VertData , 0 );
	CustomData_free ( &bm->EdgeData , 0 );
	CustomData_free ( &bm->LoopData , 0 );
	CustomData_free ( &bm->FaceData , 0 );

	/* destroy element pools */
	LIB_mempool_destroy ( bm->VertPool );
	LIB_mempool_destroy ( bm->EdgePool );
	LIB_mempool_destroy ( bm->LoopPool );
	LIB_mempool_destroy ( bm->FacePool );

	if ( bm->VertTable ) {
		MEM_freeN ( bm->VertTable );
	}
	if ( bm->EdgeTable ) {
		MEM_freeN ( bm->EdgeTable );
	}
	if ( bm->FaceTable ) {
		MEM_freeN ( bm->FaceTable );
	}
}

void BM_mesh_clear ( BMesh *bm ) {
	/* free old mesh */
	BM_mesh_data_free ( bm );
	memset ( bm , 0 , sizeof ( BMesh ) );

	/* allocate the memory pools for the mesh elements */
	bm_mempool_init ( bm , &bm_mesh_allocsize_default , false );

	CustomData_reset ( &bm->VertData );
	CustomData_reset ( &bm->EdgeData );
	CustomData_reset ( &bm->LoopData );
	CustomData_reset ( &bm->FaceData );
}

void BM_mesh_elem_toolflags_ensure ( BMesh *bm ) {
	LIB_assert_unreachable ( );
}

void BM_mesh_elem_toolflags_clear ( BMesh *bm ) {
	LIB_assert_unreachable ( );
}

void BM_mesh_elem_index_ensure_ex ( BMesh *bm , char htype , int elem_offset [ 4 ] ) {
	if ( elem_offset == nullptr ) {
		/* Simple case. */
		const char htype_needed = bm->ElemIndexDirty & htype;
		if ( htype_needed == 0 ) {
			goto finally;
		}
	}

	if ( htype & BM_VERT ) {
		if ( ( bm->ElemIndexDirty & BM_VERT ) || ( elem_offset && elem_offset [ 0 ] ) ) {
			BMIter iter;
			BMElem *ele;

			int index = elem_offset ? elem_offset [ 0 ] : 0;
			BM_ITER_MESH ( ele , &iter , bm , BM_VERTS_OF_MESH ) {
				BM_elem_index_set ( ele , index++ ); /* set_ok */
			}
			LIB_assert ( elem_offset || index == bm->TotVert );
		} else {
			// printf("%s: skipping vert index calc!\n", __func__);
		}
	}

	if ( htype & BM_EDGE ) {
		if ( ( bm->ElemIndexDirty & BM_EDGE ) || ( elem_offset && elem_offset [ 1 ] ) ) {
			BMIter iter;
			BMElem *ele;

			int index = elem_offset ? elem_offset [ 1 ] : 0;
			BM_ITER_MESH ( ele , &iter , bm , BM_EDGES_OF_MESH ) {
				BM_elem_index_set ( ele , index++ ); /* set_ok */
			}
			LIB_assert ( elem_offset || index == bm->TotEdge );
		} else {
			// printf("%s: skipping edge index calc!\n", __func__);
		}
	}

	if ( htype & ( BM_FACE | BM_LOOP ) ) {
		if ( ( bm->ElemIndexDirty & ( BM_FACE | BM_LOOP ) ) ||
		     ( elem_offset && ( elem_offset [ 2 ] || elem_offset [ 3 ] ) ) ) {
			BMIter iter;
			BMElem *ele;

			const bool update_face = ( htype & BM_FACE ) && ( bm->ElemIndexDirty & BM_FACE );
			const bool update_loop = ( htype & BM_LOOP ) && ( bm->ElemIndexDirty & BM_LOOP );

			int index_loop = elem_offset ? elem_offset [ 2 ] : 0;
			int index = elem_offset ? elem_offset [ 3 ] : 0;

			BM_ITER_MESH ( ele , &iter , bm , BM_FACES_OF_MESH ) {
				if ( update_face ) {
					BM_elem_index_set ( ele , index++ ); /* set_ok */
				}

				if ( update_loop ) {
					BMLoop *l_iter , *l_first;

					l_iter = l_first = BM_FACE_FIRST_LOOP ( ( BMFace * ) ele );
					do {
						BM_elem_index_set ( l_iter , index_loop++ ); /* set_ok */
					} while ( ( l_iter = l_iter->Next ) != l_first );
				}
			}

			LIB_assert ( elem_offset || !update_face || index == bm->TotFace );
			if ( update_loop ) {
				LIB_assert ( elem_offset || !update_loop || index_loop == bm->TotLoop );
			}
		} else {
			// printf("%s: skipping face/loop index calc!\n", __func__);
		}
	}

finally:
	bm->ElemIndexDirty &= ~htype;
	if ( elem_offset ) {
		if ( htype & BM_VERT ) {
			elem_offset [ 0 ] += bm->TotVert;
			if ( elem_offset [ 0 ] != bm->TotVert ) {
				bm->ElemIndexDirty |= BM_VERT;
			}
		}
		if ( htype & BM_EDGE ) {
			elem_offset [ 1 ] += bm->TotEdge;
			if ( elem_offset [ 1 ] != bm->TotEdge ) {
				bm->ElemIndexDirty |= BM_EDGE;
			}
		}
		if ( htype & BM_LOOP ) {
			elem_offset [ 2 ] += bm->TotLoop;
			if ( elem_offset [ 2 ] != bm->TotLoop ) {
				bm->ElemIndexDirty |= BM_LOOP;
			}
		}
		if ( htype & BM_FACE ) {
			elem_offset [ 3 ] += bm->TotFace;
			if ( elem_offset [ 3 ] != bm->TotFace ) {
				bm->ElemIndexDirty |= BM_FACE;
			}
		}
	}
}

void BM_mesh_elem_index_ensure ( BMesh *bm , char htype ) {
	BM_mesh_elem_index_ensure_ex ( bm , htype , nullptr );
}

void BM_mesh_elem_index_validate ( BMesh *bm , const char *location , const char *func , const char *msg_a , const char *msg_b ) {
	const char iter_types [ 3 ] = { BM_VERTS_OF_MESH, BM_EDGES_OF_MESH, BM_FACES_OF_MESH };

	const char flag_types [ 3 ] = { BM_VERT, BM_EDGE, BM_FACE };
	const char *type_names [ 3 ] = { "vert", "edge", "face" };

	BMIter iter;
	BMElem *ele;
	int i;
	bool is_any_error = false;

	for ( i = 0; i < 3; i++ ) {
		const bool is_dirty = ( flag_types [ i ] & bm->ElemIndexDirty ) != 0;
		int index = 0;
		bool is_error = false;
		int err_val = 0;
		int err_idx = 0;

		BM_ITER_MESH ( ele , &iter , bm , iter_types [ i ] ) {
			if ( !is_dirty ) {
				if ( BM_elem_index_get ( ele ) != index ) {
					err_val = BM_elem_index_get ( ele );
					err_idx = index;
					is_error = true;
					break;
				}
			}
			index++;
		}

		if ( ( is_error == true ) && ( is_dirty == false ) ) {
			is_any_error = true;
			fprintf ( stderr ,
				  "Invalid Index: at %s, %s, %s[%d] invalid index %d, '%s', '%s'\n" ,
				  location ,
				  func ,
				  type_names [ i ] ,
				  err_idx ,
				  err_val ,
				  msg_a ,
				  msg_b );
		} else if ( ( is_error == false ) && ( is_dirty == true ) ) {

#ifdef _DEBUG

			// dirty may have been incorrectly set
			fprintf ( stderr ,
				  "Invalid Dirty: at %s, %s (%s), dirty flag was set but all index values are "
				  "correct, '%s', '%s'\n" ,
				  location ,
				  func ,
				  type_names [ i ] ,
				  msg_a ,
				  msg_b );
#endif
		}
	}

	( void ) is_any_error; // shut up the compiler
}

bool BM_mesh_elem_table_check ( BMesh *bm ) {
	BMIter iter;
	BMElem *ele;
	int i;

	if ( bm->VertTable && ( ( bm->ElemTableDirty & BM_VERT ) == 0 ) ) {
		BM_ITER_MESH_INDEX ( ele , &iter , bm , BM_VERTS_OF_MESH , i ) {
			if ( ele != ( BMElem * ) bm->VertTable [ i ] ) {
				return false;
			}
		}
	}

	if ( bm->EdgeTable && ( ( bm->ElemTableDirty & BM_EDGE ) == 0 ) ) {
		BM_ITER_MESH_INDEX ( ele , &iter , bm , BM_EDGES_OF_MESH , i ) {
			if ( ele != ( BMElem * ) bm->EdgeTable [ i ] ) {
				return false;
			}
		}
	}

	if ( bm->FaceTable && ( ( bm->ElemTableDirty & BM_FACE ) == 0 ) ) {
		BM_ITER_MESH_INDEX ( ele , &iter , bm , BM_FACES_OF_MESH , i ) {
			if ( ele != ( BMElem * ) bm->FaceTable [ i ] ) {
				return false;
			}
		}
	}

	return true;
}

void BM_mesh_elem_table_ensure ( BMesh *bm , const char htype ) {
	/* assume if the array is non-null then its valid and no need to recalc */
	const char htype_needed =
		( ( ( bm->VertTable && ( ( bm->ElemTableDirty & BM_VERT ) == 0 ) ) ? 0 : BM_VERT ) |
		  ( ( bm->EdgeTable && ( ( bm->ElemTableDirty & BM_EDGE ) == 0 ) ) ? 0 : BM_EDGE ) |
		  ( ( bm->FaceTable && ( ( bm->ElemTableDirty & BM_FACE ) == 0 ) ) ? 0 : BM_FACE ) ) &
		htype;

	LIB_assert ( ( htype & ~BM_ALL_NOLOOP ) == 0 );

	/* in debug mode double check we didn't need to recalculate */
	LIB_assert ( BM_mesh_elem_table_check ( bm ) == true );

	if ( htype_needed == 0 ) {
		goto finally;
	}

	if ( htype_needed & BM_VERT ) {
		if ( bm->VertTable && bm->TotVert <= bm->VertTableTot && bm->TotVert * 2 >= bm->VertTableTot ) {
			/* pass (re-use the array) */
		} else {
			if ( bm->VertTable ) {
				MEM_freeN ( bm->VertTable );
			}
			bm->VertTable = static_cast< BMVert ** >( MEM_mallocN ( sizeof ( void ** ) * bm->TotVert , "bm->VertTable" ) );
			bm->VertTableTot = bm->TotVert;
		}
		BM_iter_as_array ( bm , BM_VERTS_OF_MESH , nullptr , ( void ** ) bm->VertTable , bm->TotVert );
	}
	if ( htype_needed & BM_EDGE ) {
		if ( bm->EdgeTable && bm->TotEdge <= bm->EdgeTableTot && bm->TotEdge * 2 >= bm->EdgeTableTot ) {
			/* pass (re-use the array) */
		} else {
			if ( bm->EdgeTable ) {
				MEM_freeN ( bm->EdgeTable );
			}
			bm->EdgeTable = static_cast< BMEdge ** >( MEM_mallocN ( sizeof ( void ** ) * bm->TotEdge , "bm->EdgeTable" ) );
			bm->EdgeTableTot = bm->TotEdge;
		}
		BM_iter_as_array ( bm , BM_EDGES_OF_MESH , nullptr , ( void ** ) bm->EdgeTable , bm->TotEdge );
	}
	if ( htype_needed & BM_FACE ) {
		if ( bm->FaceTable && bm->TotFace <= bm->FaceTableTot && bm->TotFace * 2 >= bm->FaceTableTot ) {
			/* pass (re-use the array) */
		} else {
			if ( bm->FaceTable ) {
				MEM_freeN ( bm->FaceTable );
			}
			bm->FaceTable = static_cast< BMFace ** >( MEM_mallocN ( sizeof ( void ** ) * bm->TotFace , "bm->FaceTable" ) );
			bm->FaceTableTot = bm->TotFace;
		}
		BM_iter_as_array ( bm , BM_FACES_OF_MESH , nullptr , ( void ** ) bm->FaceTable , bm->TotFace );
	}

finally:
	/* Only clear dirty flags when all the pointers and data are actually valid.
	* This prevents possible threading issues when dirty flag check failed but
	* data wasn't ready still.
	*/
	bm->ElemTableDirty &= ~htype_needed;
}

void BM_mesh_elem_table_init ( BMesh *bm , const char htype ) {
	LIB_assert ( ( htype & ~BM_ALL_NOLOOP ) == 0 );

	/* force recalc */
	BM_mesh_elem_table_free ( bm , BM_ALL_NOLOOP );
	BM_mesh_elem_table_ensure ( bm , htype );
}

void BM_mesh_elem_table_free ( BMesh *bm , const char htype ) {
	if ( htype & BM_VERT ) {
		MEM_SAFE_FREE ( bm->VertTable );
	}

	if ( htype & BM_EDGE ) {
		MEM_SAFE_FREE ( bm->EdgeTable );
	}

	if ( htype & BM_FACE ) {
		MEM_SAFE_FREE ( bm->FaceTable );
	}
}
