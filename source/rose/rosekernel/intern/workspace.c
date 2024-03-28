#include "DNA_screen_types.h"
#include "DNA_workspace_types.h"

#include "MEM_alloc.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_string.h"
#include "LIB_string_utils.h"
#include "LIB_utildefines.h"

#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_main.h"
#include "KER_workspace.h"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name ID Data-Block Management
 * \{ */

static void workspace_free_data(ID *id) {
	WorkSpace *workspace = (WorkSpace *)id;

	KER_workspace_relations_free(&workspace->hook_layout_relations);

	LIB_freelistN(&workspace->layouts);
}

static void workspace_foreach_id(ID *id, LibraryForeachIDData *data) {
	WorkSpace *workspace = (WorkSpace *)id;

	LISTBASE_FOREACH(WorkSpaceLayout *, layout, &workspace->layouts) {
		KER_LIB_FOREACHID_PROCESS_IDSUPER(data, layout->screen, IDWALK_CB_USER);
	}
}

IDTypeInfo IDType_ID_WS = {
	.id_code = ID_WS,
	.id_filter = FILTER_ID_WS,
	.main_listbase_index = INDEX_ID_WS,
	.struct_size = sizeof(WorkSpace),

	.name = "WorkSpace",
	.name_plural = "Workspaces",

	.init_data = NULL,
	.copy_data = NULL,
	.free_data = workspace_free_data,

	.foreach_id = workspace_foreach_id,
};

/* \} */

/* -------------------------------------------------------------------- */
/** \name Internal Utils
 * \{ */

static void workspace_layout_name_set(WorkSpace *workspace, WorkSpaceLayout *layout, const char *new_name) {
	LIB_strncpy(layout->name, new_name, ARRAY_SIZE(layout->name));

	LIB_uniquename_ensure(&workspace->layouts, layout, "Layout", '.', offsetof(WorkSpaceLayout, name), ARRAY_SIZE(layout->name));
}

static WorkSpaceLayout *workspace_layout_find_exec(const WorkSpace *workspace, const struct Screen *screen) {
	return (WorkSpaceLayout *)LIB_findptr(&workspace->layouts, screen, offsetof(WorkSpaceLayout, screen));
}

static void worksapce_relation_add(ListBase *relations, void *parent, const int parentid, void *data) {
	WorkSpaceDataRelation *relation = MEM_callocN(sizeof(WorkSpaceDataRelation), __func__);
	relation->parent = parent;
	relation->parentid = parentid;
	relation->value = data;

	/** Add to head, if we switch back to it soon we find it faster. */
	LIB_addhead(relations, relation);
}

static void workspace_relation_remove(ListBase *relations, WorkSpaceDataRelation *relation) {
	LIB_remlink(relations, relation);
	MEM_freeN(relation);
}

static void workspace_relation_ensure_updated(ListBase *relations, void *parent, const int parentid, void *data) {
	WorkSpaceDataRelation *relation = LIB_findbytes(relations, &parentid, sizeof(parentid), offsetof(WorkSpaceDataRelation, parentid));
	if (relation != NULL) {
		relation->parent = parent;
		relation->value = data;

		/** Reinsert at the head of the list, so that more commonly used relations are found faster. */
		LIB_remlink(relations, relation);
		LIB_addhead(relations, relation);
	}
	else {
		worksapce_relation_add(relations, parent, parentid, data);
	}
}

static void *workspace_relation_get_data_matching_parent(const ListBase *relations, const void *parent) {
	WorkSpaceDataRelation *relation = LIB_findptr(relations, parent, offsetof(WorkSpaceDataRelation, parent));
	return (relation) ? relation->value : NULL;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Create, Delete, Initialize
 * \{ */

struct WorkSpace *KER_workspace_new(struct Main *main, const char *name) {
	WorkSpace *workspace = KER_id_new(main, ID_WS, name);
	id_us_ensure_real(&workspace->id);
	return workspace;
}

void KER_workspace_free(struct Main *main, struct WorkSpace *workspace) {
	LISTBASE_FOREACH_MUTABLE(WorkSpaceLayout *, layout, &workspace->layouts) {
		KER_workspace_layout_free(main, workspace, layout);
	}
	KER_id_free(main, workspace);
}

struct WorkSpaceLayout *KER_workspace_layout_new(struct Main *main, struct WorkSpace *workspace, struct Screen *screen, const char *name) {
	WorkSpaceLayout *layout = MEM_callocN(sizeof(WorkSpaceLayout), "rose::kernel::WorkSpaceLayout");
	layout->screen = screen;
	id_us_plus(&layout->screen->id);
	workspace_layout_name_set(workspace, layout, name);
	LIB_addtail(&workspace->layouts, layout);
	return layout;
}

void KER_workspace_layout_free(struct Main *main, struct WorkSpace *workspace, struct WorkSpaceLayout *layout) {
	if (layout->screen) {
		id_us_min(&layout->screen->id);
		KER_id_free(main, layout->screen);
	}
	LIB_remlink(&workspace->layouts, layout);
	MEM_freeN(layout);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Relations Create, Delete, Initialize
 * \{ */

struct WorkSpaceInstanceHook *KER_workspace_instance_hook_create(const struct Main *main, int winid) {
	WorkSpaceInstanceHook *hook = MEM_callocN(sizeof(WorkSpaceInstanceHook), "rose::kernel::WorkSpaceInstanceHook");

	LISTBASE_FOREACH(WorkSpace *, workspace, &main->workspaces) {
		KER_workspace_active_layout_set(hook, winid, workspace, (workspace->layouts.first));
	}

	return hook;
}

void KER_workspace_instance_hook_free(const struct Main *main, struct WorkSpaceInstanceHook *hook) {
	LISTBASE_FOREACH(WorkSpace *, workspace, &main->workspaces) {
		LISTBASE_FOREACH_MUTABLE(WorkSpaceDataRelation *, relation, &workspace->hook_layout_relations) {
			if (relation->parent == hook) {
				workspace_relation_remove(&workspace->hook_layout_relations, relation);
			}
		}
	}

	MEM_freeN(hook);
}

void KER_workspace_relations_free(struct ListBase *relations) {
	LISTBASE_FOREACH(WorkSpaceDataRelation *, relation, relations) {
		workspace_relation_remove(relations, relation);
	}
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name General Utilities
 * \{ */

struct WorkSpaceLayout *KER_workspace_layout_find(const struct WorkSpace *workspace, struct Screen *screen) {
	WorkSpaceLayout *layout = workspace_layout_find_exec(workspace, screen);
	if (layout) {
		return layout;
	}

	printf("%s: Couldn't find layout in the '%s' workspace with screen '%s'.", __func__, workspace->id.name + 2, screen->id.name + 2);
	return NULL;
}

struct WorkSpaceLayout *KER_workspace_layout_find_global(const struct Main *main, const struct Screen *screen, struct WorkSpace **r_workspace) {
	WorkSpaceLayout *layout;

	if (r_workspace) {
		*r_workspace = NULL;
	}

	LISTBASE_FOREACH(WorkSpace *, workspace, &main->workspaces) {
		if ((layout = workspace_layout_find_exec(workspace, screen)) != NULL) {
			if (r_workspace) {
				*r_workspace = workspace;
			}
			return layout;
		}
	}

	return NULL;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Getters/Setters
 * \{ */

struct WorkSpace *KER_workspace_active_get(struct WorkSpaceInstanceHook *hook) {
	return hook->active;
}

struct WorkSpaceLayout *KER_workspace_active_layout_get(struct WorkSpaceInstanceHook *hook) {
	return hook->active_layout;
}

struct Screen *KER_workspace_active_screen_get(struct WorkSpaceInstanceHook *hook) {
	return (hook->active_layout) ? hook->active_layout->screen : NULL;
}

struct Screen *KER_workspace_layout_screen_get(struct WorkSpaceLayout *layout) {
	return layout->screen;
}

const char *KER_workspace_layout_name_get(struct WorkSpaceLayout *layout) {
	return layout->name;
}

void KER_workspace_active_set(struct WorkSpaceInstanceHook *hook, struct WorkSpace *workspace) {
	if ((hook->active = workspace) != NULL) {
		WorkSpaceLayout *layout = (WorkSpaceLayout *)workspace_relation_get_data_matching_parent(&workspace->hook_layout_relations, hook);
		if (layout) {
			hook->active_layout = layout;
		}
	}
}

void KER_workspace_active_layout_set(struct WorkSpaceInstanceHook *hook, int winid, struct WorkSpace *workspace, struct WorkSpaceLayout *layout) {
	hook->active_layout = layout;
	workspace_relation_ensure_updated(&workspace->hook_layout_relations, hook, winid, layout);
}

void KER_workspace_active_screen_set(struct WorkSpaceInstanceHook *hook, int winid, struct WorkSpace *workspace, struct Screen *screen) {
	WorkSpaceLayout *layout = KER_workspace_layout_find(hook->active, screen);
	KER_workspace_active_layout_set(hook, winid, workspace, layout);
}

void KER_workspace_layout_name_set(struct WorkSpace *workspace, struct WorkSpaceLayout *layout, const char *name) {
	workspace_layout_name_set(workspace, layout, name);
}

struct WorkSpaceLayout *KER_workspace_active_layout_for_workspace_get(struct WorkSpaceInstanceHook *hook, struct WorkSpace *workspace) {
	if (hook->active == workspace) {
		return hook->active_layout;
	}

	return workspace_relation_get_data_matching_parent(&workspace->hook_layout_relations, hook);
}

/* \} */
