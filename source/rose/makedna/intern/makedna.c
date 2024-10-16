#include "MEM_guardedalloc.h"

#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "RT_parser.h"
#include "RT_source.h"

#include <stdio.h>
#include <stdbool.h>

const char *header_typedef[] = {
#include "DNA_strings_all.h"
};

const char *source = NULL;
const char *binary = NULL;

ROSE_INLINE void help(const char *program_name) {
	fprintf(stderr, "Invalid argument list!\n\n");
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\t%s --src <directory> --bin <directory>\n", program_name);
}

ROSE_INLINE bool init(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		if (STREQ(argv[i], "--src") && i + 1 < argc) {
			source = argv[++i];
			continue;
		}
		if (STREQ(argv[i], "--bin") && i + 1 < argc) {
			binary = argv[++i];
			continue;
		}
		return false; // Invalid argument
	}
	return source && binary; // Return true only if both arguments are provided
}

ROSE_INLINE bool build(const char *filepath) {
	FILE *file = fopen(filepath, "w");
	if (!file) {
		fprintf(stderr, "Failed to open file '%s' for writing.\n", filepath);
		return false;
	}

	// Write a warning header into the generated file
	fprintf(file, "/* Auto-generated file. Do not edit manually. */\n");

	fclose(file);
	return true;
}

int main(int argc, char **argv) {
	if(!init(argc, argv)) {
		help(argv[0]);
		return -1;
	}
	
	char path[1024];
	
	for(size_t index = 0; index < ARRAY_SIZE(header_typedef); index++) {
		LIB_strcpy(path, ARRAY_SIZE(path), source);
		LIB_strcat(path, ARRAY_SIZE(path), "/");
		LIB_strcat(path, ARRAY_SIZE(path), header_typedef[index]);
		
		RCCFileCache *cache = RT_fcache_read(path);
		RCCFile *file = RT_file_new(header_typedef[index], cache);
		RCCParser *parser = RT_parser_new(file);
		
		if(!RT_parser_do(parser)) {
			printf("Compilation failed for %s!\n", header_typedef[index]);
			return -1;
		}
		
		RT_parser_free(parser);
		RT_file_free(file);
		RT_fcache_free(cache);
	}
	
	LIB_strcpy(path, ARRAY_SIZE(path), binary);
	LIB_strcat(path, ARRAY_SIZE(path), "/");
	LIB_strcat(path, ARRAY_SIZE(path), "dna.c");
	
	if(!build(path)) {
		return -1;
	}
	
	return 0;
}

/*

--src "C:/Users/Jim/source/repos/rose/source/rose/makedna"
--bin "C:/Users/Jim/source/build/rose/source/rose/makedna/intern"

*/
