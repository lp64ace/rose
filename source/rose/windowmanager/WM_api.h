#ifndef WM_API_H
#define WM_API_H

#include "DNA_windowmanager_types.h"

#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct rContext;

void WM_init(struct rContext *C);
void WM_main(struct rContext *C);
void WM_exit(struct rContext *C);

#ifdef __cplusplus
}
#endif

#endif // WM_API_H