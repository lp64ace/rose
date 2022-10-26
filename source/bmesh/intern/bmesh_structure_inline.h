#pragma once

#include "bmesh_private.h"

ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 , 2 ) ROSE_INLINE BMDiskLink *bmesh_disk_edge_link_from_vert ( const BMEdge *e , const BMVert *v ) {
        LIB_assert ( ELEM ( v , e->Vert1 , e->Vert2 ) );
        return ( BMDiskLink * ) &( &e->DiskLink1 ) [ v == e->Vert2 ];
}

/**
 * \brief Next Disk Edge
 *
 * Find the next edge in a disk cycle
 *
 * \return Pointer to the next edge in the disk cycle for the vertex v.
 */
ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 ) ROSE_INLINE BMEdge *bmesh_disk_edge_next_safe ( const BMEdge *e , const BMVert *v ) {
        if ( v == e->Vert1 ) {
                return e->DiskLink1.Next;
        }
        if ( v == e->Vert2 ) {
                return e->DiskLink2.Next;
        }
        return NULL;
}

ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 ) ROSE_INLINE BMEdge *bmesh_disk_edge_prev_safe ( const BMEdge *e , const BMVert *v ) {
        if ( v == e->Vert1 ) {
                return e->DiskLink1.Prev;
        }
        if ( v == e->Vert2 ) {
                return e->DiskLink2.Prev;
        }
        return NULL;
}

ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 , 2 ) ROSE_INLINE BMEdge *bmesh_disk_edge_next ( const BMEdge *e ,
                                                                                          const BMVert *v ) {
        return BM_DISK_EDGE_NEXT ( e , v );
}

ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 , 2 ) ROSE_INLINE BMEdge *bmesh_disk_edge_prev ( const BMEdge *e ,
                                                                                          const BMVert *v ) {
        return BM_DISK_EDGE_PREV ( e , v );
}
