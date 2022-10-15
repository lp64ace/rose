#pragma once

#include "lib/lib_span.h"
#include "lib/lib_string.h"

#include "gpu/gpu_shader.h"

#include "gpu_shader_create_info.h"
#include "gpu_shader_interface.h"

#include <sstream>
#include <string>
#include <cmath>

namespace rose {
namespace gpu {

class Shader {
public:
	/** Uniform & attribute locations for shader. */
	ShaderInterface *mInterface = nullptr;
public:
	Shader ( const char *name );
	virtual ~Shader ( );

	virtual void VertexShader ( MutableSpan<const char *> sources ) = 0;
	virtual void GeometryShader ( MutableSpan<const char *> sources ) = 0;
	virtual void FragmentShader ( MutableSpan<const char *> sources ) = 0;
	virtual void ComputeShader ( MutableSpan<const char *> sources ) = 0;

	virtual bool Finalize ( const ShaderCreateInfo *info = nullptr ) = 0;

	virtual void Bind ( ) = 0;
	virtual void Unbind ( ) = 0;

	virtual void UniformFloat ( int location , int comp , int size , const float *data ) = 0;
	virtual void UniformInt ( int location , int comp , int size , const int *data ) = 0;

	std::string DefinesDeclare ( const ShaderCreateInfo &info ) const;
	virtual std::string ResourcesDeclare ( const ShaderCreateInfo &info ) const = 0;
	virtual std::string VertexInterfaceDeclare ( const ShaderCreateInfo &info ) const = 0;
	virtual std::string FragmentInterfaceDeclare ( const ShaderCreateInfo &info ) const = 0;
	virtual std::string GeometryInterfaceDeclare ( const ShaderCreateInfo &info ) const = 0;
	virtual std::string GeometryLayoutDeclare ( const ShaderCreateInfo &info ) const = 0;
	virtual std::string ComputeLayoutDeclare ( const ShaderCreateInfo &info ) const = 0;

	virtual int GetProgramHandle ( ) const = 0;

	inline const char *GetName ( )const { return this->mName; }
private:
	char mName [ 64 ];
};

/* Syntactic sugar. */
static inline GPU_Shader *wrap ( Shader *vert ) {
	return reinterpret_cast< GPU_Shader * >( vert );
}
static inline Shader *unwrap ( GPU_Shader *vert ) {
	return reinterpret_cast< Shader * >( vert );
}
static inline const Shader *unwrap ( const GPU_Shader *vert ) {
	return reinterpret_cast< const Shader * >( vert );
}

}
}
