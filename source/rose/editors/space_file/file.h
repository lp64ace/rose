#ifndef FILE_H
#define FILE_H

#include "UI_interface.h"

struct FileSelectParams;
struct SpaceFile;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name File Select Params
 * \{ */

struct FileSelectParams *ED_fileselect_ensure_active_params(struct SpaceFile *file);
struct FileSelectParams *ED_fileselect_get_active_params(struct SpaceFile *file);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// FILE_H
