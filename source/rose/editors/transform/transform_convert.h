#ifndef TRANSFORM_CONVERT_H
#define TRANSFORM_CONVERT_H

#include "transform.h"

#ifdef __cplusplus
extern "C" {
#endif

void ED_transform_data_create(struct rContext *C, struct TransInfo *t);
void ED_transform_object_create(struct rContext *C, struct TransInfo *t);

#ifdef __cplusplus
}
#endif

#endif	// !TRANSFORM_CONVERT_H
