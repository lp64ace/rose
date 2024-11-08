#ifndef KER_LIBRARY_H
#define KER_LIBRARY_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Library;
struct Main;

/* -------------------------------------------------------------------- */
/** \name Path Methods
 * \{ */

bool KER_library_filepath_set(struct Main *main, struct Library *lib, const char *filepath);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_LIBRARY_H
