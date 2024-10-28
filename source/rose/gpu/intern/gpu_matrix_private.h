#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

struct GPUMatrixState *GPU_matrix_state_create(void);

void GPU_matrix_state_discard(struct GPUMatrixState *);

#if defined(__cplusplus)
}
#endif
