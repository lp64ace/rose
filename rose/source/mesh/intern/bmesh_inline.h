#pragma once

#include "mesh/bmesh_class.h"

#define MESH_elem_flag_test(ele, hflag) _mesh_elem_flag_test(&(ele)->Head, hflag)
#define MESH_elem_flag_test_bool(ele, hflag) _mesh_elem_flag_test_bool(&(ele)->Head, hflag)
#define MESH_elem_flag_enable(ele, hflag) _mesh_elem_flag_enable(&(ele)->Head, hflag)
#define MESH_elem_flag_disable(ele, hflag) _mesh_elem_flag_disable(&(ele)->Head, hflag)
#define MESH_elem_flag_set(ele, hflag, val) _mesh_elem_flag_set(&(ele)->Head, hflag, val)
#define MESH_elem_flag_toggle(ele, hflag) _mesh_elem_flag_toggle(&(ele)->Head, hflag)
#define MESH_elem_flag_merge(ele_a, ele_b) _mesh_elem_flag_merge(&(ele_a)->Head, &(ele_b)->Head)
#define MESH_elem_flag_merge_ex(ele_a, ele_b, hflag_and) _mesh_elem_flag_merge_ex(&(ele_a)->Head, &(ele_b)->Head, hflag_and)
#define MESH_elem_flag_merge_into(ele, ele_a, ele_b) _mesh_elem_flag_merge_into(&(ele)->Head, &(ele_a)->Head, &(ele_b)->Head)

ATTR_WARN_UNUSED_RESULT ROSE_INLINE char _mesh_elem_flag_test ( const BMHeader *head , const char hflag ) {
	return head->ElemFlag & hflag;
}

ATTR_WARN_UNUSED_RESULT ROSE_INLINE bool _mesh_elem_flag_test_bool ( const BMHeader *head , const char hflag ) {
	return ( head->ElemFlag & hflag ) != 0;
}

ROSE_INLINE void _mesh_elem_flag_enable ( BMHeader *head , const char hflag ) {
	head->ElemFlag |= hflag;
}

ROSE_INLINE void _mesh_elem_flag_disable ( BMHeader *head , const char hflag ) {
	head->ElemFlag &= ( char ) ~hflag;
}

ROSE_INLINE void _mesh_elem_flag_set ( BMHeader *head , const char hflag , const int val ) {
	if ( val ) {
		_mesh_elem_flag_enable ( head , hflag );
	} else {
		_mesh_elem_flag_disable ( head , hflag );
	}
}

ROSE_INLINE void _mesh_elem_flag_toggle ( BMHeader *head , const char hflag ) {
	head->ElemFlag ^= hflag;
}

ROSE_INLINE void _mesh_elem_flag_merge ( BMHeader *head_a , BMHeader *head_b ) {
	head_a->ElemFlag = head_b->ElemFlag = head_a->ElemFlag | head_b->ElemFlag;
}

ROSE_INLINE void _mesh_elem_flag_merge_ex ( BMHeader *head_a , BMHeader *head_b , const char hflag_and ) {
	if ( ( ( head_a->ElemFlag & head_b->ElemFlag ) & hflag_and ) == 0 ) {
		head_a->ElemFlag &= ~hflag_and;
		head_b->ElemFlag &= ~hflag_and;
	}
	_mesh_elem_flag_merge ( head_a , head_b );
}

ROSE_INLINE void _mesh_elem_flag_merge_into ( BMHeader *head , const BMHeader *head_a , const BMHeader *head_b ) {
	head->ElemFlag = head_a->ElemFlag | head_b->ElemFlag;
}
