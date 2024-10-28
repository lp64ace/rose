#pragma once

#include "MEM_guardedalloc.h"

#include "LIB_vector.hh"

#include "intern/gpu_shader_create_info.hh"
#include "intern/gpu_shader_interface.hh"

namespace rose::gpu {

class GLVaoCache;

class GLShaderInterface : public ShaderInterface {
private:
	/** Reference to VaoCaches using this interface */
	Vector<GLVaoCache *> refs_;

public:
	GLShaderInterface(GLuint program, const shader::ShaderCreateInfo &info);
	GLShaderInterface(GLuint program);
	~GLShaderInterface();

	void ref_add(GLVaoCache *ref);
	void ref_remove(GLVaoCache *ref);
};

}
