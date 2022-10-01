#pragma once

#include "gpu/gpu_vertex_format.h"

struct GPU_VertFormat;

void gpu_vertex_format_pack ( struct GPU_VertFormat *format );
void gpu_vertex_format_texture_buffer_pack ( struct GPU_VertFormat *format );
unsigned int vertex_buffer_size ( const struct GPU_VertFormat *format , unsigned int vertex_len );
