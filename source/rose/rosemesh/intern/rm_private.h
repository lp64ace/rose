#ifndef RM_PRIVATE_H
#define RM_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/* clang format off */

#define RM_ELEM_API_FLAG_ENABLE(element, f) \
	{ \
		((element)->head.api_flag |= (f)); \
	} \
	(void)0
#define RM_ELEM_API_FLAG_DISABLE(element, f) \
	{ \
		((element)->head.api_flag &= (uchar) ~(f)); \
	} \
	(void)0
#define RM_ELEM_API_FLAG_TEST(element, f) \
	{ \
		((element)->head.api_flag & (f)) \
	} \
	(void)0
#define RM_ELEM_API_FLAG_CLEAR(element) \
	{ \
		((element)->head.api_flag = 0); \
	} \
	(void)0

/* clang format on */

#ifdef __cplusplus
}
#endif

/* include the rest of our private declarations */
#include "intern/rm_structure.h"

#endif // RM_PRIVATE_H
