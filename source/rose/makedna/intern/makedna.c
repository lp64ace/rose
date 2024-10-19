#include "MEM_guardedalloc.h"

#include "LIB_string.h"
#include "LIB_ghash.h"
#include "LIB_utildefines.h"

#include "RT_context.h"
#include "RT_object.h"
#include "RT_parser.h"
#include "RT_source.h"
#include "RT_token.h"

#include "genfile.h"

#include <stdio.h>
#include <stdbool.h>

const char *header_typedef[] = {
#include "DNA_strings_all.h"
};

#ifdef __BIG_ENDIAN__
/* Big Endian */
#	define MAKE_ID4(a, b, c, d) ((a) << 24 | (b) << 16 | (c) << 8 | (d))
#else
/* Little Endian */
#	define MAKE_ID4(a, b, c, d) ((d) << 24 | (c) << 16 | (b) << 8 | (a))
#endif

const char *source = NULL;
const char *binary = NULL;

GSet *written;

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

ROSE_INLINE bool build(const char *filepath, SDNA *sdna) {
	FILE *file = fopen(filepath, "w");
	if (!file) {
		fprintf(stderr, "Failed to open file '%s' for writing.\n", filepath);
		return false;
	}
	
	// Write a warning header into the generated file
	fprintf(file, "/* Auto-generated file. Do not edit manually. */\n");
	fprintf(file, "\n");

	fprintf(file, "extern const unsigned char DNAstr[];\n");
	fprintf(file, "const unsigned char DNAstr[] = {\n\t");
	
	for(size_t index = 0; index < sdna->length; index++) {
		if(index && index % 16 == 0) {
			fprintf(file, "\n\t");
		}
		fprintf(file, "0x%02x, ", ((const unsigned char *)sdna->data)[index]);
	}
	
	fprintf(file, "\n};\n");
	fprintf(file, "extern const int DNAlen;\n");
	fprintf(file, "const int DNAlen = sizeof(DNAstr);\n");

	fclose(file);
	return true;
}

int main(int argc, char **argv) {
	int status = 0;

	if(!init(argc, argv)) {
		help(argv[0]);
		return -1;
	}
	
	char path[1024];
	
	SDNA *sdna = DNA_sdna_new_empty();
	
	void *ptr = POINTER_OFFSET(sdna->data, sdna->length);

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
		
		LISTBASE_FOREACH(const RCCNode *, node, &parser->nodes) {
			if(ELEM(node->kind, NODE_OBJECT) && ELEM(node->type, OBJ_TYPEDEF)) {
				const RCCObject *object = RT_node_object(node);

				status |= (!DNA_sdna_write_token(sdna, &ptr, ptr, object->identifier)) ? 0xe0 : 0x00;
				status |= (!DNA_sdna_write_type(sdna, &ptr, ptr, object->type)) ? 0xe0 : 0x00;
			}
		}
		
		RT_parser_free(parser);
		RT_file_free(file);
		RT_fcache_free(cache);
	}

	ROSE_assert(DNA_sdna_check(sdna));
	
	LIB_strcpy(path, ARRAY_SIZE(path), binary);
	LIB_strcat(path, ARRAY_SIZE(path), "/");
	LIB_strcat(path, ARRAY_SIZE(path), "dna.c");
	
	if(!build(path, sdna)) {
		DNA_sdna_free(sdna);
		return -1;
	}
	
	DNA_sdna_free(sdna);
	
	return status;
}

/*

--src "C:/Users/Jim/source/repos/rose/source/rose/makedna"
--bin "C:/Users/Jim/source/build/rose/source/rose/makedna/intern"

*/
