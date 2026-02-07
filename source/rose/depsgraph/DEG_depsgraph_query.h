#ifndef DEG_DEPSGRAPH_QUERY_H
#define DEG_DEPSGRAPH_QUERY_H

#include "DEG_depsgraph.h"

struct CustomData_MeshMasks;
struct Depsgraph;
struct Main;
struct Scene;
struct ViewLayer;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name DEG input data
 * \{ */

struct Scene *DEG_get_input_scene(const struct Depsgraph *graph);
struct ViewLayer *DEG_get_input_view_layer(const struct Depsgraph *graph);
struct Main *DEG_get_main(const struct Depsgraph *graph);

float DEG_get_ctime(const struct Depsgraph *graph);

/** \} */

/* -------------------------------------------------------------------- */
/** \name DEG evaluated data
 * \{ */

bool DEG_id_type_updated(const struct Depsgraph *depsgraph, short id_type);
bool DEG_id_type_any_updated(const struct Depsgraph *depsgraph);

struct Object *DEG_get_original_object(struct Object *object);
struct ID *DEG_get_original_id(struct ID *id);

/**
 * Check whether depsgraph is fully evaluated. This includes the following checks:
 * - Relations are up-to-date.
 * - Nothing is tagged for update.
 */
bool DEG_is_fully_evaluated(const struct Depsgraph *depsgraph);
/** Get additional mesh CustomData_MeshMasks flags for the given object. */
void DEG_get_customdata_mask_for_object(const struct Depsgraph *graph, struct Object *object, struct CustomData_MeshMasks *r_mask);

struct Scene *DEG_get_evaluated_scene(const struct Depsgraph *graph);
struct ViewLayer *DEG_get_evaluated_view_layer(const struct Depsgraph *graph);
struct Object *DEG_get_evaluated_object(const struct Depsgraph *graph, struct Object *object);
struct ID *DEG_get_evaluated_id(const struct Depsgraph *depsgraph, struct ID *id);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// !DEG_DEPSGRAPH_QUERY_H
