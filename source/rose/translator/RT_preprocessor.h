#ifndef RT_PREPROCESSOR_H
#define RT_PREPROCESSOR_H

#include "LIB_listbase.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RTContext;

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void RT_pp_do(struct RTContext *, const struct RTFile *file, ListBase *tokens);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_PREPROCESSOR_H
