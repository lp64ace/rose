#include "gl_compute.h"

#include <gl/glew.h>

namespace rose {
namespace gpu {

void GLCompute::Dipsatch ( int group_x_len , int group_y_len , int group_z_len ) {
	glDispatchCompute ( group_x_len , group_y_len , group_z_len );
}

}
}
