#ifndef DRW_ENGINE_H
#define DRW_ENGINE_H

#include "LIB_listbase.h"
#include "LIB_thread.h"

#include <stdbool.h>

struct rContext;
struct DrawEngineType;
struct Mesh;
struct Scene;
struct WindowManager;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Draw Init/Exit
 * \{ */

void DRW_render_context_create(struct WindowManager *wm);
void DRW_render_context_destroy(struct WindowManager *wm);

void DRW_render_context_enable();
void DRW_render_context_disable();
void DRW_render_context_enable_ex(bool restore);
void DRW_render_context_disable_ex(bool restore);

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
} DRWPass;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw Engines
 * \{ */

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
} DRWManager;

void DRW_engines_register(void);
void DRW_engines_register_experimental(void);
void DRW_engines_free(void);

void DRW_engine_register(struct DrawEngineType *draw_engine_type);

struct DrawEngineType *DRW_engine_find(const char *name);
struct DrawEngineType *DRW_engine_type(const struct rContext *C, struct Scene *scene);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw
 * \{ */

struct GPUShader;

struct DRWPass *DRW_pass_new_ex(const char *name, struct DRWPass *original);
struct DRWPass *DRW_pass_new(const char *name);

struct DRWShadingGroup *DRW_shading_group_new(struct GPUShader *shader, struct DRWPass *pass);

void DRW_shading_group_call_ex(DRWShadingGroup *shgroup, struct GPUBatch *batch);
void DRW_shading_group_call_range_ex(DRWShadingGroup *shgroup, struct GPUBatch *batch, unsigned int vfirst, unsigned int vcount);

struct DRWData *DRW_viewport_data_new(void);
void DRW_viewport_data_free(struct DRWData *);

void DRW_draw_view(const struct rContext *C);
void DRW_draw_pass(struct DRWPass *ps);
void DRW_draw_pass_range(struct DRWPass *ps, struct DRWShadingGroup *first, struct DRWShadingGroup *last);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // DRW_ENGINE_H
