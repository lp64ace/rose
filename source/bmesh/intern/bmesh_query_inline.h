#pragma once

#include "bmesh/bmesh_class.h"

/**
* Returns whether or not a given vertex is
* is part of a given edge.
*/
ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 ) ROSE_INLINE
bool BM_vert_in_edge ( const BMEdge *e , const BMVert *v ) {
        return ( ELEM ( v , e->Vert1 , e->Vert2 ) );
}

/**
* Returns whether or not a given edge is part of a given loop.
*/
ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 , 2 ) ROSE_INLINE
bool BM_edge_in_loop ( const BMEdge *e , const BMLoop *l ) {
        return ( l->Edge == e || l->Prev->Edge == e );
}

/**
* Returns whether or not two vertices are in
* a given edge
*/
ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 , 2 , 3 ) ROSE_INLINE
bool BM_verts_in_edge ( const BMVert *v1 , const BMVert *v2 , const BMEdge *e ) {
        return ( ( e->Vert1 == v1 && e->Vert2 == v2 ) || ( e->Vert1 == v2 && e->Vert2 == v1 ) );
}

/**
* Given a edge and one of its vertices, returns
* the other vertex.
*/
ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 , 2 ) ROSE_INLINE 
BMVert *BM_edge_other_vert ( BMEdge *e , const BMVert *v ) {
        if ( e->Vert1 == v ) {
                return e->Vert2;
        } else if ( e->Vert2 == v ) {
                return e->Vert1;
        }
        return NULL;
}

/**
* Tests whether or not the edge is part of a wire.
* (ie: has no faces attached to it)
*/
ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 ) ROSE_INLINE 
bool BM_edge_is_wire ( const BMEdge *e ) {
        return ( e->Loop == NULL );
}

/**
* Tests whether or not this edge is manifold.
* A manifold edge has exactly 2 faces attached to it.
*/
ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 ) ROSE_INLINE 
bool BM_edge_is_manifold ( const BMEdge *e ) {
        const BMLoop *l = e->Loop;
        return ( l && ( l->RadialNext != l ) && ( l->RadialNext->RadialNext == l ) );
}

/**
* Tests that the edge is manifold and
* that both its faces point the same way.
*/
ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 ) ROSE_INLINE 
bool BM_edge_is_contiguous ( const BMEdge *e ) {
        const BMLoop *l = e->Loop;
        const BMLoop *l_other;
        return ( l && ( ( l_other = l->RadialNext ) != l ) &&
                 ( l_other->RadialNext == l ) && ( l_other->Vert != l->Vert ) );
}

/**
* Tests whether or not an edge is on the boundary
* of a shell (has one face associated with it)
*/
ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 ) ROSE_INLINE
bool BM_edge_is_boundary ( const BMEdge *e ) {
        const BMLoop *l = e->Loop;
        return ( l && ( l->RadialNext == l ) );
}

/**
* Tests whether one loop is next to another within the same face.
*/
ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 , 2 ) ROSE_INLINE
bool BM_loop_is_adjacent ( const BMLoop *l_a , const BMLoop *l_b ) {
        LIB_assert ( l_a->Face == l_b->Face );
        LIB_assert ( l_a != l_b );
        return ( ELEM ( l_b , l_a->Next , l_a->Prev ) );
}

ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 ) ROSE_INLINE 
bool BM_loop_is_manifold ( const BMLoop *l ) {
        return ( ( l != l->RadialNext ) && ( l == l->RadialNext->RadialNext ) );
}

/**
* Check if we have a single wire edge user.
*/
ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 ) ROSE_INLINE 
bool BM_vert_is_wire_endpoint ( const BMVert *v ) {
        const BMEdge *e = v->Edge;
        if ( e && e->Loop == NULL ) {
                return ( BM_DISK_EDGE_NEXT ( e , v ) == e );
        }
        return false;
}
