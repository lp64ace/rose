#pragma once

#include "lib/lib_utildefines.h"

#include "bmesh_private.h"

struct BMAllocTemplate;

void BM_mesh_elem_toolflags_ensure ( BMesh *bm );
void BM_mesh_elem_toolflags_clear ( BMesh *bm );

struct BMeshCreateParams {
	bool use_toolflags : 1;
};

/** \brief BMesh Make Mesh. 
* Allocates a new BMesh structure. 
* \return The New bmesh. 
* \note ob is needed by multires. */
BMesh *BM_mesh_create ( const struct BMAllocTemplate *allocsize , const struct BMeshCreateParams *params );

/** \brief BMesh Free Mesh.
* Frees a BMesh data and its structure. */
void BM_mesh_free ( BMesh *bm );

/** \brief BMesh Free Mesh Data. 
* Frees a BMesh structure. 
* \note frees mesh, but not actual BMesh struct. */
void BM_mesh_data_free ( BMesh *bm );

/** \brief BMesh Clear Mesh.
* Clear all data in bm. */
void BM_mesh_clear ( BMesh *bm );

void BM_mesh_elem_index_ensure_ex ( BMesh *bm , char htype , int elem_offset [ 4 ] );
void BM_mesh_elem_index_ensure ( BMesh *bm , char htype );

/**
* Array checking/setting macros.
*
* Currently vert/edge/loop/face index data is being abused, in a few areas of the code.
*
* To avoid correcting them afterwards, set 'bm->elem_index_dirty' however its possible
* this flag is set incorrectly which could crash blender.
*
* Functions that calls this function may depend on dirty indices on being set.
*
* This is read-only, so it can be used for assertions that don't impact behavior.
*/
void BM_mesh_elem_index_validate ( BMesh *bm , const char *location , const char *func , const char *msg_a , const char *msg_b );

bool BM_mesh_elem_table_check ( BMesh *bm );

void BM_mesh_elem_table_ensure ( BMesh *bm , char htype );

// use BM_mesh_elem_table_ensure where possible to avoid full rebuild
void BM_mesh_elem_table_init ( BMesh *bm , char htype );
void BM_mesh_elem_table_free ( BMesh *bm , char htype );

typedef struct BMAllocTemplate {
	int totvert , totedge , totloop , totface;
} BMAllocTemplate;

/* used as an extern, defined in bmesh.h */
extern const BMAllocTemplate bm_mesh_allocsize_default;
extern const BMAllocTemplate bm_mesh_chunksize_default;
