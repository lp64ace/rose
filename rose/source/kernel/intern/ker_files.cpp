#include "kernel/ker_files.h"

#include "lib/lib_utildefines.h"
#include "lib/lib_error.h"
#include "lib/lib_string.h"

#include <io.h>

const char *common_text_file_paths [ ] = { "." , "text" , "../../" , "../../text" };
const char *common_ini_file_paths [ ] = { "." , "ini" , "../../", "../../ini" };
const char *common_shaders_file_paths [ ] = { "." , "shaders" , "../../", "../../shaders" };
const char *common_textures_file_paths [ ] = { "." , "textures" , "../../", "../../textures" };
const char *common_models_file_paths [ ] = { "." , "assets" , "../../", "../../assets" };

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
	return _waccess ( ker_get_fullpath ( path , file ).WCStr ( ) , 0 ) != -1;
}

bool KER_wfile_exists ( const wchar_t *path , const wchar_t *file ) {
	return _waccess ( ker_get_fullpath ( path , file ).WCStr ( ) , 0 ) != -1;
}

unsigned char *KER_import_file ( const String& file ) {
	FILE *in; _wfopen_s ( &in , file.WCStr ( ) , L"r" );
	if ( in ) {
		size_t alloc = 16 , length = 0;
		char *data = ( char * ) MEM_callocN ( alloc , "file" );
		while ( data && fscanf_s ( in , "%c" , &data [ length++ ] ) != EOF ) {
			while ( alloc <= length + 8 ) {
				char *buf = ( char * ) MEM_reallocN ( data , ( alloc += 64 ) );
				if ( buf ) {
					data = buf;
					memset ( data + length , 0 , alloc - length );
				} else {
					MEM_SAFE_FREE ( data );
				}
			}
		}
		fclose ( in );
		return ( unsigned char * ) data;
	}
	return nullptr;
}

unsigned char *KER_import_file_c ( const char *path , const char *file ) {
	return KER_import_file ( ker_get_fullpath ( path , file ) );
}

unsigned char *KER_import_wfile_c ( const wchar_t *path , const wchar_t *file ) {
	return KER_import_file ( ker_get_fullpath ( path , file ) );
}
