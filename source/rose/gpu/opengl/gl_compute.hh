#pragma once

namespace rose::gpu {

class GLCompute {
public:
	static void dispatch(unsigned int group_x_len, unsigned int group_y_len, unsigned int group_z_len);
};

}  // namespace rose::gpu
