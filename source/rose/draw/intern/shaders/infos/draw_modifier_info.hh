#include "intern/draw_defines.h"

GPU_SHADER_CREATE_INFO(draw_modifier)
	.typedef_source("draw_shader_shared.h");

GPU_SHADER_CREATE_INFO(draw_compute_armature)
    .local_group_size(64)
	.uniform_buf(DRW_DVGROUP_UBO_SLOT, "DVertGroupMatrices", "grp_matrices", Frequency::BATCH)
    .define("TargetToArmatureMatrix", "(grp_matrices.drw_TargetToArmature)")
    .define("ArmatureToTargetMatrix", "(grp_matrices.drw_ArmatureToTarget)")
    .storage_buf(0, Qualifier::READ, "float[]", "mpos")
    .storage_buf(1, Qualifier::WRITE, "float[]", "pos")
    .storage_buf(2, Qualifier::READ, "uint[]", "mnor")
    .storage_buf(3, Qualifier::WRITE, "uint[]", "nor")
    .storage_buf(4, Qualifier::READ, "MDeformDeviceData[]", "deform")
    .compute_source("draw_shader_armature_compute.glsl");

GPU_SHADER_CREATE_INFO(draw_armature_modifier)
    .additional_info("draw_modifier")
    .additional_info("draw_compute_armature")
    .do_static_compilation(true);
