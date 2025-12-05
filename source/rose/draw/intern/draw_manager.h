#include "draw_pass.h"

/* -------------------------------------------------------------------- */
/** \name Draw Manager Structure
 * \{ */

typedef struct DRWData {
	struct MemBlock *commands;
	struct MemBlock *passes;
	struct MemBlock *shgroups;
	struct MemBlock *uniforms;

	struct MemBlock *obmats;
	struct MemBlock *dvinfo;

	struct DRWViewData *vdata_engine[2];

	struct GPUUniformBuf **matrices_ubo;

	size_t ubo_length;
} DRWData;

typedef struct DRWManager {
	struct DRWData *vdata_pool;
	struct DRWViewData *vdata_engine;

	/** Evaluated Scene. */
	struct Scene *scene;
	/** Evaluated ViewLayer. */
	struct ViewLayer *view_layer;

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

typedef struct DRWGlobal {
	struct GPUUniformBuf *view;
} DRWGlobal;

extern DRWGlobal GDraw;

/** \} */
