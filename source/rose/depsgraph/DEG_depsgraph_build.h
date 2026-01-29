#ifndef DEG_DEPSGRAPH_BUILD_H
#define DEG_DEPSGRAPH_BUILD_H

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Graph Building
 * \{ */

void DEG_graph_relations_update(struct Depsgraph *graph);
void DEG_relations_tag_update(struct Main *bmain);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// !DEG_DEPSGRAPH_BUILD_H
