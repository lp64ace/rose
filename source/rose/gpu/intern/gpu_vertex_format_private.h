#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

struct GPUVertFormat;

void VertexFormat_pack(struct GPUVertFormat *format);
void VertexFormat_texture_buffer_pack(struct GPUVertFormat *format);

unsigned int padding(unsigned int offset, unsigned int alignment);
unsigned int vertex_buffer_size(const struct GPUVertFormat *format, unsigned int vertex_length);

#if defined(__cplusplus)
}
#endif
