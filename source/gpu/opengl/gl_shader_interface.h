#pragma once

#include "lib/lib_alloc.h"
#include "lib/lib_vector.h"

#include "gpu/intern/gpu_shader_create_info.h"
#include "gpu/intern/gpu_shader_interface.h"

namespace rose {
namespace gpu {

class GLVaoCache;

class GLShaderInterface : public ShaderInterface {
private:
	/** Reference to VaoCaches using this interface */
	Vector<GLVaoCache *> mRefs;
public:
	GLShaderInterface ( unsigned int program , const ShaderCreateInfo &info );
	GLShaderInterface ( unsigned int program );
	~GLShaderInterface ( );

	void RefAdd ( GLVaoCache *ref );
	void RefRemove ( GLVaoCache *ref );
};

}
}
