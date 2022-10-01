#include "gpu/gpu_immediate.h"
#include "gpu/gpu_texture.h"

#include "gpu_context_private.h"
#include "gpu_immediate_private.h"
#include "gpu_shader_private.h"
#include "gpu_vertex_format_private.h"

using namespace rose::gpu;

static thread_local Immediate *imm = nullptr;


