#pragma once

#include "gpu/gpu_batch.h"

#include "gl_shader_interface.h"

namespace rose {
namespace gpu {

namespace GLVertArray {

// Update the Attribute Binding of the currently bound VAO.
void UpdateBindings ( const unsigned int vao , const GPU_Batch *batch , const ShaderInterface *interface , int base_instance );

// Another version of UpdateBindings for Immediate mode.
void UpdateBindings ( const unsigned int vao , unsigned int v_first , const GPU_VertFormat *format , const ShaderInterface *interface );

}

}
}
