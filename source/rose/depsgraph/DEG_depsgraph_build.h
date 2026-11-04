#ifndef DEG_DEPSGRAPH_BUILD_H
#define DEG_DEPSGRAPH_BUILD_H

struct Depsgraph;
struct DepsNodeHandle;
struct Object;
struct Main;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Graph Building
 * \{ */

void DEG_graph_relations_update(struct Depsgraph *graph);
void DEG_relations_tag_update(struct Main *bmain);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Graph Dependencies
 * \{ */

typedef enum eDepsObjectComponentType {
	/* Used in query API, to denote which component caller is interested in. */
	DEG_OB_COMP_ANY,

	/**
	 * Parameters Component - Default when nothing else fits
	 * (i.e. just SDNA property setting).
	 */
	DEG_OB_COMP_PARAMETERS,

	/* Animation Component. */
	DEG_OB_COMP_ANIMATION,
	/* Transform Component (Parenting/Constraints) */
	DEG_OB_COMP_TRANSFORM,
	/* Geometry Component (#Mesh / #DispList). */
	DEG_OB_COMP_GEOMETRY,

	/* Evaluation-Related Outer Types (with Sub-data) */

	/* Pose Component - Owner/Container of Bones Eval */
	DEG_OB_COMP_EVAL_POSE,
	/* Bone Component - Child/Sub-component of Pose */
	DEG_OB_COMP_BONE,
} eDepsObjectComponentType;

void DEG_add_object_relation(struct DepsNodeHandle *node_handle, struct Object *object, eDepsObjectComponentType component, const char *description);

/**
 * Special function which is used from modifiers' #update_depsgraph() callback
 * to indicate that the modifier needs to know transformation of the object
 * which that modifier belongs to.
 * This function will take care of checking which operation is required to
 * have transformation for the modifier, taking into account possible simulation solvers.
 */
void DEG_add_depends_on_transform_relation(struct DepsNodeHandle *node_handle, const char *description);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// !DEG_DEPSGRAPH_BUILD_H
