#pragma once

#include "LIB_sys_types.h"

#include "GPU_primitive.h"
#include "GPU_shader_builtin.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** Returns a cleared vertex format, ready to add attributes. */
struct GPUVertFormat *immVertexFormat(void);

/** Every #immBegin must have a program bound first. */
void immBindShader(struct GPUShader *shader);
/** Call after your last immEnd, or before binding another program. */
void immUnbindProgram(void);

/** Must supply exactly #vertex_length vertices. */
void immBegin(PrimType prim, uint vertex_length);
/** Can supply at most #vertex_length vertices, fewer vertices are still valid. */
void immBeginAtMost(PrimType prim, uint vertex_length);
void immEnd(void);

/**
 * - #immBegin a batch, then use standard `imm*` functions as usual.
 * - #immEnd will finalize the batch instead of drawing.
 *
 * Then you can draw it as many times as you like!
 * Partially replaces the need for display lists.
 */
struct GPUBatch *immBeginBatch(PrimType primitive, uint vertex_length);
struct GPUBatch *immBeginBatchAtMost(PrimType primitive, uint vertex_length);

void immAttr1f(uint attr_id, float x);
void immAttr2f(uint attr_id, float x, float y);
void immAttr3f(uint attr_id, float x, float y, float z);
void immAttr4f(uint attr_id, float x, float y, float z, float w);

void immAttr2i(uint attr_id, int x, int y);

void immAttr1u(uint attr_id, uint x);

void immAttr2s(uint attr_id, short x, short y);

void immAttr2fv(uint attr_id, const float data[2]);
void immAttr3fv(uint attr_id, const float data[3]);
void immAttr4fv(uint attr_id, const float data[4]);

void immAttr3ub(uint attr_id, unsigned char r, unsigned char g, unsigned char b);
void immAttr4ub(uint attr_id, unsigned char r, unsigned char g, unsigned char b, unsigned char a);

void immAttr3ubv(uint attr_id, const unsigned char data[3]);
void immAttr4ubv(uint attr_id, const unsigned char data[4]);

void immAttrSkip(uint attr_id);

void immVertex2f(uint attr_id, float x, float y);
void immVertex3f(uint attr_id, float x, float y, float z);
void immVertex4f(uint attr_id, float x, float y, float z, float w);

void immVertex2i(uint attr_id, int x, int y);

void immVertex2s(uint attr_id, short x, short y);

void immVertex2fv(uint attr_id, const float data[2]);
void immVertex3fv(uint attr_id, const float data[3]);

void immVertex2iv(uint attr_id, const int data[2]);

void immUniform1i(const char *name, int x);
void immUniform1f(const char *name, float x);
void immUniform2f(const char *name, float x, float y);
void immUniform2fv(const char *name, const float data[2]);
void immUniform3f(const char *name, float x, float y, float z);
void immUniform3fv(const char *name, const float data[3]);
void immUniform4f(const char *name, float x, float y, float z, float w);
void immUniform4fv(const char *name, const float data[4]);

void immUniformArray4fv(const char *bare_name, const float *data, int count);
void immUniformMatrix4fv(const char *name, const float data[4][4]);

void immBindTexture(const char *name, struct GPUTexture *tex);
void immBindTextureSampler(const char *name, struct GPUTexture *tex, struct GPUSamplerState state);
void immBindUniformBuf(const char *name, struct GPUUniformBuf *ubo);

void immUniformColor4f(float r, float g, float b, float a);
void immUniformColor4fv(const float rgba[4]);
void immUniformColor3f(float r, float g, float b);
void immUniformColor3fv(const float rgb[3]);
void immUniformColor3fvAlpha(const float rgb[3], float a);

void immUniformColor3ub(unsigned char r, unsigned char g, unsigned char b);
void immUniformColor4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void immUniformColor3ubv(const unsigned char rgb[3]);
void immUniformColor3ubvAlpha(const unsigned char rgb[3], unsigned char a);
void immUniformColor4ubv(const unsigned char rgba[4]);

/** Same as #immBindShader but uses a builtin shader instead. */
void immBindBuiltinProgram(BuiltinShader shader_id);

#if defined(__cplusplus)
}
#endif
