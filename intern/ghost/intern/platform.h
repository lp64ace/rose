#pragma once

#include "glib.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Init the platform object and all available modules (graphics, inputs and more). */
int InitPlatform(void);
/** Exit and delete the platform object deallocating any memory associated with this module. */
void ClosePlatform(void);

#ifdef __cplusplus
}
#endif
