#ifndef DRW_ENGINE_H
#define DRW_ENGINE_H

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
/** \name Draw Engines
 * \{ */

void DRW_engines_register(void);
void DRW_engines_register_experimental(void);
void DRW_engines_free(void);

bool DRW_engine_render_support(struct DrawEngineType *draw_engine_type);
void DRW_engine_register(struct DrawEngineType *draw_engine_type);

struct DrawEngineType *DRW_engine_find(const char *name);
struct DrawEngineType *DRW_engine_type(const struct rContext *C, struct Scene *scene);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw
 * \{ */

void DRW_draw_view(const struct rContext *C);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // DRW_ENGINE_H
