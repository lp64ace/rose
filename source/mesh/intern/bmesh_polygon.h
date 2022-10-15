#pragma once

#include "bmesh_core.h"

/**
* \brief Update face normal.
*
* Updates the stored normal for the 
* given face. Requires that a buffer 
* of sufficient length to store projected 
* coordinates for all of the face's vertices 
* is passed in as well.
*/
float BM_face_calc_normal ( const BMFace *f , float r_no [ 3 ] ) ATTR_NONNULL ( );

// Exact same as 'BM_face_calc_normal' but accepts vertex coords.
float BM_face_calc_normal_vcos ( const BMesh *bm ,
                                 const BMFace *f ,
                                 float r_no [ 3 ] ,
                                 float const ( *vertexCos ) [ 3 ] ) ATTR_NONNULL ( );

void BM_face_normal_update ( BMFace *f ) ATTR_NONNULL ( );
