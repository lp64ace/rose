#pragma once

#include "mesh/bmesh_class.h"

int bmesh_radial_length ( const BMLoop *l );
int bmesh_disk_count_at_most ( const BMVert *v , int count_max );
int bmesh_disk_count ( const BMVert *v );

enum {
	_FLAG_JF = ( 1 << 0 ) ,       // Join faces. 
	_FLAG_MF = ( 1 << 1 ) ,       // Make face. 
	_FLAG_MV = ( 1 << 1 ) ,       // Make face, vertex. 
	_FLAG_OVERLAP = ( 1 << 2 ) ,  // General overlap flag. 
	_FLAG_WALK = ( 1 << 3 ) ,     // General walk flag (keep clean). 
	_FLAG_WALK_ALT = ( 1 << 4 ) , // Same as #_FLAG_WALK, for when a second tag is needed. 

	_FLAG_ELEM_CHECK = ( 1 << 7 ) , // Reserved for bmesh_elem_check.
};

#define BM_ELEM_API_FLAG_ENABLE(element, f) \
  { \
    ((element)->Head.ApiFlag |= (f)); \
  } \
  (void)0
#define BM_ELEM_API_FLAG_DISABLE(element, f) \
  { \
    ((element)->Head.ApiFlag &= (byte) ~(f)); \
  } \
  (void)0
#define BM_ELEM_API_FLAG_TEST(element, f) ((element)->Head.ApiFlag & (f))
#define BM_ELEM_API_FLAG_CLEAR(element) \
  { \
    ((element)->Head.ApiFlag = 0); \
  } \
  (void)0
