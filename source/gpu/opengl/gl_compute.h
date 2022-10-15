#pragma once

namespace rose {
namespace gpu {

class GLCompute {
public:
	static void Dipsatch ( int group_x_len , int group_y_len , int group_z_len );
};

}
}
