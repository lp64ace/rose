#include "MEM_guardedalloc.h"

#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "RT_parser.h"
#include "RT_source.h"

#include <stdio.h>

const char *header_typedef[] = {
#include "DNA_strings_all.h"
};

const char *source = NULL;
const char *binary = NULL;

ROSE_INLINE void help(int i, char **argv) {
	if (i) {
		fprintf(stderr, "Error near '%s'!\n\n", argv[i]);
	}
	else {
		fprintf(stderr, "Invalid argument list!\n\n");
	}
	fprintf(stderr, "Usage:\n\n");
	fprintf(stderr, "\t%s\n", argv[0]);
	fprintf(stderr, "\t\t--src <directory>\n");
	fprintf(stderr, "\t\t--bin <directory>\n");
}

ROSE_INLINE int arg(int argc, char **argv) {
	for(int i = 1; i < argc; i++) {
		if(STREQ(argv[i], "--src") && i + 1 < argc) {
			source = argv[++i];
			continue;
		}
		if(STREQ(argv[i], "--bin") && i + 1 < argc) {
			binary = argv[++i];
			continue;
		}
		else {
			help(i, argv);
			return 0xf1;
		}
	}
	
	if(!source) {
		help(0, argv);
		return 0xf2;
	}
	if(!binary) {
		help(0, argv);
		return 0xf3;
	}
	return 0;
}

ROSE_INLINE size_t fsize(FILE *file) {
	long size;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	return (size_t)size;
}

ROSE_INLINE char *fload(const char *path) {
	FILE *handle = fopen(path, "rb");
	if(!handle) {
		fprintf(stderr, "Failed to open file '%s' for reading.\n", path);
		return NULL;
	}
	size_t size = fsize(handle);
	
	char *buffer = (char *)MEM_callocN(fsize(handle) + 1, "file::buffer");
	if(!buffer) {
		fprintf(stderr, "Failed to allocate enough space to read file.\n");
		MEM_freeN(buffer);
		fclose(handle);
		return NULL;
	}
	if(!fread(buffer, size, 1, handle)) {
		fprintf(stderr, "Failed to read file.\n");
		MEM_freeN(buffer);
		fclose(handle);
		return NULL;
	}
	
	fclose(handle);
	return buffer;
}

ROSE_INLINE bool fmake(const char *path) {
	FILE *handle = fopen(path, "w");
	if(!handle) {
		printf("Failed to open file '%s' for writing.\n", path);
		return false;
	}

	fprintf(handle, "/* Do not edit manually, changes will be overwritten. */\n");

	fclose(handle);
	return true;
}

int main(int argc, char **argv) {
	int status = 0;
	
	if((status = arg(argc, argv))) {
		/** Handle invalid or missing arguments error status here! */
		return status;
	}
	
	char path[1024];
	
	for(size_t index = 0; index < ARRAY_SIZE(header_typedef); index++) {
		LIB_strcpy(path, ARRAY_SIZE(path), source);
		LIB_strcat(path, ARRAY_SIZE(path), "/");
		LIB_strcat(path, ARRAY_SIZE(path), header_typedef[index]);
		
		char *buffer = fload(path);
		if(!buffer) {
			return 0xe1;
		}
		
		RCCFileCache *cache = RT_fcache_new(path, buffer, LIB_strlen(buffer));
		RCCFile *file = RT_file_new(header_typedef[index], cache);
		{
			RCCParser *parser = RT_parser_new(file);
			
			if(!RT_parser_do(parser)) {
				printf("Compilation failed for %s!\n", header_typedef[index]);
				return 0xe2;
			}
		}
		RT_file_free(file);
		RT_fcache_free(cache);
		
		MEM_freeN(buffer);
	}
	
	LIB_strcpy(path, ARRAY_SIZE(path), binary);
	LIB_strcat(path, ARRAY_SIZE(path), "/");
	LIB_strcat(path, ARRAY_SIZE(path), "dna.c");
	
	if(!fmake(path)) {
		return 0xe3;
	}
	
	return status;
}

/*

--src "C:/Users/Jim/source/repos/rose/source/rose/makedna"
--bin "C:/Users/Jim/source/build/rose/source/rose/makedna/intern"

*/
