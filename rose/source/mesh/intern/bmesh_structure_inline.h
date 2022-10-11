#pragma once

#include "bmesh_private.h"

ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 , 2 ) ROSE_INLINE BMEdge *bmesh_disk_edge_next ( const BMEdge *e ,
                                                                                         const BMVert *v ) {
        return BM_DISK_EDGE_NEXT ( e , v );
}

ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 , 2 ) ROSE_INLINE BMEdge *bmesh_disk_edge_prev ( const BMEdge *e ,
                                                                                         const BMVert *v ) {
        return BM_DISK_EDGE_PREV ( e , v );
}
