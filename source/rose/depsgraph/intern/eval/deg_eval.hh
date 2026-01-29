#ifndef DEG_EVAL_HH
#define DEG_EVAL_HH

namespace rose::depsgraph {

struct Depsgraph;

/**
 * Evaluate all nodes tagged for updating,
 * \warning This is usually done as part of main loop, but may also be
 * called from frame-change update.
 *
 * \note Time sources should be all valid!
 */
void deg_evaluate_on_refresh(Depsgraph *graph);

}  // namespace rose::depsgraph

#endif	// !DEG_EVAL_HH
