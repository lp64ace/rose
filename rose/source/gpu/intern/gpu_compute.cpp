#include "gpu/gpu_compute.h"
#include "gpu_backend.h"

void GPU_compute_dispatch ( GPU_Shader *shader , unsigned int groups_x_len , unsigned int groups_y_len , unsigned int groups_z_len ) {
        rose::gpu::GPU_Backend &gpu_backend = *rose::gpu::GPU_Backend::Get ( );
        GPU_shader_bind ( shader );
        gpu_backend.ComputeDispatch ( groups_x_len , groups_y_len , groups_z_len );
}
