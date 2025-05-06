#ifndef RM_INLINE_H
#define RM_INLINE_H

#include "LIB_utildefines.h"

/* stuff for dealing with header flags */
#define RM_elem_flag_test(ele, hflag) _rm_elem_flag_test(&(ele)->head, hflag)
#define RM_elem_flag_test_bool(ele, hflag) _rm_elem_flag_test_bool(&(ele)->head, hflag)
#define RM_elem_flag_enable(ele, hflag) _rm_elem_flag_enable(&(ele)->head, hflag)
#define RM_elem_flag_disable(ele, hflag) _rm_elem_flag_disable(&(ele)->head, hflag)
#define RM_elem_flag_set(ele, hflag, val) _rm_elem_flag_set(&(ele)->head, hflag, val)
#define RM_elem_flag_toggle(ele, hflag) _rm_elem_flag_toggle(&(ele)->head, hflag)
#define RM_elem_flag_merge(ele_a, ele_b) _rm_elem_flag_merge(&(ele_a)->head, &(ele_b)->head)
#define RM_elem_flag_merge_ex(ele_a, ele_b, hflag_and) _rm_elem_flag_merge_ex(&(ele_a)->head, &(ele_b)->head, hflag_and)
#define RM_elem_flag_merge_into(ele, ele_a, ele_b) _rm_elem_flag_merge_into(&(ele)->head, &(ele_a)->head, &(ele_b)->head)

ROSE_INLINE char _rm_elem_flag_test(const RMHeader *head, const char hflag) {
	return head->hflag & hflag;
}

ROSE_INLINE bool _rm_elem_flag_test_bool(const RMHeader *head, const char hflag) {
	return (head->hflag & hflag) != 0;
}

ROSE_INLINE void _rm_elem_flag_enable(RMHeader *head, const char hflag) {
	head->hflag |= hflag;
}

ROSE_INLINE void _rm_elem_flag_disable(RMHeader *head, const char hflag) {
	head->hflag &= (char)~hflag;
}

ROSE_INLINE void _rm_elem_flag_set(RMHeader *head, const char hflag, const int val) {
	if (val) {
		_rm_elem_flag_enable(head, hflag);
	}
	else {
		_rm_elem_flag_disable(head, hflag);
	}
}

ROSE_INLINE void _rm_elem_flag_toggle(RMHeader *head, const char hflag) {
	head->hflag ^= hflag;
}

ROSE_INLINE void _rm_elem_flag_merge(RMHeader *head_a, RMHeader *head_b) {
	head_a->hflag = head_b->hflag = head_a->hflag | head_b->hflag;
}

ROSE_INLINE void _rm_elem_flag_merge_ex(RMHeader *head_a, RMHeader *head_b, const char hflag_and) {
	if (((head_a->hflag & head_b->hflag) & hflag_and) == 0) {
		head_a->hflag &= ~hflag_and;
		head_b->hflag &= ~hflag_and;
	}
	_rm_elem_flag_merge(head_a, head_b);
}

ROSE_INLINE void _rm_elem_flag_merge_into(RMHeader *head, const RMHeader *head_a, const RMHeader *head_b) {
	head->hflag = head_a->hflag | head_b->hflag;
}

/**
 * notes on #RM_elem_index_set(...) usage,
 * Set index is sometimes abused as temp storage, other times we can't be
 * sure if the index values are valid because certain operations have modified
 * the mesh structure.
 *
 * To set the elements to valid indices 'BM_mesh_elem_index_ensure' should be used
 * rather than adding inline loops, however there are cases where we still
 * set the index directly
 *
 * In an attempt to manage this,
 * here are 5 tags I'm adding to uses of #BM_elem_index_set
 *
 * - 'set_inline'  -- since the data is already being looped over set to a
 *                    valid value inline.
 *
 * - 'set_dirty!'  -- intentionally sets the index to an invalid value,
 *                    flagging 'bm->elem_index_dirty' so we don't use it.
 *
 * - 'set_ok'      -- this is valid use since the part of the code is low level.
 *
 * - 'set_ok_invalid'  -- set to -1 on purpose since this should not be
 *                    used without a full array re-index, do this on
 *                    adding new vert/edge/faces since they may be added at
 *                    the end of the array.
 *
 * - campbell */

#define RM_elem_index_get(ele) _rm_elem_index_get(&(ele)->head)
#define RM_elem_index_set(ele, index) _rm_elem_index_set(&(ele)->head, index)

ROSE_INLINE void _rm_elem_index_set(RMHeader *head, const int index) {
	head->index = index;
}

ROSE_INLINE int _rm_elem_index_get(const RMHeader *head) {
	return head->index;
}

#endif // RM_INLINE_H
