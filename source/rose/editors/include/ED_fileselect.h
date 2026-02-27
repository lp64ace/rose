#ifndef ED_FILESELECT_H
#define ED_FILESELECT_H

struct ScrArea;
struct SpaceFile;
struct wmOperator;
struct wmWindow;

#ifdef __cplusplus
extern "C" {
#endif

void ED_fileselect_set_params_from_userdef(struct SpaceFile *sfile);
/**
 * Update the user-preference data for the file space. In fact, this also contains some
 * non-FileSelectParams data, but we can safely ignore this.
 *
 * \param temp_win_size: If the browser was opened in a temporary window,
 * pass its size here so we can store that in the preferences. Otherwise NULL.
 */
void ED_fileselect_params_to_userdef(struct SpaceFile *sfile);

/**
 * Return the File Browser area in which \a file_operator is active.
 */
struct ScrArea *ED_fileselect_handler_area_find(const struct wmWindow *win, const struct wmOperator *file_operator);
/**
 * Check if there is any area in \a win that acts as a modal File Browser (#SpaceFile.op is set)
 * and return it.
 */
struct ScrArea *ED_fileselect_handler_area_find_any_with_op(const struct wmWindow *win);

#ifdef __cplusplus
}
#endif

#endif	// !ED_FILESELECT_H
