#pragma once

struct WorkSpace;
struct WorkSpaceLayout;
struct Screen;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Create, Delete, Initialize
 * \{ */

struct WorkSpace *KER_workspace_new(struct Main *main, const char *name);

/**
 * This is a higher-level wrapper that calls #workspace_free_data (through #KER_id_free) to free the workspace data, 
 * and frees other data-blocks owned by #workspace and its layouts, currently that is screens only.
 */
void KER_workspace_free(struct Main *main, struct WorkSpace *workspace);

struct WorkSpaceLayout *KER_workspace_layout_new(struct Main *main, struct WorkSpace *workspace, struct Screen *screen, const char *name);

/**
 * This will remove the #layout from the specified #workspace and then free it.
 */
void KER_workspace_layout_free(struct Main *main, struct WorkSpace *workspace, struct WorkSpaceLayout *layout);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Relations Create, Delete, Initialize
 * \{ */

struct WorkSpaceInstanceHook *KER_workspace_instance_hook_create(const struct Main *main, int winid);
void KER_workspace_instance_hook_free(const struct Main *main, struct WorkSpaceInstanceHook *hook);

void KER_workspace_relations_free(struct ListBase *relations);

/* \} */

/* -------------------------------------------------------------------- */
/** \name General Utilities
 * \{ */

struct WorkSpaceLayout *KER_workspace_layout_find(const struct WorkSpace *workspace, struct Screen *screen);
struct WorkSpaceLayout *KER_workspace_layout_find_global(const struct Main *main, const struct Screen *screen, struct WorkSpace **r_workspace);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Getters/Setters
 * \{ */

struct WorkSpace *KER_workspace_active_get(struct WorkSpaceInstanceHook *hook);
struct WorkSpaceLayout *KER_workspace_active_layout_get(struct WorkSpaceInstanceHook *hook);
struct Screen *KER_workspace_active_screen_get(struct WorkSpaceInstanceHook *hook);
struct Screen *KER_workspace_layout_screen_get(struct WorkSpaceLayout *layout);

const char *KER_workspace_layout_name_get(struct WorkSpaceLayout *layout);

void KER_workspace_active_set(struct WorkSpaceInstanceHook *hook, struct WorkSpace *workspace);
void KER_workspace_active_layout_set(struct WorkSpaceInstanceHook *hook, int winid, struct WorkSpace *workspace, struct WorkSpaceLayout *layout);
void KER_workspace_active_screen_set(struct WorkSpaceInstanceHook *hook, int winid, struct WorkSpace *workspace, struct Screen *screen);

void KER_workspace_layout_name_set(struct WorkSpace *workspace, struct WorkSpaceLayout *layout, const char *name);

struct WorkSpaceLayout *KER_workspace_active_layout_for_workspace_get(struct WorkSpaceInstanceHook *hook, struct WorkSpace *workspace);

/* \} */

#ifdef __cplusplus
}
#endif
