#ifndef DEPSGRAPH_TAG_HH
#define DEPSGRAPH_TAG_HH

#include "intern/node/deg_node.hh"

struct ID;
struct Main;

namespace rose::depsgraph {

struct Depsgraph;

/* Get type of a node which corresponds to a ID_RECALC_GEOMETRY tag. */
NodeType geometry_tag_to_component(const ID *id);

/* Tag given ID for an update in all registered dependency graphs. */
void id_tag_update(Main *main, ID *id, int flag, eUpdateSource update_source);

/* Tag given ID for an update with in a given dependency graph. */
void graph_id_tag_update(Main *main, Depsgraph *graph, ID *id, int flag, eUpdateSource update_source);
void graph_tag_ids_for_visible_update(Depsgraph *graph);

}  // namespace rose::depsgraph

#endif	// DEPSGRAPH_TAG_HH
