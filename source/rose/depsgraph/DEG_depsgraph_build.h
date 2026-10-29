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

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// !DEG_DEPSGRAPH_BUILD_H
