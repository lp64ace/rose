#include "LIB_utildefines.h"

#include "node/deg_node_component.hh"
#include "node/deg_node.hh"
#include "node/deg_node_operation.hh"

#include "depsgraph_types.hh"

namespace rose::depsgraph {

DEGCustomDataMeshMasks::DEGCustomDataMeshMasks(const CustomData_MeshMasks *other) : vert_mask(other->vmask), edge_mask(other->emask), face_mask(other->fmask), loop_mask(other->lmask), poly_mask(other->pmask) {
}

}  // namespace rose::depsgraph

void DEG_register_node_types() {
	rose::depsgraph::deg_register_base_depsnodes();
	rose::depsgraph::deg_register_component_depsnodes();
	rose::depsgraph::deg_register_operation_depsnodes();
}

void DEG_free_node_types() {
}
