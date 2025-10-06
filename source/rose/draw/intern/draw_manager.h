#include "draw_engine.h"
#include "draw_state.h"

/* -------------------------------------------------------------------- */
/** \name Draw Manager Structure
 * \{ */

typedef struct DRWData {
	struct MemPool *commands;
	struct MemPool *passes;
	struct MemPool *shgroups;
	struct MemPool *uniforms;

	struct MemBlock *obmats;

	struct DRWViewData *vdata_engine[2];

	struct GPUUniformBuf **matrices_ubo;

	size_t ubo_length;
} DRWData;

typedef struct DRWManager {
	struct DRWData *vdata_pool;
	struct DRWViewData *vdata_engine;
	struct Scene *scene;

	float size[2];

	/**
	 * \brief The native system rendering context, see WM_render_*!
	 * We should in no way include GTK here (\type GTKRender *)!
	 */
	void *render;
	struct GPUContext *context;
	struct GPUViewport *viewport;

	ThreadMutex *mutex;

	/** This is reset each time we render so that we re-evaluate the resource data! */
	DRWResourceHandle resource_handle;
	DRWResourceHandle objcache_handle;
} DRWManager;

extern DRWManager GDrawManager;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw Engine Data
 * \{ */

typedef struct DRWShadingGroup DRWShadingGroup;

typedef struct DRWPass {
	/**
	 * \type DRWShadingGroup each pass consists of multiple shading group rendering passes!
	 * Each group commands are executed in order.
	 */
	ListBase groups;

	/**
	 * Draw the shading group commands of the original pass instead,
	 * this way we avoid duplicating drawcalls and shading group commands for similar passes!
	 */
	struct DRWPass *original;
	/** Link list of additional passes to render! */
	struct DRWPass *next;

	char name[64];

	DRWState state;
} DRWPass;

/** \} */
