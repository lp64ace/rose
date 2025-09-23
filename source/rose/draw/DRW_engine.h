#ifndef DRW_ENGINE_H
#define DRW_ENGINE_H

#include <stdbool.h>

struct Context;
struct DrawEngineType;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Draw Engines
 * \{ */

void DRW_engines_register(void);
void DRW_engines_register_experimental(void);
void DRW_engines_free(void);

bool DRW_engine_render_support(struct DrawEngineType *draw_engine_type);
void DRW_engine_register(struct DrawEngineType *draw_engine_type);

struct DrawEngineType *DRW_engine_find(const char *name);
struct DrawEngineType *DRW_engine_type(const struct Context *C, struct Scene *scene);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw
 * \{ */

void DRW_draw_view(const struct Context *C);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // DRW_ENGINE_H
