#pragma once

#include "gpu/intern/gpu_shader_private.h"

#include "gl_shader_interface.h"

namespace rose {
namespace gpu {

class GLShader : public Shader {
	// Handle for full program (links shader stages below).
	unsigned int mShaderProgram = 0;
	// Individual shader stages.
	unsigned int mVertShader = 0;
	unsigned int mFragShader = 0;
	unsigned int mGeomShader = 0;
	unsigned int mCompShader = 0;
	// True if any shader failed to compile.
	bool mCompilationFailed = false;
public:
	GLShader ( const char *name );
	~GLShader ( );

	void VertexShader ( MutableSpan<const char *> sources );
	void GeometryShader ( MutableSpan<const char *> sources );
	void FragmentShader ( MutableSpan<const char *> sources );
	void ComputeShader ( MutableSpan<const char *> sources );

	bool Finalize ( const ShaderCreateInfo *info = nullptr );

	void Bind ( );
	void Unbind ( );

	void UniformFloat ( int location , int comp , int size , const float *data );
	void UniformInt ( int location , int comp , int size , const int *data );

	std::string ResourcesDeclare ( const ShaderCreateInfo &info ) const;
	std::string VertexInterfaceDeclare ( const ShaderCreateInfo &info ) const;
	std::string FragmentInterfaceDeclare ( const ShaderCreateInfo &info ) const;
	std::string GeometryInterfaceDeclare ( const ShaderCreateInfo &info ) const;
	std::string GeometryLayoutDeclare ( const ShaderCreateInfo &info ) const;
	std::string ComputeLayoutDeclare ( const ShaderCreateInfo &info ) const;

	inline int GetProgramHandle ( ) const { return this->mShaderProgram; }
	inline bool IsCompute ( ) const { return this->mCompShader != 0; }
private:
	char *GetPatch ( unsigned int gl_stage );

	/** Create, compile and attach the shader stage to the shader program. */
	unsigned int CreateShaderStage ( unsigned int gl_stage , MutableSpan<const char *> sources );

	/**
	* \brief features available on newer implementation such as native barycentric coordinates
	* and layered rendering, necessitate a geometry shader to work on older hardware.
	*/
	std::string WorkaroundGeometryShaderSourceCreate ( const ShaderCreateInfo &info );

	bool DoGeometryShaderInjection ( const ShaderCreateInfo *info );
};

}
}
