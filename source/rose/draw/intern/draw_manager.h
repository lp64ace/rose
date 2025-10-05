#include "draw_engine.h"

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

	struct GPUUniform **matrices_ubo;

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
