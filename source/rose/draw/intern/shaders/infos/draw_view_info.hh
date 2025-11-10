#include "intern/draw_defines.h"

/* -------------------------------------------------------------------- */
/** \name Resource ID
 *
 * This is used to fetch per object data in drw_matrices and other object indexed
 * buffers. There is multiple possibilities depending on how we are drawing the object.
 *
 * \{ */

/* Variation used when drawing multiple instances for one object. */
GPU_SHADER_CREATE_INFO(draw_resource_id_uniform)
    .define("UNIFORM_RESOURCE_ID")
    .push_constant(Type::INT, "drw_ResourceID");

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw View
 * \{ */

GPU_SHADER_CREATE_INFO(draw_view)
    .typedef_source("draw_shader_shared.h");
 
GPU_SHADER_CREATE_INFO(draw_modelmat)
    .uniform_buf(DRW_OBJ_MAT_UBO_SLOT, "ObjectMatrices", "drw_matrices[DRW_RESOURCE_CHUNK_LEN]", Frequency::BATCH)
    .define("ModelMatrix", "(drw_matrices[resource_id].drw_modelMatrix)")
    .define("ModelMatrixInverse", "(drw_matrices[resource_id].drw_modelMatrixInverse)")
    .define("ArmatureMatrix", "(drw_matrices[resource_id].drw_armatureMatrix)")
    .define("ArmatureMatrixInverse", "(drw_matrices[resource_id].drw_armatureMatrixInverse)")
    .additional_info("draw_view");

/** \} */

GPU_SHADER_CREATE_INFO(draw_mesh)
	.uniform_buf(DRW_DVGROUP_UBO_SLOT, "DVertGroupMatrices", "grp_matrices[DRW_RESOURCE_BONES_LEN]", Frequency::BATCH)
	.additional_info("draw_modelmat", "draw_resource_id_uniform");
