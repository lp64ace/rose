#pragma once

#include "lib/lib_string.h"
#include "lib/lib_string_ref.h"
#include "lib/lib_vector.h"
#include "lib/lib_types.h"

#include "gpu/gpu_texture.h"
#include "gpu/gpu_shader.h"

#include <iostream>

namespace rose {
namespace gpu {

enum class Type {
	FLOAT = ROSE_FLOAT ,
	VEC2 = ROSE_FLOAT_VEC2 ,
	VEC3 = ROSE_FLOAT_VEC3 ,
	VEC4 = ROSE_FLOAT_VEC4 ,
	MAT3 = ROSE_FLOAT_MAT3 ,
	MAT4 = ROSE_FLOAT_MAT4 ,

	UINT = ROSE_UNSIGNED_INT ,
	UVEC2 = ROSE_UNSIGNED_INT_VEC2 ,
	UVEC3 = ROSE_UNSIGNED_INT_VEC3 ,
	UVEC4 = ROSE_UNSIGNED_INT_VEC4 ,

	INT = ROSE_INT ,
	IVEC2 = ROSE_INT_VEC2 ,
	IVEC3 = ROSE_INT_VEC3 ,
	IVEC4 = ROSE_INT_VEC4 ,

	BOOL = ROSE_INT ,

	/* Additionally supported types to enable data optimization and native
	* support in some GPU back-ends.
	* NOTE: These types must be representable in all APIs. E.g. `VEC3_101010I2` is aliased as vec3
	* in the GL back-end, as implicit type conversions from packed normal attribute data to vec3 is
	* supported. UCHAR/CHAR types are natively supported in Metal and can be used to avoid
	* additional data conversions for `GPU_COMP_U8` vertex attributes. */
	VEC3_101010I2 = ROSE_I10_10_10_2 ,
};

static inline const char *to_glsl ( const Type type ) {
	switch ( type ) {
		case Type::FLOAT: return "float";
		case Type::VEC2: return "vec2";
		case Type::VEC3: return "vec3";
		case Type::VEC4: return "vec4";

		case Type::UINT: return "uint";
		case Type::UVEC2: return "uvec2";
		case Type::UVEC3: return "uvec3";
		case Type::UVEC4: return "uvec4";

		case Type::INT: return "int";
		case Type::IVEC2: return "ivec2";
		case Type::IVEC3: return "ivec3";
		case Type::IVEC4: return "ivec4";

		case Type::VEC3_101010I2: return "vec3_1010102_Inorm";
	}
	return "void";
}

#define GPU_BUILTIN_BITS_NONE			0x00000000
#define GPU_BUILTIN_BITS_BARYCENTRIC_COORD	0x00000001 // Allow getting barycentric coordinates inside the fragment shader.
#define GPU_BUILTIN_BITS_FRAG_COORD		0x00000002
#define GPU_BUILTIN_BITS_GLOBAL_INVOCATION_ID	0x00000004
#define GPU_BUILTIN_BITS_INSTANCE_ID		0x00000008
#define GPU_BUILTIN_BITS_LAYER			0x00000010 // Allow setting the target layer when the output is a layered frame-buffer.
#define GPU_BUILTIN_BITS_LOCAL_INVOCATION_ID	0x00000020
#define GPU_BUILTIN_BITS_LOCAL_INVOCATION_INDEX	0x00000040
#define GPU_BUILTIN_BITS_NUM_WORK_GROUP		0x00000080
#define GPU_BUILTIN_BITS_POINT_COORD		0x00000100
#define GPU_BUILTIN_BITS_POINT_SIZE		0x00000200
#define GPU_BUILTIN_BITS_PRIMITIVE_ID		0x00000400
#define GPU_BUILTIN_BITS_VERTEX_ID		0x00000800
#define GPU_BUILTIN_BITS_WORK_GROUP_ID		0x00001000
#define GPU_BUILTIN_BITS_WORK_GROUP_SIZE	0x00002000

#define GPU_DEPTH_WRITE_ANY			0x00000000
#define GPU_DEPTH_WRITE_GREATER			0x00000001
#define GPU_DEPTH_WRITE_LESS			0x00000002
#define GPU_DEPTH_WRITE_UNCHANGED		0x00000003

enum class ImageType {
	/** Color samplers/image. */
	FLOAT_BUFFER ,
	FLOAT_1D ,
	FLOAT_1D_ARRAY ,
	FLOAT_2D ,
	FLOAT_2D_ARRAY ,
	FLOAT_3D ,
	FLOAT_CUBE ,
	FLOAT_CUBE_ARRAY ,
	INT_BUFFER ,
	INT_1D ,
	INT_1D_ARRAY ,
	INT_2D ,
	INT_2D_ARRAY ,
	INT_3D ,
	INT_CUBE ,
	INT_CUBE_ARRAY ,
	UINT_BUFFER ,
	UINT_1D ,
	UINT_1D_ARRAY ,
	UINT_2D ,
	UINT_2D_ARRAY ,
	UINT_3D ,
	UINT_CUBE ,
	UINT_CUBE_ARRAY ,
	/** Depth samplers (not supported as image). */
	SHADOW_2D ,
	SHADOW_2D_ARRAY ,
	SHADOW_CUBE ,
	SHADOW_CUBE_ARRAY ,
	DEPTH_2D ,
	DEPTH_2D_ARRAY ,
	DEPTH_CUBE ,
	DEPTH_CUBE_ARRAY ,
};

#define GPU_QUALIFIER_NO_RESTRICT		0x00000001 // Restrict flag is set by default. Unless specified otherwise.
#define GPU_QUALIFIER_READ			0x00000002
#define GPU_QUALIFIER_WRITE			0x00000004
#define GPU_QUALIFIER_READWRITE			(GPU_QUALIFIER_READ|GPU_QUALIFIER_WRITE)

#define GPU_FREQUENCY_BATCH			0x00000000
#define GPU_FREQUENCY_PASS			0x00000001

#define GPU_DUAL_BLEND_NONE			0x00000000
#define GPU_DUAL_BLEND_SRC_0			0x00000001
#define GPU_DUAL_BLEND_SRC_1			0x00000002

#define GPU_INTERPOLATION_SMOOTH		0x00000000
#define GPU_INTERPOLATION_FLAT			0x00000001
#define GPU_INTERPOLATION_NO_PERSPECTIVE	0x00000002

#define GPU_PRIMITIVE_IN_POINTS			0x00000000
#define GPU_PRIMITIVE_IN_LINES			0x00000001
#define GPU_PRIMITIVE_IN_LINES_ADJ		0x00000002
#define GPU_PRIMITIVE_IN_TRIANGLES		0x00000003
#define GPU_PRIMITIVE_IN_TRIANGLES_ADJ		0x00000004

#define GPU_PRIMITIVE_OUT_POINTS		0x00000000
#define GPU_PRIMITIVE_OUT_LINE_STRIP		0x00000001
#define GPU_PRIMITIVE_OUT_TRIANGLE_STRIP	0x00000002
#define GPU_PRIMITIVE_OUT_LINES			0x00000003
#define GPU_PRIMITIVE_OUT_TRIANGLES		0x00000004

struct StageInterfaceInfo {
	struct InOut {
		int Interp;
		Type Type;
		StringRefNull Name;
	};

	StringRefNull Name;
	StringRefNull InstanceName; // Name of the instance of the block (used to access). Empty only for geometry shader.
	Vector<InOut> InOuts; // List of all members of the interface.

	StageInterfaceInfo ( const char *name , const char *instance ) : Name ( name ) , InstanceName ( InstanceName ) { }

	StageInterfaceInfo &Smooth ( Type type , StringRefNull name ) {
		this->InOuts.Append ( { GPU_INTERPOLATION_SMOOTH , type , name } );
	}

	StageInterfaceInfo &Flat ( Type type , StringRefNull name ) {
		this->InOuts.Append ( { GPU_INTERPOLATION_FLAT , type , name } );
	}

	StageInterfaceInfo &NoPerspective ( Type type , StringRefNull name ) {
		this->InOuts.Append ( { GPU_INTERPOLATION_NO_PERSPECTIVE , type , name } );
	}
};

struct ShaderCreateInfo {
	StringRefNull Name; // Shader name for debugging.
	bool DoStaticComplilation = false; // True if the shader is static and can be pre-compiled at compile time.
	bool Finalized = false; // If true, all additionally linked create info will be merged into this one.
	bool AutoResourceLocation = false; // If true, all resources will have an automatic location assigned.
	bool EarlyFragmentTest = false; // If true, force depth and stencil tests to always happen before fragment shader invocation.
	bool LegacyResourceLocation = false; // If true, force the use of the GL shader introspection for resource location.
	
	int DepthWrite = GPU_DEPTH_WRITE_ANY; // Allow optimization when fragment shader writes to `gl_FragDepth`.

	// Maximum length of all the resource names including each null terminator.
	// Only for names used by #gpu::ShaderInterface.
	size_t InterfaceNamesSize = 0;

	// Manually set builtins.
	int BuiltinBits = GPU_BUILTIN_BITS_NONE;

	std::string GeneratedVertexSource = "";
	std::string GeneratedFragmentSource = "";
	std::string GeneratedComputeSource = "";
	std::string GeneratedGeometrySource = "";
	std::string GeneratedTypedefSource = "";

	Vector<const char * , 0> Dependencies;

	struct VertIn {
		int Index;
		Type Type;
		StringRefNull Name;

		bool operator == ( const VertIn &other ) const {
			return this->Index == other.Index && this->Type == other.Type && this->Name == other.Name;
		}
	};

	Vector<VertIn> VertexInputs;

	struct GeometryStageLayout {
		int PrimIn;
		int Invocations;
		int PrimOut;

		int MaxVertices = -1;

		bool operator == ( const GeometryStageLayout &other ) const {
			return this->PrimIn == other.PrimIn && this->Invocations == other.Invocations &&
				this->PrimOut == other.PrimOut && this->MaxVertices == other.MaxVertices;
		}
	};

	GeometryStageLayout GeometryLayout;

	struct ComputeStageLayout {
		int LocalSizeX = -1;
		int LocalSizeY = -1;
		int LocalSizeZ = -1;

		bool operator == ( const ComputeStageLayout &other )const {
			return this->LocalSizeX == other.LocalSizeX && this->LocalSizeY == other.LocalSizeY && this->LocalSizeZ == other.LocalSizeZ;
		}
	};

	ComputeStageLayout ComputeLayout;

	struct FragOut {
		int Index;
		Type Type;
		int Blend;
		StringRefNull Name;

		bool operator == ( const FragOut &other ) const {
			return this->Index == other.Index && this->Type == other.Type &&
				this->Blend == other.Blend && this->Name == other.Name;
		}
	};

	Vector<FragOut > FragmentOutputs;

	struct Sampler {
		ImageType Type;
		// GPU_SAMPLER_* (in gpu_texture.h)
		int Sampler;
		StringRefNull Name;
	};

	struct Image {
		int Format;
		ImageType Type;
		int Qualifiers;
		StringRefNull Name;
	};

	struct UniformBuf {
		StringRefNull TypeName;
		StringRefNull Name;
	};

	struct StorageBuf {
		int Qualifiers;
		StringRefNull TypeName;
		StringRefNull Name;
	};

	struct Resource {
		enum class BindType {
			UNIFORM_BUFFER = 0,
			STORAGE_BUFFER,
			SAMPLER,
			IMAGE
		};

		BindType BindMethod;
		int Slot;

		union {
			Sampler Sampler;
			Image Image;
			UniformBuf UniformBuf;
			StorageBuf StorageBuf;
		};

		Resource ( BindType type , int slot ) : BindMethod ( type ) , Slot ( slot ) { }

		bool operator == ( const Resource &other ) const {
			if ( this->BindMethod == other.BindMethod && this->Slot == other.Slot ) {
				switch ( this->BindMethod ) {
					case BindType::UNIFORM_BUFFER: {
						return this->UniformBuf.TypeName == other.UniformBuf.TypeName &&
							this->UniformBuf.Name == other.UniformBuf.Name;
					}break;
					case BindType::STORAGE_BUFFER: {
						return this->StorageBuf.Qualifiers == other.StorageBuf.Qualifiers &&
							this->StorageBuf.TypeName == other.StorageBuf.TypeName &&
							this->StorageBuf.Name == other.StorageBuf.Name;
					}break;
					case BindType::SAMPLER: {
						return this->Sampler.Type == other.Sampler.Type &&
							this->Sampler.Sampler == other.Sampler.Sampler &&
							this->Sampler.Name == other.Sampler.Name;
					}break;
					case BindType::IMAGE: {
						return this->Image.Format == other.Image.Format &&
							this->Image.Type == other.Image.Type &&
							this->Image.Qualifiers == other.Image.Qualifiers &&
							this->Image.Name == other.Image.Name;
					}break;
				}
			}
			return false;
		}
	};

	/**
	* Resources are grouped by frequency of change.
	* Pass resources are meant to be valid for the whole pass.
	* Batch resources can be changed in a more granular manner (per object/material).
	* Mis-usage will only produce suboptimal performance.
	*/
	Vector<Resource> PassResources;
	Vector<Resource> BatchResources;

	Vector<StageInterfaceInfo *> VertexOutInterfaces;
	Vector<StageInterfaceInfo *> GeometryOutInterfaces;

	struct PushConst {
		Type Type;
		StringRefNull Name;
		int ArraySize;

		bool operator == ( const PushConst &other ) const {
			return this->Type == other.Type && this->ArraySize == other.ArraySize && this->Name == other.Name;
		}
	};

	Vector<PushConst> PushConstants;

	// Sources for resources type definitions.
	Vector<StringRefNull> TypedefSources;

	StringRefNull VertexSource;
	StringRefNull GeometrySource;
	StringRefNull FragmentSource;
	StringRefNull ComputeSource;

	Vector<std::array<StringRefNull , 2>> Defines;

	/**
	* Name of other infos to recursively merge with this one.
	* No data slot must overlap otherwise we throw an error.
	*/
	Vector<StringRefNull> AdditionalInfos;
public:
	ShaderCreateInfo ( const char *name ) : Name ( name ) { }

	ShaderCreateInfo &VertexIn ( int slot , Type type , StringRefNull name ) {
		this->VertexInputs.Append ( { slot , type , name } );
		this->InterfaceNamesSize += name.Size ( ) + 1;
		return *( ShaderCreateInfo * ) this;
	}

	ShaderCreateInfo &VertexOut ( StageInterfaceInfo &interface ) {
		this->VertexOutInterfaces.Append ( &interface );
		return *( ShaderCreateInfo * ) this;
	}

	/**
	* IMPORTANT: invocations count is only used if GL_ARB_gpu_shader5 is supported. On
	* implementations that do not supports it, the max_vertices will be multiplied by invocations.
	* Your shader needs to account for this fact. Use `#ifdef GPU_ARB_gpu_shader5` and make a code
	* path that does not rely on #gl_InvocationID.
	*/
	ShaderCreateInfo &geometry_layout ( int prim_in ,
					    int prim_out ,
					    int max_vertices ,
					    int invocations = -1 ) {
		this->GeometryLayout.PrimIn = prim_in;
		this->GeometryLayout.PrimOut = prim_out;
		this->GeometryLayout.MaxVertices = max_vertices;
		this->GeometryLayout.Invocations = invocations;
		return *( ShaderCreateInfo * ) this;
	}

	ShaderCreateInfo &LocalGroupSize ( int local_size_x = -1 , int local_size_y = -1 , int local_size_z = -1 ) {
		this->ComputeLayout.LocalSizeX = local_size_x;
		this->ComputeLayout.LocalSizeY = local_size_y;
		this->ComputeLayout.LocalSizeZ = local_size_z;
		return *( ShaderCreateInfo * ) this;
	}

	ShaderCreateInfo &SetEarlyFragmentTest ( bool enable ) {
		this->EarlyFragmentTest = enable;
		return *( ShaderCreateInfo * ) this;
	}

	/**
	* Only needed if geometry shader is enabled.
	* IMPORTANT: Input and output instance name will have respectively "_in" and "_out" suffix
	* appended in the geometry shader IF AND ONLY IF the vertex_out interface instance name matches
	* the geometry_out interface instance name.
	*/
	ShaderCreateInfo &GeometryOut ( StageInterfaceInfo &interface ) {
		this->GeometryOutInterfaces.Append ( &interface );
		return *( ShaderCreateInfo * ) this;
	}

	ShaderCreateInfo &fragment_out ( int slot , Type type , StringRefNull name , int blend = GPU_DUAL_BLEND_NONE ) {
		this->FragmentOutputs.Append ( { slot, type, blend, name } );
		return *( ShaderCreateInfo * ) this;
	}

	// Resources bindings points

	ShaderCreateInfo &UniformBuf ( int slot , StringRefNull type_name , StringRefNull name , int freq = GPU_FREQUENCY_PASS ) {
		Resource res ( Resource::BindType::UNIFORM_BUFFER , slot );
		res.UniformBuf.Name = name;
		res.UniformBuf.TypeName = type_name;
		( ( freq == GPU_FREQUENCY_PASS ) ? this->PassResources : this->BatchResources ).Append ( res );
		this->InterfaceNamesSize += name.Size ( ) + 1;
		return *( ShaderCreateInfo * ) this;
	}

	ShaderCreateInfo &StorageBuf ( int slot , int qualifiers , StringRefNull type_name , StringRefNull name , int freq = GPU_FREQUENCY_PASS ) {
		Resource res ( Resource::BindType::STORAGE_BUFFER , slot );
		res.StorageBuf.Name = name;
		res.StorageBuf.TypeName = type_name;
		res.StorageBuf.Qualifiers = qualifiers;
		( ( freq == GPU_FREQUENCY_PASS ) ? this->PassResources : this->BatchResources ).Append ( res );
		this->InterfaceNamesSize += name.Size ( ) + 1;
		return *( ShaderCreateInfo * ) this;
	}

	ShaderCreateInfo &Image ( int slot , int format , int qualifiers , ImageType type , StringRefNull name , int freq = GPU_FREQUENCY_PASS ) {
		Resource res ( Resource::BindType::IMAGE , slot );
		res.Image.Name = name;
		res.Image.Qualifiers = qualifiers;
		res.Image.Format = format;
		res.Image.Type = type;
		( ( freq == GPU_FREQUENCY_PASS ) ? this->PassResources : this->BatchResources ).Append ( res );
		this->InterfaceNamesSize += name.Size ( ) + 1;
		return *( ShaderCreateInfo * ) this;
	}

	ShaderCreateInfo &Sampler ( int slot ,  ImageType type , StringRefNull name , int freq = GPU_FREQUENCY_PASS ) {
		Resource res ( Resource::BindType::SAMPLER , slot );
		res.Sampler.Name = name;
		res.Sampler.Type = type;
		res.Sampler.Sampler = GPU_SAMPLER_DEFAULT;
		( ( freq == GPU_FREQUENCY_PASS ) ? this->PassResources : this->BatchResources ).Append ( res );
		this->InterfaceNamesSize += name.Size ( ) + 1;
		return *( ShaderCreateInfo * ) this;
	}

	// Shader Source

	ShaderCreateInfo &SetVertexSource ( StringRefNull filename ) {
		this->VertexSource = filename;
		return *( ShaderCreateInfo * ) this;
	}

	ShaderCreateInfo &SetGeometrySource ( StringRefNull filename ) {
		this->GeometrySource = filename;
		return *( ShaderCreateInfo * ) this;
	}

	ShaderCreateInfo &SetFragmentSource ( StringRefNull filename ) {
		this->FragmentSource = filename;
		return *( ShaderCreateInfo * ) this;
	}

	ShaderCreateInfo &SetComputeSource ( StringRefNull filename ) {
		this->ComputeSource = filename;
		return *( ShaderCreateInfo * ) this;
	}

	// Push constants

	ShaderCreateInfo &PushConstant ( Type type , StringRefNull name , int array_size = 0 ) {
		if ( name.Find ( "[" ) != StringRefNull::npos ) {
			fprintf ( stderr , "Array syntax is forbidden for push constants. Use the array_size parameter instead.\n" );
		}
		this->PushConstants.Append ( { type,name,array_size } );
		this->InterfaceNamesSize += name.Size ( ) + 1;
		return *( ShaderCreateInfo * ) this;
	}

	// Defines

	ShaderCreateInfo &Define ( StringRefNull name , StringRefNull value = "" ) {
		this->Defines.Append ( { name, value } );
		return *( ShaderCreateInfo * ) this;
	}

	ShaderCreateInfo &SetDoStaticCompilation ( bool value ) {
		this->DoStaticComplilation = value;
		return *( ShaderCreateInfo * ) this;
	}

	ShaderCreateInfo &Builtins ( int builtin ) {
		this->BuiltinBits |= builtin;
		return *( ShaderCreateInfo * ) this;
	}

	ShaderCreateInfo &SetDepthWrite ( int value ) {
		this->DepthWrite = value;
		return *( ShaderCreateInfo * ) this;
	}

	ShaderCreateInfo &SetAutoResourceLocation ( bool value ) {
		this->AutoResourceLocation = value;
		return *( ShaderCreateInfo * ) this;
	}

	ShaderCreateInfo &SetLegacyResourceLocation ( bool value ) {
		this->LegacyResourceLocation = value;
		return *( ShaderCreateInfo * ) this;
	}

	// Additional Create Info
	// Used to share parts of the infos that are common to many shaders.

	ShaderCreateInfo &AdditionalInfo ( StringRefNull info_name ) {
		this->AdditionalInfos.Append ( info_name );
		return *( ShaderCreateInfo * ) this;
	}

	template<typename... Args> ShaderCreateInfo &AdditionalInfo ( StringRefNull info_name , Args... args ) {
		AdditionalInfo ( info_name );
		AdditionalInfo ( args... );
		return *( ShaderCreateInfo * ) this;
	}

	// Typedef Sources
	// Some resource declarations might need some special structure defined.
	// Adding a file using typedef_source will include it before the resource 
	// and interface definitions.

	ShaderCreateInfo &TypedefSource ( StringRefNull filename ) {
		this->TypedefSources.Append ( filename );
		return *( ShaderCreateInfo * ) this;
	}

	// Recursive evaluation.
	// Flatten all dependency so that this descriptor contains all the data from the additional 
	// descriptors. This avoids tedious traversal in shader source creation.

	void Finalize ( );

	std::string CheckError ( ) const;

	// Error detection that some backend compilers do not complain about.
	void ValidateMerge ( const ShaderCreateInfo &other );
	void ValidateVertexAttributes ( const ShaderCreateInfo *other = nullptr );

	// Operators

	bool operator==( const ShaderCreateInfo &b ) {
		return this->BuiltinBits == b.BuiltinBits &&
			this->GeneratedVertexSource == b.GeneratedVertexSource &&
			this->GeneratedFragmentSource == b.GeneratedFragmentSource &&
			this->GeneratedGeometrySource == b.GeneratedGeometrySource &&
			this->GeneratedComputeSource == b.GeneratedComputeSource &&
			this->GeneratedTypedefSource == b.GeneratedTypedefSource &&
			this->VertexInputs == b.VertexInputs &&
			this->GeometryLayout == b.GeometryLayout &&
			this->ComputeLayout == b.ComputeLayout &&
			this->FragmentOutputs == b.FragmentOutputs &&
			this->PassResources == b.PassResources &&
			this->BatchResources == b.BatchResources &&
			this->VertexOutInterfaces == b.VertexOutInterfaces &&
			this->GeometryOutInterfaces == b.GeometryOutInterfaces &&
			this->PushConstants == b.PushConstants &&
			this->TypedefSources == b.TypedefSources &&
			this->VertexSource == b.VertexSource &&
			this->FragmentSource == b.FragmentSource &&
			this->ComputeSource == b.ComputeSource &&
			this->GeometrySource == b.GeometrySource &&
			this->AdditionalInfos == b.AdditionalInfos &&
			this->Defines == b.Defines;
	}

	bool HasResourceType ( Resource::BindType bind_type ) const {
		for ( size_t i = 0; i < this->BatchResources.Size ( ); i++ ) {
			if ( this->BatchResources [ i ].BindMethod == bind_type ) {
				return true;
			}
		}
		for ( size_t i = 0; i < this->PassResources.Size ( ); i++ ) {
			if ( this->PassResources [ i ].BindMethod == bind_type ) {
				return true;
			}
		}
		return false;
	}

	bool has_resource_image ( ) const {
		return HasResourceType ( Resource::BindType::IMAGE );
	}

	bool has_resource_storage ( ) const {
		return HasResourceType ( Resource::BindType::STORAGE_BUFFER );
	}

};

}
}
