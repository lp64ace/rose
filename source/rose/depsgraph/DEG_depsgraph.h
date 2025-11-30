#ifndef DEG_DEPSGRAPH_H
#define DEG_DEPSGRAPH_H

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
/** \name Update Tagging
 * \{ */

/** Tag given ID for an update in all the dependency graphs. */
void DEG_id_tag_update(struct ID *id, int flag);
void DEG_id_tag_update_ex(struct Main *main, struct ID *id, int flag);

void DEG_graph_id_tag_update(struct Main *main, struct Depsgraph *depsgraph, struct ID *id, int flag);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Graph Evaluation
 * \{ */
 
void DEG_evaluate_on_framechange(struct Depsgraph *graph, float frame);
void DEG_evaluate_on_refresh(struct Depsgraph *graph);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // DEG_DEPSGRAPH_H
