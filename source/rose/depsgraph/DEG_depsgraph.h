#ifndef DEG_DEPSGRAPH_H
#define DEG_DEPSGRAPH_H

#include "KER_lib_id.h"
#include "KER_main.h"
#include "KER_scene.h"

struct ID;
struct Main;
struct Scene;
struct ViewLayer;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Depsgraph Depsgraph;

/* -------------------------------------------------------------------- */
/** \name Creation & Deletion
 * \{ */

struct Depsgraph *DEG_graph_new(struct Main *main, struct Scene *scene, struct ViewLayer *layer);

/** Replace the "owner" pointers (currently Main/Scene/ViewLayer) of this depsgraph. */
void DEG_graph_move(struct Depsgraph *depsgraph, struct Main *main, struct Scene *scene, struct ViewLayer *layer);
void DEG_graph_free(struct Depsgraph *depsgraph);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Node Types Registry
 * \{ */

/** Register and free all node types. */
void DEG_register_node_types(void);
void DEG_free_node_types(void);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Update Tagging
 * \{ */

/** Tag dependency graph for updates when visible scenes/layers changes. */
void DEG_graph_tag_on_visible_update(struct Depsgraph *depsgraph, bool do_time);

/** Tag all dependency graphs for update when visible scenes/layers changes. */
void DEG_tag_on_visible_update(struct Main *main, bool do_time);

/** Tag given ID for an update in all the dependency graphs. */
void DEG_id_tag_update(struct ID *id, int flag);
void DEG_id_tag_update_ex(struct Main *main, struct ID *id, int flag);

void DEG_graph_id_tag_update(struct Main *main, struct Depsgraph *depsgraph, struct ID *id, int flag);

/**
 * Mark a particular data-block type as having changing.
 * This does not cause any updates but is used by external
 * render engines to detect if for example a data-block was removed.
 */
void DEG_graph_id_type_tag(struct Depsgraph *depsgraph, short id_type);
void DEG_id_type_tag(struct Main *main, short id_type);

/** Clear recalc flags after editors or renderers have handled updates. */
void DEG_ids_clear_recalc(Depsgraph *depsgraph, bool backup);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Evaluation
 * \{ */

bool DEG_is_active(const struct Depsgraph *depsgraph);
void DEG_make_active(struct Depsgraph *depsgraph);
void DEG_make_inactive(struct Depsgraph *depsgraph);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Editors Integration
 *
 * Mechanism to allow editors to be informed of depsgraph updates,
 * to do their own updates based on changes.
 * \{ */

typedef struct DEGEditorUpdateContext {
	struct Main *main;
	struct Depsgraph *depsgraph;
	struct Scene *scene;
	struct ViewLayer *view_layer;
} DEGEditorUpdateContext;

typedef void (*DEG_EditorUpdateIDCb)(const DEGEditorUpdateContext *update_ctx, struct ID *id);
typedef void (*DEG_EditorUpdateSceneCb)(const DEGEditorUpdateContext *update_ctx, bool updated);

/** Set callbacks which are being called when depsgraph changes. */
void DEG_editors_set_update_cb(DEG_EditorUpdateIDCb id_func, DEG_EditorUpdateSceneCb scene_func);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Graph Evaluation
 * \{ */

void DEG_evaluate_on_framechange(struct Depsgraph *graph);
void DEG_evaluate_on_refresh(struct Depsgraph *graph);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // DEG_DEPSGRAPH_H
