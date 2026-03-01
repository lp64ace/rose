#ifndef FILE_H
#define FILE_H

#include "UI_interface.h"

struct FileSelectParams;
struct ScrArea;
struct SpaceFile;
struct rContext;
struct uiBut;
struct wmOperatorType;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name File Select Operations
 * \{ */

void file_directory_enter_handle(struct rContext *C, struct uiBut *but, void *unused1, void *unused2);

void file_draw_check_ex(struct rContext *C, struct ScrArea *area);
void file_draw_check(struct rContext *C);

void file_sfile_to_operator(struct rContext *C, struct Main *main, struct wmOperator *op, struct SpaceFile *sfile);
void file_operator_to_sfile(struct Main *main, struct SpaceFile *sfile, struct wmOperator *op);

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Select Params
 * \{ */

struct FileSelectParams *ED_fileselect_ensure_active_params(struct SpaceFile *file);
struct FileSelectParams *ED_fileselect_get_active_params(struct SpaceFile *file);

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Select Params
 * \{ */

void ED_fileselect_change_dir(struct rContext *C);

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Operator Types
 * \{ */

void FILE_OT_cancel(struct wmOperatorType *ot);

void file_operatortypes();

void file_keymap(struct wmKeyConfig *keyconf);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// FILE_H
