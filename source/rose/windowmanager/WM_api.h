#ifndef WM_API_H
#define WM_API_H

#include "DNA_windowmanager_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void WM_init();
void WM_main();
void WM_exit();

#ifdef __cplusplus
}
#endif

#endif // WM_API_H