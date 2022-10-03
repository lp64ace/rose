#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

	#define KER_FILES_TEXT				0x00000000
	#define KER_FILES_INI				0x00000001
	#define KER_FILES_SHADERS			0x00000002
	#define KER_FILES_TEXTURES			0x00000003
	#define KER_FILES_MODELS			0x00000004

	// Returns the number of common paths for the specified file type.
	int KER_get_num_common_paths_to_files ( int file );

	// Returns the specified common path for the file type.
	const char *KER_get_common_path_to_files ( int file , int path_index );

	// Check if file exists.
	bool KER_file_exists ( const char *path , const char *file );
	bool KER_wfile_exists ( const wchar_t *path , const wchar_t *file );

	// Import a file.
	unsigned char *KER_import_file_c ( const char *path , const char *file );
	unsigned char *KER_import_wfile_c ( const wchar_t *path , const wchar_t *file );

#ifdef __cplusplus
}
#endif
