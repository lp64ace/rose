#include <algorithm>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>

#include "lib/lib_string_ref.h"

#include "gpu_shader_create_info.h"
#include "gpu_shader_dependency_private.h"

#include <map>

extern "C" {
#define SHADER_SOURCE(datatoc, filename, filepath) extern char datatoc[];
#undef SHADER_SOURCE
}

namespace rose {
namespace gpu {

using GPUSourceDictionnary = std::map<StringRef , struct GPU_Source *>;
using GPUFunctionDictionnary = std::map<StringRef , struct GPU_Function *>;

static GPUSourceDictionnary *g_sources = nullptr;
static GPUFunctionDictionnary *g_functions = nullptr;

struct GPU_Source {
	StringRefNull Fullpath;
	StringRefNull Filename;
	StringRefNull Source;
	
	Vector<GPU_Source *> Dependencies;
	bool DependenciesInit = false;

	int Builtins = GPU_BUILTIN_BITS_NONE;

	std::string ProcessedSource;

	GPU_Source ( const char *path , const char *file , const char *datatoc , GPUFunctionDictionnary *g_functions ) {
		this->Fullpath = path;
		this->Filename = file;
		this->Source = datatoc;

		if ( Source.Find ( "gl_FragCoord" , 0 ) != StringRef::npos ) {
			Builtins |= GPU_BUILTIN_BITS_FRAG_COORD;
		}
		if ( Source.Find ( "gl_GlobalInvocationID" , 0 ) != StringRef::npos ) {
			Builtins |= GPU_BUILTIN_BITS_GLOBAL_INVOCATION_ID;
		}
		if ( Source.Find ( "gl_InstanceID" , 0 ) != StringRef::npos ) {
			Builtins |= GPU_BUILTIN_BITS_INSTANCE_ID;
		}
		if ( Source.Find ( "gl_LocalInvocationID" , 0 ) != StringRef::npos ) {
			Builtins |= GPU_BUILTIN_BITS_LOCAL_INVOCATION_ID;
		}
		if ( Source.Find ( "gl_LocalInvocationIndex" , 0 ) != StringRef::npos ) {
			Builtins |= GPU_BUILTIN_BITS_LOCAL_INVOCATION_INDEX;
		}
		if ( Source.Find ( "gl_NumWorkGroup" , 0 ) != StringRef::npos ) {
			Builtins |= GPU_BUILTIN_BITS_NUM_WORK_GROUP;
		}
		if ( Source.Find ( "gl_PointCoord" , 0 ) != StringRef::npos ) {
			Builtins |= GPU_BUILTIN_BITS_POINT_COORD;
		}
		if ( Source.Find ( "gl_PointSize" , 0 ) != StringRef::npos ) {
			Builtins |= GPU_BUILTIN_BITS_POINT_SIZE;
		}
		if ( Source.Find ( "gl_PrimitiveID" , 0 ) != StringRef::npos ) {
			Builtins |= GPU_BUILTIN_BITS_PRIMITIVE_ID;
		}
		if ( Source.Find ( "gl_VertexID" , 0 ) != StringRef::npos ) {
			Builtins |= GPU_BUILTIN_BITS_VERTEX_ID;
		}
		if ( Source.Find ( "gl_WorkGroupID" , 0 ) != StringRef::npos ) {
			Builtins |= GPU_BUILTIN_BITS_WORK_GROUP_ID;
		}
		if ( Source.Find ( "gl_WorkGroupSize" , 0 ) != StringRef::npos ) {
			Builtins |= GPU_BUILTIN_BITS_WORK_GROUP_SIZE;
		}

		if ( Filename.EndsWith ( ".h" ) || Filename.EndsWith ( ".hh" ) ) {
			EnumPreprocess ( );
			QuotePreprocess ( );
		} else {
			if ( Source.Find ( "'" ) != StringRef::npos ) {
				CharLiteralsPreprocess ( );
			}
			CheckNoQuotes ( );
		}
	}

	static bool IsInComment ( const StringRef &input , int64_t offset ) {
		return ( input.ReverseFind ( "/*" , offset ) > input.ReverseFind ( "*/" , offset ) ) ||
			( input.ReverseFind ( "//" , offset ) > input.ReverseFind ( "\n" , offset ) );
	}

	template<bool check_whole_word = true , bool reversed = false , typename T>
	static int64_t FindStr ( const StringRef &input , const T keyword , int64_t offset = 0 ) {
		while ( true ) {
			if constexpr ( reversed ) {
				offset = input.ReverseFind ( keyword , offset );
			} else {
				offset = input.ReverseFind ( keyword , offset );
			}
			if ( offset > 0 ) {
				if constexpr ( check_whole_word ) {
					/* Fix false positive if something has "enum" as suffix. */
					char previous_char = input [ offset - 1 ];
					if ( !( ELEM ( previous_char , '\n' , '\t' , ' ' , ':' , '(' , ',' ) ) ) {
						offset += ( reversed ) ? -1 : 1;
						continue;
					}
				}
				/* Fix case where the keyword is in a comment. */
				if ( IsInComment ( input , offset ) ) {
					offset += ( reversed ) ? -1 : 1;
					continue;
				}
			}
			return offset;
		}
	}

#define find_keyword FindStr<true, false>
#define rfind_keyword FindStr<true, true>
#define find_token FindStr<false, false>
#define rfind_token FindStr<false, true>

	void PrintError ( const StringRef &input , size_t offset , const StringRef message ) {
		StringRef sub = input.SubStr ( 0 , offset );
		int64_t line_number = std::count ( sub.Begin ( ) , sub.End ( ) , '\n' ) + 1;
		int64_t line_end = input.Find ( "\n" , offset );
		int64_t line_start = input.ReverseFind ( "\n" , offset ) + 1;
		int64_t char_number = offset - line_start + 1;

		/* TODO Use clog. */

		std::cout << Fullpath.CStr ( ) << ":" << line_number << ":" << char_number;

		std::cout << " error: " << message << "\n";
		std::cout << std::setw ( 5 ) << line_number << " | " << input.SubStr ( line_start , line_end - line_start ) << "\n";
		std::cout << "      | ";
		for ( int64_t i = 0; i < char_number - 1; i++ ) {
			std::cout << " ";
		}
		std::cout << "^\n";
	}

#define CHECK(test_value, str, ofs, msg)	if ((test_value) == -1) {PrintError(str, ofs, msg);continue;}

	void CheckNoQuotes ( ) {
		int64_t pos = -1;
		do {
			pos = Source.Find ( '"' , pos + 1 );
			if ( pos == StringRefBase::npos ) {
				break;
			}
			if ( !IsInComment ( Source , pos ) ) {
				PrintError ( Source , pos , "Quote characters are forbidden in GLSL files" );
			}
		} while ( true );
	}

	/**
	* Some drivers completely forbid string characters even in unused preprocessor directives.
	* This fixes the cases we cannot manually patch: Shared headers #includes. (see T97545)
	* TODO(fclem): This could be done during the datatoc step.
	*/
	void QuotePreprocess ( ) {
		if ( Source.FindFirstOf ( '"' ) == -1 ) {
			return;
		}

		ProcessedSource = Source;
		std::replace ( ProcessedSource.begin ( ) , ProcessedSource.end ( ) , '"' , ' ' );

		Source = ProcessedSource.c_str ( );
	}

	void EnumPreprocess ( ) {
		const StringRefNull input = Source;
		std::string output;
		int64_t cursor = -1;
		int64_t last_pos = 0;
		const bool is_cpp = Filename.EndsWith ( ".hh" );

		while ( true ) {
			cursor = find_keyword ( input , "enum " , cursor + 1 );
			if ( cursor == -1 ) {
				break;
			}
			/* Skip matches like `typedef enum myEnum myType;` */
			if ( cursor >= 8 && input.SubStr ( cursor - 8 , 8 ) == "typedef " ) {
				continue;
			}
			/* Output anything between 2 enums blocks. */
			output += input.SubStr ( last_pos , cursor - last_pos );

			/* Extract enum type name. */
			int64_t name_start = input.Find ( " " , cursor );

			int64_t values_start = find_token ( input , '{' , cursor );
			CHECK ( values_start , input , cursor , "Malformed enum class. Expected \'{\' after typename." );

			StringRef enum_name = input.SubStr ( name_start , values_start - name_start );
			if ( is_cpp ) {
				int64_t name_end = find_token ( enum_name , ":" );
				CHECK ( name_end , input , name_start , "Expected \':\' after C++ enum name." );

				int64_t underlying_type = find_keyword ( enum_name , "uint32_t" , name_end );
				CHECK ( underlying_type , input , name_start , "C++ enums needs uint32_t underlying type." );

				enum_name = input.SubStr ( name_start , name_end );
			}

			output += "#define " + enum_name + " uint\n";

			/* Extract enum values. */
			int64_t values_end = find_token ( input , '}' , values_start );
			CHECK ( values_end , input , cursor , "Malformed enum class. Expected \'}\' after values." );

			/* Skip opening brackets. */
			values_start += 1;

			StringRef enum_values = input.SubStr ( values_start , values_end - values_start );

			/* Really poor check. Could be done better. */
			int64_t token = find_token ( enum_values , '{' );
			int64_t not_found = ( token == -1 ) ? 0 : -1;
			CHECK ( not_found , input , values_start + token , "Unexpected \'{\' token inside enum values." );

			/* Do not capture the comma after the last value (if present). */
			int64_t last_equal = rfind_token ( enum_values , '=' , values_end );
			int64_t last_comma = rfind_token ( enum_values , ',' , values_end );
			if ( last_comma > last_equal ) {
				enum_values = input.SubStr ( values_start , last_comma );
			}

			output += "const uint " + enum_values;

			int64_t semicolon_found = ( input [ values_end + 1 ] == ';' ) ? 0 : -1;
			CHECK ( semicolon_found , input , values_end + 1 , "Expected \';\' after enum type declaration." );

			/* Skip the curly bracket but not the semicolon. */
			cursor = last_pos = values_end + 1;
		}
		/* If nothing has been changed, do not allocate processed_source. */
		if ( last_pos == 0 ) {
			return;
		}

		if ( last_pos != 0 ) {
			output += input.SubStr ( last_pos );
		}

		ProcessedSource = output;
		Source = ProcessedSource.c_str ( );
	}

	void CharLiteralsPreprocess ( ) {
		const StringRefNull input = Source;
		std::stringstream output;
		int64_t cursor = -1;
		int64_t last_pos = 0;

		while ( true ) {
			cursor = find_token ( input , '\'' , cursor + 1 );
			if ( cursor == -1 ) {
				break;
			}
			/* Output anything between 2 print statement. */
			output << input.SubStr ( last_pos , cursor - last_pos );

			/* Extract string. */
			int64_t char_start = cursor + 1;
			int64_t char_end = find_token ( input , '\'' , char_start );
			CHECK ( char_end , input , cursor , "Malformed char literal. Missing ending `'`." );

			StringRef input_char = input.SubStr ( char_start , char_end - char_start );
			if ( input_char.Size ( ) == 0 ) {
				CHECK ( -1 , input , cursor , "Malformed char literal. Empty character constant" );
			}

			uint8_t char_value = input_char [ 0 ];

			if ( input_char [ 0 ] == '\\' ) {
				if ( input_char [ 1 ] == 'n' ) {
					char_value = '\n';
				} else {
					CHECK ( -1 , input , cursor , "Unsupported escaped character" );
				}
			} else {
				if ( input_char.Size ( ) > 1 ) {
					CHECK ( -1 , input , cursor , "Malformed char literal. Multi-character character constant" );
				}
			}

			char hex [ 8 ];
			snprintf ( hex , 8 , "0x%.2Xu" , char_value );
			output << hex;

			cursor = last_pos = char_end + 1;
		}
		/* If nothing has been changed, do not allocate processed_source. */
		if ( last_pos == 0 ) {
			return;
		}

		if ( last_pos != 0 ) {
			output << input.SubStr ( last_pos );
		}
		ProcessedSource = output.str ( );
		Source = ProcessedSource.c_str ( );
	}

#undef find_keyword
#undef rfind_keyword
#undef find_token
#undef rfind_token

	/* Return 1 one error. */
	int InitDependencies ( const GPUSourceDictionnary &dict ,
				const GPUFunctionDictionnary &g_functions ) {
		if ( this->DependenciesInit ) {
			return 0;
		}
		this->DependenciesInit = true;
		int64_t pos = -1;

		while ( true ) {
			GPU_Source *dependency_source = nullptr;

			{
				pos = Source.Find ( "pragma ROSE_REQUIRE(" , pos + 1 );
				if ( pos == -1 ) {
					return 0;
				}
				int64_t start = Source.Find ( '(' , pos ) + 1;
				int64_t end = Source.Find ( ')' , pos );
				if ( end == -1 ) {
					PrintError ( Source , start , "Malformed ROSE_REQUIRE: Missing \")\" token" );
					return 1;
				}
				StringRef dependency_name = Source.SubStr ( start , end - start );
				GPUSourceDictionnary::const_iterator itr = dict.find ( dependency_name );
				if ( itr == dict.end ( ) ) {
					PrintError ( Source , start , "Dependency not found" );
					return 1;
				}
				dependency_source = itr->second;
			}

			/* Recursive. */
			int result = dependency_source->InitDependencies ( dict , g_functions );
			if ( result != 0 ) {
				return 1;
			}

			for ( size_t i = 0; i < dependency_source->Dependencies.Size ( ); i++ ) {
				Dependencies.AppendNoNDuplicates ( dependency_source->Dependencies [ i ] );
			}
			Dependencies.AppendNoNDuplicates ( dependency_source );
		}
		return 0;
	}

	void Build ( Vector<const char *> &result ) const {
		for ( size_t i = 0; i < this->Dependencies.Size ( ); i++ ) {
			result.Append ( this->Dependencies [ i ]->Source.CStr ( ) );
		}
		result.Append ( Source.CStr ( ) );
	}

	int BuiltinsGet ( ) const {
		int out_builtins = this->Builtins;
		for ( size_t i = 0; i < this->Dependencies.Size ( ); i++ ) {
			out_builtins |= this->Dependencies [ i ]->Builtins;
		}
		return out_builtins;
	}

};

}
}

using namespace rose;
using namespace rose::gpu;

void gpu_shader_dependency_init ( ) {
	g_sources = new GPUSourceDictionnary ( );
	g_functions = new GPUFunctionDictionnary ( );

#define SHADER_SOURCE(datatoc, filename, filepath) \
	(*g_sources)[filename] = new GPU_Source(filepath, filename, datatoc, g_functions);
#undef SHADER_SOURCE

	int errors = 0;
	for ( GPUSourceDictionnary::iterator itr = g_sources->begin ( ); itr != g_sources->end ( ); itr++ ) {
		errors += itr->second->InitDependencies ( *g_sources , *g_functions );
	}
	if ( errors != 0 ) {
		fprintf ( stderr , "Dependency errors detected: Aborting" );
	}
}

void gpu_shader_dependency_exit ( ) {
	for ( GPUSourceDictionnary::iterator itr = g_sources->begin ( ); itr != g_sources->end ( ); itr++ ) {
		delete itr->second;
	}
	for ( GPUFunctionDictionnary::iterator itr = g_functions->begin ( ); itr != g_functions->end ( ); itr++ ) {
		delete itr->second;
	}
	delete g_sources; g_sources = nullptr;
	delete g_functions; g_functions = nullptr;
}

int gpu_shader_dependency_get_builtin_bits ( const StringRefNull shader_source_name ) {
	if ( shader_source_name.IsEmpty ( ) ) {
		return GPU_BUILTIN_BITS_NONE;
	}
	if ( g_sources->find ( shader_source_name ) == g_sources->end ( ) ) {
		std::cout << "Error: Could not find \"" << shader_source_name << "\" in the list of registered source.\n";
		return GPU_BUILTIN_BITS_NONE;
	}
	GPU_Source *source = g_sources->find ( shader_source_name )->second;
	return source->BuiltinsGet ( );
}

std::string to_filename ( const StringRefNull path , const StringRefNull file ) {
	if ( path.EndsWith ( "\\" ) ) {
		return  path + file;
	} else if ( path.EndsWith ( "/" ) ) {
		return  path + file;
	} else if ( path.Find ( "\\" ) != StringRefBase::npos ) {
		return path + "\\" + file;
	} else if ( path.Find ( "/" ) != StringRefBase::npos ) {
		return path + "/" + file;
	}
	return path + "/" + file;
}

bool gpu_file_exists ( const StringRefNull path , const StringRefNull file ) {
	FILE *in; fopen_s ( &in , to_filename ( path , file ).c_str ( ) , "r" );
	if ( in ) {
		fclose ( in );
		return true;
	}
	return false;
}

void gpu_shader_dependency_import_source ( const StringRefNull path , const StringRefNull file ) {
	FILE *in; fopen_s ( &in , to_filename ( path , file ).c_str ( ) , "r" );
	if ( in ) {
		size_t alloc = 16 , length = 0;
		char *code = ( char * ) MEM_mallocN ( alloc , file.CStr ( ) );

		while ( fscanf_s ( in , "%c" , &code [ length++ ] ) != EOF ) {
			while ( alloc <= length + 8 ) {
				code = ( char * ) MEM_reallocN ( code , ( alloc += 64 ) );
			}
		}

		code [ --length ] = 0;
		( *g_sources ) [ file ] = new GPU_Source ( path.CStr ( ) , file.CStr ( ) , code , g_functions );
		fclose ( in );
	}
}

void gpu_shader_dependency_try_to_locate_source ( const StringRefNull shader_source_name ) {
	GPUSourceDictionnary::const_iterator itr = g_sources->find ( shader_source_name );
	if ( itr == g_sources->end ( ) ) {
		if ( gpu_file_exists ( "shaders" , shader_source_name ) ) {
			gpu_shader_dependency_import_source ( "shaders" , shader_source_name );
		} else if ( gpu_file_exists ( "../../shaders" , shader_source_name ) ) {
			gpu_shader_dependency_import_source ( "../../shaders" , shader_source_name );
		}
	}
}

Vector<const char *> gpu_shader_dependency_get_resolved_source ( const StringRefNull shader_source_name ) {
	Vector<const char *> result;
	gpu_shader_dependency_try_to_locate_source ( shader_source_name );
	GPUSourceDictionnary::const_iterator itr = g_sources->find ( shader_source_name );
	if ( itr == g_sources->end ( ) ) {
		std::cout << "Error source not found : " << shader_source_name << std::endl;
		assert ( 0 );
	}
	GPU_Source *src = itr->second;
	src->Build ( result );
	return result;
}

StringRefNull gpu_shader_dependency_get_source ( const StringRefNull shader_source_name ) {
	GPUSourceDictionnary::const_iterator itr = g_sources->find ( shader_source_name );
	gpu_shader_dependency_try_to_locate_source ( shader_source_name );
	if ( itr == g_sources->end ( ) ) {
		std::cout << "Error source not found : " << shader_source_name << std::endl;
		assert ( 0 );
	}
	GPU_Source *src = itr->second;
	return src->Source;
}

StringRefNull gpu_shader_dependency_get_filename_from_source_string ( const StringRefNull source_string ) {
	for ( GPUSourceDictionnary::iterator itr = g_sources->begin ( ); itr != g_sources->end ( ); itr++ ) {
		if ( itr->second->Source.CStr ( ) == source_string.CStr ( ) ) {
			return itr->second->Filename;
		}
	}
	return "";
}
