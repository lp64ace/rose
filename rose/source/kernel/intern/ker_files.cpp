#define _CRT_SECURE_NO_WARNINGS
#include "kernel/ker_files.h"

#include "lib/lib_utildefines.h"
#include "lib/lib_error.h"
#include "lib/lib_string.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <string>
#  include <fstream>
#  include <streambuf>
#endif

#include <io.h>

const char *common_text_file_paths [ ] = { "../../text" , "./text" , "./" };
const char *common_ini_file_paths [ ] = { "../../ini" , "./ini", "./" };
const char *common_shaders_file_paths [ ] = { "../../shaders" , "./shaders", "./" };
const char *common_textures_file_paths [ ] = { "../../textures" , "./textures", "./" };
const char *common_models_file_paths [ ] = { "../../assets" , "./assets", "./" };

int KER_get_num_common_paths_to_files ( int file ) {
	switch ( file ) {
		case KER_FILES_TEXT: return ARRAY_SIZE ( common_text_file_paths );
		case KER_FILES_INI: return ARRAY_SIZE ( common_ini_file_paths );
		case KER_FILES_SHADERS: return ARRAY_SIZE ( common_shaders_file_paths );
		case KER_FILES_TEXTURES: return ARRAY_SIZE ( common_textures_file_paths );
		case KER_FILES_MODELS: return ARRAY_SIZE ( common_models_file_paths );
	}
	LIB_assert_unreachable ( );
	return 0;
}

const char *KER_get_common_path_to_files ( int file , int path_index ) {
	LIB_assert ( 0 <= path_index && path_index < KER_get_num_common_paths_to_files ( file ) );
	switch ( file ) {
		case KER_FILES_TEXT: return common_text_file_paths [ path_index ];
		case KER_FILES_INI: return common_ini_file_paths [ path_index ];
		case KER_FILES_SHADERS: return common_shaders_file_paths [ path_index ];
		case KER_FILES_TEXTURES: return common_textures_file_paths [ path_index ];
		case KER_FILES_MODELS: return common_models_file_paths [ path_index ];
	}
	LIB_assert_unreachable ( );
	return nullptr;
}

String ker_get_fullpath ( const String& path , const String &file ) {
	if ( path.EndsWith ( "\\" ) || path.EndsWith ( "/" ) ) {
		return path + file;
	} else if ( path.Find ( '\\' ) != String::npos ) {
		return path + "\\" + file;
	} else if ( path.Find ( '/' ) != String::npos ) {
		return path + "/" + file;
	}
	return path + "/" + file;
}

bool KER_file_exists ( const char * path , const char *file ) {
	return _waccess ( ker_get_fullpath ( path , file ).WCStr ( ) , 0 ) == 0;
}

bool KER_wfile_exists ( const wchar_t *path , const wchar_t *file ) {
	return _waccess ( ker_get_fullpath ( path , file ).WCStr ( ) , 0 ) == 0;
}

unsigned char *KER_import_file ( const String& file ) {
#ifdef _WIN32
	HANDLE handle = CreateFileW ( file.WCStr ( ) , GENERIC_READ , FILE_SHARE_READ , NULL , OPEN_EXISTING , FILE_ATTRIBUTE_NORMAL , NULL );
	if ( handle != INVALID_HANDLE_VALUE ) {
		LARGE_INTEGER size = { 0 };
		if ( GetFileSizeEx ( handle , &size ) ) {
			unsigned char *buffer = ( unsigned char * ) MEM_mallocN ( ( size_t ) ++size.QuadPart , __FUNCTION__ );
			DWORD bytesRead = 0;
			if ( ReadFile ( handle , buffer , ( DWORD ) size.QuadPart , &bytesRead , NULL ) ) {
				buffer [ bytesRead ] = 0;
				return buffer;
			} else {
				MEM_SAFE_FREE ( buffer );
			}
		}
	}
#else
	std::wifstream t ( file.WCStr ( ) );
	if ( t.is_open ( ) ) {
		std::wstring str ( ( std::istreambuf_iterator<wchar_t> ( t ) ) , std::istreambuf_iterator<wchar_t> ( ) );
		unsigned char *buffer = ( unsigned char * ) MEM_mallocN ( str.capacity ( ) , __FUNCTION__ );
		memcpy ( buffer , str.data ( ) , str.capacity ( ) );
		return buffer;
	}
#endif
	return nullptr;
	
}

unsigned char *KER_import_file_c ( const char *path , const char *file ) {
	return KER_import_file ( ker_get_fullpath ( path , file ) );
}

unsigned char *KER_import_wfile_c ( const wchar_t *path , const wchar_t *file ) {
	return KER_import_file ( ker_get_fullpath ( path , file ) );
}
