#ifndef DRW_ENGINE_H
#define DRW_ENGINE_H

#include "GPU_primitive.h"

#include "LIB_listbase.h"
#include "LIB_thread.h"
#include "LIB_utildefines.h"

struct DrawData;
struct DrawDataList;
struct DrawEngineType;
struct DrawInstanceDataList;
struct GPUBatch;
struct GPUUniformBuf;
struct GPUVertFormat;
struct Mesh;
struct Scene;
struct Object;
struct rContext;
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

void DRW_engine_register(struct DrawEngineType *draw_engine_type);

struct DrawEngineType *DRW_engine_find(const char *name);
struct DrawEngineType *DRW_engine_type(const struct rContext *C, struct Scene *scene);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw
 * \{ */

struct GPUShader;

struct DRWPass *DRW_pass_new_ex(const char *name, struct DRWPass *original, int state);
struct DRWPass *DRW_pass_new(const char *name, int state);

struct DRWShadingGroup *DRW_shading_group_new(struct GPUShader *shader, struct DRWPass *pass);

void DRW_shading_group_clear_ex(struct DRWShadingGroup *shgroup, unsigned char bits, const unsigned char rgba[4], float depth, unsigned char stencil);
void DRW_shading_group_call_ex(struct DRWShadingGroup *shgroup, struct Object *ob, const float (*obmat)[4], struct GPUBatch *batch);
void DRW_shading_group_call_range_ex(struct DRWShadingGroup *shgroup, struct Object *ob, const float (*obmat)[4], struct GPUBatch *batch, unsigned int vfirst, unsigned int vcount);
	/** Not to be confused with shading group uniforms this will be bound in order. */
void DRW_shading_group_bind_uniform_block(struct DRWShadingGroup *shgroup, struct GPUUniformBuf *block, unsigned int location);

struct DRWCallBuffer *DRW_shading_group_call_buffer(struct DRWShadingGroup *shgroup, struct GPUVertFormat *format, PrimType prim_type);
struct DRWCallBuffer *DRW_shading_group_call_buffer_instance(struct DRWShadingGroup *shgroup, struct GPUVertFormat *format, struct GPUBatch *batch);

struct DRWData *DRW_viewport_data_new(void);
void DRW_viewport_data_free(struct DRWData *);

void DRW_buffer_add_entry_struct(struct DRWCallBuffer *callbuf, const void *data);
void DRW_buffer_add_entry_array(struct DRWCallBuffer *callbuf, const void *attr[], size_t attr_len);

struct DRWInstanceDataList *DRW_instance_data_list_create(void);
void DRW_instance_data_list_free(struct DRWInstanceDataList *list);

void DRW_draw_view(const struct rContext *C);
void DRW_draw_pass(struct DRWPass *ps);
void DRW_draw_pass_range(struct DRWPass *ps, struct DRWShadingGroup *first, struct DRWShadingGroup *last);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // DRW_ENGINE_H
