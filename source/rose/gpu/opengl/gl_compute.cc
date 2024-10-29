#include "gl_compute.hh"

#include <GL/glew.h>

namespace rose::gpu {

void GLCompute::dispatch(unsigned int group_x_len, unsigned int group_y_len, unsigned int group_z_len) {
	/* Sometime we reference a dispatch size but we want to skip it by setting one dimension to 0.
	 * Avoid error being reported on some implementation for these case. */
	if (group_x_len == 0 || group_y_len == 0 || group_z_len == 0) {
		return;
	}
	
	glDispatchCompute(group_x_len, group_y_len, group_z_len);
}

}  // namespace rose::gpu
