#pragma once

#include "bmesh_structure_inline.h"

void bmesh_disk_edge_append ( BMEdge *e , BMVert *v ) ATTR_NONNULL ( );
void bmesh_disk_edge_remove ( BMEdge *e , BMVert *v ) ATTR_NONNULL ( );
ROSE_INLINE BMEdge *bmesh_disk_edge_next_safe ( const BMEdge *e ,
                                                const BMVert *v ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );
ROSE_INLINE BMEdge *bmesh_disk_edge_prev_safe ( const BMEdge *e ,
                                                const BMVert *v ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );
ROSE_INLINE BMEdge *bmesh_disk_edge_next ( const BMEdge *e , const BMVert *v ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );
ROSE_INLINE BMEdge *bmesh_disk_edge_prev ( const BMEdge *e , const BMVert *v ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );
int bmesh_disk_facevert_count_at_most ( const BMVert *v , int count_max ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );

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

/* NOTE:
*      bmesh_radial_loop_next(BMLoop *l) / prev.
* just use member access l->RadialNext, l->RadialPrev now */
int bmesh_radial_facevert_count_at_most ( const BMLoop *l ,
                                          const BMVert *v ,
                                          int count_max ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );

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

bool bmesh_radial_validate ( int radlen , BMLoop *l ) ATTR_NONNULL ( 2 );

void bmesh_radial_loop_append ( BMEdge *e , BMLoop *l ) ATTR_NONNULL ( );

/**
* \brief BMESH RADIAL REMOVE LOOP
*
* Removes a loop from an radial cycle. If edge e is non-NULL
* it should contain the radial cycle, and it will also get
* updated (in the case that the edge's link into the radial
* cycle was the loop which is being removed from the cycle).
*/
void bmesh_radial_loop_remove ( BMEdge *e , BMLoop *l ) ATTR_NONNULL ( );
