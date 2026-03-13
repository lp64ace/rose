#include "LIB_bounds_types.hh"
#include "LIB_math_vector.h"
#include "LIB_math_vector_types.hh"

#include "KER_mesh.hh"
#include "KER_mesh_types.hh"
#include "KER_object.h"

void KER_boundbox_init_from_minmax(BoundBox *bb, const float min[3], const float max[3]) {
	bb->vec[0][0] = bb->vec[1][0] = bb->vec[2][0] = bb->vec[3][0] = min[0];
	bb->vec[4][0] = bb->vec[5][0] = bb->vec[6][0] = bb->vec[7][0] = max[0];

	bb->vec[0][1] = bb->vec[1][1] = bb->vec[4][1] = bb->vec[5][1] = min[1];
	bb->vec[2][1] = bb->vec[3][1] = bb->vec[6][1] = bb->vec[7][1] = max[1];

	bb->vec[0][2] = bb->vec[3][2] = bb->vec[4][2] = bb->vec[7][2] = min[2];
	bb->vec[1][2] = bb->vec[2][2] = bb->vec[5][2] = bb->vec[6][2] = max[2];
}

void KER_object_evaluated_geometry_bounds(Object *object, BoundBox **r_bb, bool use_subdivision) {
	std::optional<rose::Bounds<float3>> bounds;

	switch (object->type) {
		case OB_MESH: {
			Mesh *mesh = static_cast<Mesh *>(object->data);
			if (use_subdivision && mesh->runtime->mesh_eval) {
				std::optional<rose::Bounds<float3>> local = KER_mesh_evaluated_geometry_bounds(mesh->runtime->mesh_eval);
				bounds = rose::bounds::merge(bounds, local);
			}
			else {
				std::optional<rose::Bounds<float3>> local = KER_mesh_evaluated_geometry_bounds(mesh);
				bounds = rose::bounds::merge(bounds, local);
			}
		} break;
	}


	if (bounds.has_value()) {
		if (r_bb) {
			/** Reuse old pointer if possible! */
			BoundBox *bb = (!(*r_bb)) ? static_cast<BoundBox *>(MEM_mallocN(sizeof(BoundBox), "Object::BoundBox")) : *r_bb;

			KER_boundbox_init_from_minmax(bb, bounds->min, bounds->max);

			*r_bb = bb;
		}
	}
	else {
		MEM_SAFE_FREE(*r_bb);
	}
}

const BoundBox *KER_object_boundbox_get(Object *object) {
	KER_object_evaluated_geometry_bounds(object, &object->runtime.bb, true);

	return object->runtime.bb;
}
