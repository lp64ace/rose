#ifndef LIB_SORT_UTILS_H
#define LIB_SORT_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \note keep \a sort_value first,
 * so cmp functions can be reused.
 */
struct SortPtrByFloat {
	float sort_value;
	void *data;
};

struct SortIntByFloat {
	float sort_value;
	int data;
};

struct SortPtrByInt {
	int sort_value;
	void *data;
};

struct SortIntByInt {
	int sort_value;
	int data;
};

int LIB_sortutil_cmp_flt(const void *a_, const void *b_);
int LIB_sortutil_cmp_flt_reverse(const void *a_, const void *b_);
int LIB_sortutil_cmp_int(const void *a_, const void *b_);
int LIB_sortutil_cmp_int_reverse(const void *a_, const void *b_);
int LIB_sortutil_cmp_ptr(const void *a_, const void *b_);
int LIB_sortutil_cmp_ptr_reverse(const void *a_, const void *b_);

#ifdef __cplusplus
}
#endif

#endif	// LIB_SORT_UTILS_H
