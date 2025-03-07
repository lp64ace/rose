#ifndef DNA_LAYER_TYPES_H
#define DNA_LAYER_TYPES_H

#include "DNA_listbase.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct ViewLayer {
    struct ViewLayer *prev, *next;

    char name[64];

    ListBase drawdata;
} ViewLayer;

#ifdef __cplusplus
}
#endif

#endif // DNA_LAYER_TYPES_H
