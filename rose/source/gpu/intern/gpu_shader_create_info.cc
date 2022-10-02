#include "gpu_shader_create_info.h"
#include "gpu_shader_create_info_private.h"

#include "gpu/gpu_info.h"

#include <map>
#include <set>

namespace rose {
namespace gpu {

using CreateInfoDictionnary = std::map<StringRef , ShaderCreateInfo *>;
using InterfaceDictionnary = std::map<StringRef , StageInterfaceInfo *>;

static CreateInfoDictionnary *g_create_infos = nullptr;
static InterfaceDictionnary *g_interfaces = nullptr;

void ShaderCreateInfo::Finalize ( ) {
	if ( this->Finalized ) {
		return;
	}

	this->Finalized = true;

	std::set<StringRefNull> DepsMerged;

	ValidateVertexAttributes ( );

	for ( size_t i = 0; i < this->AdditionalInfos.Size ( ); i++ ) {
		const ShaderCreateInfo &info = *( const ShaderCreateInfo * ) gpu_shader_create_info_get ( this->AdditionalInfos [ i ].CStr ( ) );

		const_cast< ShaderCreateInfo & >( info ).Finalize ( );

		InterfaceNamesSize += info.InterfaceNamesSize;

		this->VertexInputs.Extend ( info.VertexInputs );
		this->FragmentOutputs.Extend ( info.FragmentOutputs );
		this->VertexOutInterfaces.Extend ( info.VertexOutInterfaces );
		this->GeometryOutInterfaces.Extend ( info.GeometryOutInterfaces );

		ValidateVertexAttributes ( &info );

		this->PushConstants.Extend ( info.PushConstants );
		this->Defines.Extend ( info.Defines );

		this->BatchResources.Extend ( info.BatchResources );
		this->PassResources.Extend ( info.PassResources );
		this->TypedefSources.ExtendNoNDuplicates ( info.TypedefSources );

		if ( info.EarlyFragmentTest ) {
			this->EarlyFragmentTest = true;
		}

		if ( info.DepthWrite != GPU_DEPTH_WRITE_ANY ) {
			this->DepthWrite = info.DepthWrite;
		}

		ValidateMerge ( info );

		auto assert_no_overlap = [ & ] ( const bool test , const StringRefNull error ) {
			if ( !test ) {
				fprintf ( stderr , "%s: Validation failed while merging %s : %s\n" , Name.CStr ( ) , info.Name.CStr ( ) , error.CStr ( ) );
			}
		};

		if ( !DepsMerged.insert ( info.Name ).second ) {
			assert_no_overlap ( false , "additional info already merged via another info" );
		}

		if ( info.ComputeLayout.LocalSizeX != -1 ) {
			assert_no_overlap ( ComputeLayout.LocalSizeX == -1 , "Compute layout already defined" );
			this->ComputeLayout = info.ComputeLayout;
		}

		if ( !info.VertexSource.IsEmpty ( ) ) {
			assert_no_overlap ( VertexSource.IsEmpty ( ) , "Vertex source already existing" );
			VertexSource = info.VertexSource;
		}
		if ( !info.GeometrySource.IsEmpty ( ) ) {
			assert_no_overlap ( GeometrySource.IsEmpty ( ) , "Geometry source already existing" );
			GeometrySource = info.GeometrySource;
			GeometryLayout = info.GeometryLayout;
		}
		if ( !info.FragmentSource.IsEmpty ( ) ) {
			assert_no_overlap ( FragmentSource.IsEmpty ( ) , "Fragment source already existing" );
			FragmentSource = info.FragmentSource;
		}
		if ( !info.ComputeSource.IsEmpty ( ) ) {
			assert_no_overlap ( ComputeSource.IsEmpty ( ) , "Compute source already existing" );
			ComputeSource = info.ComputeSource;
		}

		if ( this->AutoResourceLocation ) {
			int images = 0 , samplers = 0 , ubos = 0 , ssbos = 0;

			auto set_resource_slot = [ & ] ( Resource &res ) {
				switch ( res.BindMethod ) {
					case Resource::BindType::UNIFORM_BUFFER: {
						res.Slot = ubos++;
					}break;
					case Resource::BindType::STORAGE_BUFFER: {
						res.Slot = ssbos++;
					}break;
					case Resource::BindType::SAMPLER: {
						res.Slot = samplers++;
					}break;
					case Resource::BindType::IMAGE: {
						res.Slot = images++;
					}break;
				}
			};

			for ( size_t i = 0; i < this->BatchResources.Size ( ); i++ ) {
				set_resource_slot ( this->BatchResources [ i ] );
			}
			for ( size_t i = 0; i < this->PassResources.Size ( ); i++ ) {
				set_resource_slot ( this->PassResources [ i ] );
			}
		}
	}
}

std::string ShaderCreateInfo::CheckError ( ) const {
	std::string error;

	/* At least a vertex shader and a fragment shader are required, or only a compute shader. */
	if ( this->ComputeSource.IsEmpty ( ) ) {
		if ( this->VertexSource.IsEmpty ( ) ) {
			error += "Missing vertex shader in " + this->Name + ".\n";
		}
		if ( this->FragmentSource.IsEmpty ( ) ) {
			error += "Missing fragment shader in " + this->Name + ".\n";
		}
	} else {
		if ( !this->VertexSource.IsEmpty ( ) ) {
			error += "Compute shader has vertex_source_ shader attached in " + this->Name + ".\n";
		}
		if ( !this->GeometrySource.IsEmpty ( ) ) {
			error += "Compute shader has geometry_source_ shader attached in " + this->Name + ".\n";
		}
		if ( !this->FragmentSource.IsEmpty ( ) ) {
			error += "Compute shader has fragment_source_ shader attached in " + this->Name + ".\n";
		}
	}

	return error;
}

void ShaderCreateInfo::ValidateMerge ( const ShaderCreateInfo &other_info ) {
	if ( !this->AutoResourceLocation ) {
		/* Check same bind-points usage in OGL. */
		std::set<int> images , samplers , ubos , ssbos;

		auto register_resource = [ & ] ( const Resource &res ) -> bool {
			switch ( res.BindMethod ) {
				case Resource::BindType::UNIFORM_BUFFER: return images.insert ( res.Slot ).second;
				case Resource::BindType::STORAGE_BUFFER: return samplers.insert ( res.Slot ).second;
				case Resource::BindType::SAMPLER: return ubos.insert ( res.Slot ).second;
				case Resource::BindType::IMAGE: return ssbos.insert ( res.Slot ).second;
				default: return false;
			}
		};

		auto print_error_msg = [ & ] ( const Resource &res ) {
			fprintf ( stderr , "%s: Validation failed : Overlapping " , this->Name.CStr ( ) );

			switch ( res.BindMethod ) {
				case Resource::BindType::UNIFORM_BUFFER: {
					fprintf ( stderr , "Uniform Buffer %s" , res.UniformBuf.Name.CStr ( ) );
				}break;
				case Resource::BindType::STORAGE_BUFFER: {
					fprintf ( stderr , "Storage Buffer %s" , res.StorageBuf.Name.CStr ( ) );
				}break;
				case Resource::BindType::SAMPLER: {
					fprintf ( stderr , "Sampler %s" , res.Sampler.Name.CStr ( ) );
				}break;
				case Resource::BindType::IMAGE: {
					fprintf ( stderr , "Image %s" , res.Image.Name.CStr ( ) );
				}break;
				default: {
					fprintf ( stderr , "Unknown Type" );
				}break;
			}
			fprintf ( stderr , "(%d) while merging %s\n" , res.Slot , other_info.Name.CStr ( ) );
		};

		for ( size_t i = 0; i < this->BatchResources.Size ( ); i++ ) {
			if ( register_resource ( this->BatchResources [ i ] ) == false ) {
				print_error_msg ( this->BatchResources [ i ] );
			}
		}

		for ( size_t i = 0; i < this->PassResources.Size ( ); i++ ) {
			if ( register_resource ( this->PassResources [ i ] ) == false ) {
				print_error_msg ( this->PassResources [ i ] );
			}
		}
	}
}

void ShaderCreateInfo::ValidateVertexAttributes ( const ShaderCreateInfo *other_info ) {
	uint32_t attr_bits = 0;
	for ( auto &attr : this->VertexInputs ) {
		if ( attr.Index >= 16 || attr.Index < 0 ) {
			std::cout << Name << ": \"" << attr.Name << "\" : Type::MAT3 unsupported as vertex attribute." << std::endl;
			assert ( 0 );
		}
		if ( attr.Index >= 16 || attr.Index < 0 ) {
			std::cout << Name << ": Invalid index for attribute \"" << attr.Name << "\"" << std::endl;
			assert ( 0 );
		}
		uint32_t attr_new = 0;
		if ( attr.Type == Type::MAT4 ) {
			for ( int i = 0; i < 4; i++ ) {
				attr_new |= 1 << ( attr.Index + i );
			}
		} else {
			attr_new |= 1 << attr.Index;
		}

		if ( ( attr_bits & attr_new ) != 0 ) {
			std::cout << Name << ": Attribute \"" << attr.Name
				<< "\" overlap one or more index from another attribute."
				" Note that mat4 takes up 4 indices.";
			if ( other_info ) {
				std::cout << " While merging " << other_info->Name << std::endl;
			}
			std::cout << std::endl;
			assert ( 0 );
		}
		attr_bits |= attr_new;
	}
}

}
}

using namespace rose;
using namespace rose::gpu;

void gpu_shader_create_info_init ( ) {
	g_create_infos = new CreateInfoDictionnary ( );
	g_interfaces = new InterfaceDictionnary ( );

#define GPU_SHADER_INTERFACE_INFO(_interface, _inst_name) \
	auto *ptr_##_interface = new StageInterfaceInfo(#_interface, _inst_name); \
	auto &_interface = *ptr_##_interface; \
	(*g_interfaces)[#_interface] = ptr_##_interface; \
	_interface

#define GPU_SHADER_CREATE_INFO(_info) \
	auto *ptr_##_info = new ShaderCreateInfo(#_info); \
	auto &_info = *ptr_##_info; \
	(*g_create_infos)[#_info] = ptr_##_info; \
	_info

#include "gpu/infos/gpu_shader_create_info_list.h"

}

void gpu_shader_create_info_exit ( ) {
	for ( CreateInfoDictionnary::iterator itr = g_create_infos->begin ( ); itr != g_create_infos->end ( ); itr++ ) {
		delete itr->second;
	}
	delete g_create_infos;
	for ( InterfaceDictionnary::iterator itr = g_interfaces->begin ( ); itr != g_interfaces->end ( ); itr++ ) {
		delete itr->second;
	}
	delete g_interfaces;
}

bool gpu_shader_create_info_compile_all ( ) {
	int success = 0;
	int skipped = 0;
	int total = 0;
	for ( CreateInfoDictionnary::iterator itr = g_create_infos->begin ( ); itr != g_create_infos->end ( ); itr++ ) {
		ShaderCreateInfo *info = itr->second;
		info->Finalize ( );
		if ( info->DoStaticComplilation ) {
			if ( ( GPU_get_info_i ( GPU_INFO_COMPUTE_SHADER_SUPPORT ) == false && info->ComputeSource != nullptr ) ||
			     ( GPU_get_info_i ( GPU_INFO_SHADER_IMAGE_LOAD_STORE_SUPPORT ) == false && info->has_resource_image ( ) ) ||
			     ( GPU_get_info_i ( GPU_INFO_SHADER_STORAGE_BUFFER_OBJECTS_SUPPORT ) == false && info->has_resource_storage ( ) ) ) {
				skipped++;
				continue;
			}
			total++;
			GPU_Shader *shader = GPU_shader_create_from_info ( reinterpret_cast< const GPU_ShaderCreateInfo * >( info ) );
			if ( shader == nullptr ) {
				printf ( "Compilation %s Failed\n" , info->Name.CStr ( ) );
			} else {
				success++;
			}
			GPU_shader_free ( shader );
		}
	}
	printf ( "Shader Test compilation result: %d / %d passed" , success , total );
	if ( skipped > 0 ) {
		printf ( " (skipped %d for compatibility reasons)" , skipped );
	}
	printf ( "\n" );
	return success == total;
}

// Runtime create infos are not registered in the dictionary and cannot be searched.
const GPU_ShaderCreateInfo *gpu_shader_create_info_get ( const char *info_name ) {
	CreateInfoDictionnary::iterator itr = g_create_infos->find ( info_name );
	if ( itr == g_create_infos->end ( ) ) {
		printf ( "Error: Cannot find shader create info named \"%s\"\n" , info_name );
		return nullptr;
	}
	ShaderCreateInfo *info = itr->second;
	return reinterpret_cast< const GPU_ShaderCreateInfo * >( info );
}
