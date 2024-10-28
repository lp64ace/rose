#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

struct GSet;

typedef enum {
	FUNCTION_QUAL_IN,
	FUNCTION_QUAL_OUT,
	FUNCTION_QUAL_INOUT,
} GPUFunctionQual;

typedef struct GPUFunction {
	char name[64];
	int paramtype[64];
	GPUFunctionQual paramqual[64];
	int totparam;
	/* TODO(@fclem): Clean that void pointer. */
	void *source; /* GPUSource */
} GPUFunction;

GPUFunction *gpu_material_library_use_function(struct GSet *used_libraries, const char *name);

#if defined(__cplusplus)
}
#endif
