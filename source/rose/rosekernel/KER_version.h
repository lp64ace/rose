#pragma once

#include "LIB_utildefines.h"

#define ROSE_VERSION_MAJOR 0
#define ROSE_VERSION_MINOR 4
#define ROSE_VERSION_REVISION 0

#define AUX_STR_EXP(__Α) #__Α
#define AUX_STR(__Α) AUX_STR_EXP(__Α)

#define ROSE_VERSION AUX_STR(ROSE_VERSION_MAJOR) "." AUX_STR(ROSE_VERSION_MINOR) "." AUX_STR(ROSE_VERSION_REVISION)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
