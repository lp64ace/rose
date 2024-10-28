#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

struct GPUBatch *GPU_batch_preset_quad(void);

void gpu_batch_presets_init(void);
void gpu_batch_presets_register(struct GPUBatch *preset_batch);
void gpu_batch_presets_unregister(struct GPUBatch *preset_batch);
void gpu_batch_presets_exit(void);

#if defined(__cplusplus)
}
#endif
