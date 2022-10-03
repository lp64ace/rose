#include "gpu/gpu_shader.h"
#include "lib/lib_utildefines.h"

#include <assert.h>
#include <stdint.h>

#define MAX_DEFINE_LENGTH 256
#define MAX_EXT_DEFINE_LENGTH 512

const struct GPU_ShaderConfigData GPU_shader_cfg_data [ GPU_SHADER_CFG_LEN ] = {
	[GPU_SHADER_CFG_DEFAULT] = {
		.lib = "",
		.def = "",
	},
	[ GPU_SHADER_CFG_CLIPPED ] = {
		.lib = "",
		.def = "#define USE_WORLD_CLIP_PLANES\n",
	},
};

/* cache of built-in shaders (each is created on first use) */
static GPU_Shader *builtin_shaders [ GPU_SHADER_CFG_LEN ][ GPU_SHADER_BUILTIN_LEN ] = { { NULL } };

typedef struct {
	const char *name;
	const char *vert;
	// Optional.
	const char *geom;
	const char *frag;
	// Optional.
	const char *defs;

	const char *create_info;
	const char *clipped_create_info;
} GPU_ShaderStages;

static const GPU_ShaderStages builtin_shader_stages [ GPU_SHADER_BUILTIN_LEN ] = {
	[GPU_SHADER_DEFAULT_DIFFUSE] = {
		.name = "GPU_SHADER_DEFAULT_DIFFUSE",
		.create_info = "gpu_shader_default_diffuse",
	},
};

GPU_Shader *GPU_shader_get_builtin_shader_with_config ( GPUBuiltinShader shader , int sh_cfg ) {
	assert ( 0 <= shader && shader < GPU_SHADER_BUILTIN_LEN );
	assert ( 0 <= sh_cfg && sh_cfg < GPU_SHADER_CFG_LEN );
	GPU_Shader **sh_p = &builtin_shaders [ sh_cfg ][ shader ];

	if ( *sh_p == NULL ) {
		const GPU_ShaderStages *stages = &builtin_shader_stages [ shader ];

		/* common case */
		if ( sh_cfg == GPU_SHADER_CFG_DEFAULT ) {
			if ( stages->create_info != NULL ) {
				*sh_p = GPU_shader_create_from_info_name ( stages->create_info );
			} else {
				*sh_p = GPU_shader_create_from_arrays_named (
					stages->name ,
					{
					    .vert = ( const char *[ ] ){stages->vert, NULL},
					    .geom = ( const char *[ ] ){stages->geom, NULL},
					    .frag = ( const char *[ ] ){stages->frag, NULL},
					    .defs = ( const char *[ ] ){stages->defs, NULL},
					} );
			}
		} else if ( sh_cfg == GPU_SHADER_CFG_CLIPPED ) {
			/* In rare cases geometry shaders calculate clipping themselves. */
			if ( stages->clipped_create_info != NULL ) {
				*sh_p = GPU_shader_create_from_info_name ( stages->clipped_create_info );
			} else {
				const char *world_clip_lib = "";
				const char *world_clip_def = "#define USE_WORLD_CLIP_PLANES\n";
				*sh_p = GPU_shader_create_from_arrays_named (
					stages->name ,
					{
					    .vert = ( const char *[ ] ){world_clip_lib, stages->vert, NULL},
					    .geom = ( const char *[ ] ){stages->geom ? world_clip_lib : NULL, stages->geom, NULL},
					    .frag = ( const char *[ ] ){stages->frag, NULL},
					    .defs = ( const char *[ ] ){world_clip_def, stages->defs, NULL},
					} );
			}
		} else {
			assert ( 0 );
		}
	}

	return *sh_p;
}

GPU_Shader *GPU_shader_get_builtin_shader ( GPUBuiltinShader shader ) {
	return GPU_shader_get_builtin_shader_with_config ( shader , GPU_SHADER_CFG_DEFAULT );
}

void GPU_shader_free_builtin_shaders ( void ) {
	for ( int i = 0; i < GPU_SHADER_CFG_LEN; i++ ) {
		for ( int j = 0; j < GPU_SHADER_BUILTIN_LEN; j++ ) {
			if ( builtin_shaders [ i ][ j ] ) {
				GPU_shader_free ( builtin_shaders [ i ][ j ] );
				builtin_shaders [ i ][ j ] = NULL;
			}
		}
	}
}
