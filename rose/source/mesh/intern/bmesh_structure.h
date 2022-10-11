#pragma once

#include "bmesh_structure_inline.h"

/**
* \brief Disk count face vert.
*
* Counts the number of loop users 
* for this vertex. Note that this is 
* equivalent to counting the number of 
* faces incident upon this vertex.
*/
int bmesh_disk_facevert_count ( const BMVert *v ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );

/**
* \brief Find first face edge.
*
* Finds the first edge in a vertices 
* Disk cycle that has one of this 
* vert's loops attached 
* to it.
*/
BMEdge *bmesh_disk_faceedge_find_first ( const BMEdge *e , const BMVert *v ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );

/**
* Special case for BM_LOOPS_OF_VERT & BM_FACES_OF_VERT, avoids 2x calls.
*
* The returned #BMLoop.Edge matches the result of #bmesh_disk_faceedge_find_first.
*/
BMLoop *bmesh_disk_faceloop_find_first ( const BMEdge *e , const BMVert *v ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );

/**
* A version of #bmesh_disk_faceloop_find_first that ignores hidden faces.
*/
BMLoop *bmesh_disk_faceloop_find_first_visible ( const BMEdge *e , const BMVert *v ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );

BMEdge *bmesh_disk_faceedge_find_next ( const BMEdge *e , const BMVert *v ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );

/**
* \brief BME Radial find first face vert.
*
* Finds the first loop of v around radial 
* cycle.
*/
BMLoop *bmesh_radial_faceloop_find_first ( const BMLoop *l , const BMVert *v ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );
BMLoop *bmesh_radial_faceloop_find_next ( const BMLoop *l , const BMVert *v ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );

/**
* \brief Radial count face vert.
*
* Returns the number of times a vertex appears 
* in a radial cycle.
*/
int bmesh_radial_facevert_count ( const BMLoop *l , const BMVert *v ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );

/**
* \brief Radial check face vert.
*
* Quicker check for `bmesh_radial_facevert_count(...) != 0`.
*/
bool bmesh_radial_facevert_check ( const BMLoop *l , const BMVert *v ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );
