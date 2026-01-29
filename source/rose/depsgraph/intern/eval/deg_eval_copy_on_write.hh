#ifndef DEG_EVAL_COPY_ON_WRITE_HH
#define DEG_EVAL_COPY_ON_WRITE_HH

#include "DNA_ID.h"

struct Depsgraph;

namespace rose::depsgraph {

struct Depsgraph;
struct IDNode;

/**
 * Makes sure given CoW data-block is brought back to state of the original
 * data-block.
 */
ID *deg_update_copy_on_write_datablock(const struct Depsgraph *depsgraph, const struct IDNode *id_node);
ID *deg_update_copy_on_write_datablock(const struct Depsgraph *depsgraph, struct ID *id_orig);

/**
 * Callback function for depsgraph operation node which ensures copy-on-write
 * data-block is ready for use by further evaluation routines.
 */
void deg_evaluate_copy_on_write(struct ::Depsgraph *depsgraph, const struct IDNode *id_node);

/** Helper function which frees memory used by copy-on-written data-block. */
void deg_free_copy_on_write_datablock(struct ID *id_cow);

/** Tag given ID block as being copy-on-written. */
void deg_tag_copy_on_write_id(struct ID *id_cow, const struct ID *id_orig);

/**
 * Check whether ID data-block is expanded.
 */
bool deg_copy_on_write_is_expanded(const struct ID *id_cow);

/**
 * Check whether copy-on-write data-block is needed for given ID.
 *
 * There are some exceptions on data-blocks which are covered by dependency graph
 * but which we don't want to start duplicating.
 *
 * This includes images.
 */
bool deg_copy_on_write_is_needed(const ID *id_orig);
bool deg_copy_on_write_is_needed(const ID_Type id_type);

}  // namespace rose::depsgraph

#endif	// !DEG_EVAL_COPY_ON_WRITE_HH
