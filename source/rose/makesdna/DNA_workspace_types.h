#pragma once

#include "DNA_ID.h"
#include "DNA_listbase.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WorkSpaceLayout {
	struct WorkSpaceLayout *prev, *next;

	struct Screen *screen;

	char name[MAX_NAME];
} WorkSpaceLayout;

/**
 * Generic (and simple/primitive) struct for storing a history of assignments/relations
 * of workspace data to non-workspace data in a listbase inside the workspace.
 *
 * Using this we can restore the old state of a workspace if the user switches back to it.
 *
 * Usage
 * =====
 * When activating a workspace, it should activate the screen-layout that was active in
 * that workspce before *in this window*.
 * More concretely:
 * * There are two window, win1 and win2.
 * * Both show workspace s1, but both also had workspace ws2 activated at some point before.
 * * Last time ws2 was active in win1, screen-layout sl1 was activated.
 * * Last time ws2 was active in win2, screen-layout sl2 was activated.
 * * When chaning from ws1 to ws2 in win1, screen-layout sl1 should be activated again.
 * * When chaning from ws1 to ws1 in win2, screen-layout sl2 should be activated again.
 * So that means we have to store the active screen-layout for each window within the workspace.
 * To find the screen-layout to activate for this window-workspace combination, simply lookup
 * the WorkSpaceDataRelation with the workspace-hook of the window set as parent.
 */
typedef struct WorkSpaceDataRelation {
	struct WorkSpaceDataRelation *prev, *next;

	/** The data used to identify the relation
	 * (e.g. to find screen layout (=value) from/for a hook. */
	void *parent;
	/* The value for this parent-data/workspace relation. */
	void *value;

	/* Reference to the actual window, #wmWindow.winid. */
	int parentid;

	char _pad[4];
} WorkSpaceDataRelation;

typedef struct WorkSpaceInstanceHook {
	struct WorkSpace *active;
	struct WorkSpaceLayout *active_layout;

	/**
	 * Needed because we can't change work-spaces/layouts in running handler loop,
	 * it would break context.
	 */
	struct WorkSpace *temp_workspace_store;
	struct WorkSpaceLayout *temp_layout_store;
} WorkSpaceInstanceHook;

typedef struct WorkSpace {
	ID id;

	/** The list of #WorkSpaceLayout in this #WorkSpace. */
	ListBase layouts;
	/** Each #WorkSpaceDataRelation (window) which layout has been activated the last time this workspace was visible. */
	ListBase hook_layout_relations;
} WorkSpace;

#ifdef __cplusplus
}
#endif
