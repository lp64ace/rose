#include "LIB_bounds_types.hh"
#include "LIB_math_vector.h"
#include "LIB_math_vector_types.hh"

#include "KER_mesh.hh"
#include "KER_mesh_types.hh"
#include "KER_object.h"

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

			copy_v3_v3(bb->min, static_cast<const float *>(bounds->min));
			copy_v3_v3(bb->max, static_cast<const float *>(bounds->max));

			*r_bb = bb;
		}
	}
	else {
		MEM_SAFE_FREE(*r_bb);
	}
}
