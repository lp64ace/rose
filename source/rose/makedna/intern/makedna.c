#include "MEM_guardedalloc.h"

#include "LIB_ghash.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "RT_context.h"
#include "RT_object.h"
#include "RT_parser.h"
#include "RT_source.h"
#include "RT_token.h"

#include "genfile.h"

#include <stdbool.h>
#include <stdio.h>

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
		return false;  // Invalid argument
	}
	return source && binary;  // Return true only if both arguments are provided
}

ROSE_INLINE bool build_dna_c(const char *filepath, SDNA *sdna) {
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

	for (size_t index = 0; index < sdna->length; index++) {
		if (index && index % 16 == 0) {
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

ROSE_INLINE bool build_verify_c(const char *filepath, SDNA *sdna) {
	FILE *file = fopen(filepath, "w");
	if (!file) {
		fprintf(stderr, "Failed to open file '%s' for writing.\n", filepath);
		return false;
	}

	// Write a warning header into the generated file
	fprintf(file, "/* Auto-generated file. Do not edit manually. */\n");
	fprintf(file, "\n");
	fprintf(file, "#include \"DNA_include_all.h\"\n");
	fprintf(file, "\n");
	fprintf(file, "#include \"LIB_utildefines.h\"\n");

	RTCParser *parser = RT_parser_new(NULL);
	{
		SDNA *ndna = DNA_sdna_new_memory(sdna->data, sdna->length);

		const void *ptr = POINTER_OFFSET(ndna->data, 8);
		while (ptr < POINTER_OFFSET(ndna->data, ndna->length)) {
			const RTToken *token;
			if (!DNA_sdna_read_token(ndna, &ptr, ptr, &token)) {
				RT_parser_free(parser);
				return false;
			}
			const RTType *type;
			if (!DNA_sdna_read_type(ndna, &ptr, ptr, &type)) {
				RT_parser_free(parser);
				return false;
			}

			if (type->kind != TP_STRUCT) {
				continue;
			}

			fprintf(file, "\n");
			fprintf(file, "ROSE_STATIC_ASSERT(");
			fprintf(file, "sizeof(%s) == %llu, ", RT_token_as_string(token), RT_parser_size(parser, type));
			fprintf(file, "\"DNA type size verify\");\n");

			for (const RTField *field = RT_type_struct_field_first(type); field; field = field->next) {
				fprintf(file, "ROSE_STATIC_ASSERT(");
				fprintf(file, "offsetof(%s, %s) == %llu, ", RT_token_as_string(token), RT_token_as_string(field->identifier), RT_parser_offsetof(parser, type, field));
				fprintf(file, "\"DNA field offset verify\");\n");
			}
		}

		DNA_sdna_free(ndna);
	}
	RT_parser_free(parser);

	fclose(file);
	return true;
}

RTFileCache *build_virtual_source(const char *virtual_path) {
	RTFileCache *cache = NULL;

	char *includes[ARRAY_SIZE(header_typedef)], *content;
	for (size_t index = 0; index < ARRAY_SIZE(header_typedef); index++) {
		includes[index] = LIB_strformat_allocN("#include \"%s\"\n", header_typedef[index]);
	}
	content = LIB_string_join_arrayN((const char **)includes, ARRAY_SIZE(includes));

	for (size_t index = 0; index < ARRAY_SIZE(header_typedef); index++) {
		MEM_SAFE_FREE(includes[index]);
	}
	cache = RT_fcache_new(virtual_path, content, LIB_strlen(content));

	MEM_freeN(content);
	return cache;
}

int main(int argc, char **argv) {
#ifndef NDEBUG
	MEM_init_memleak_detection();
	MEM_enable_fail_on_memleak();
	MEM_use_guarded_allocator();
#endif

	int status = 0;

	if (!init(argc, argv)) {
		help(argv[0]);
		return -1;
	}

	SDNA *sdna = DNA_sdna_new_empty();

	void *ptr = POINTER_OFFSET(sdna->data, sdna->length);

	char path[1024];
	LIB_strcpy(path, ARRAY_SIZE(path), source);
	LIB_strcat(path, ARRAY_SIZE(path), "/");
	LIB_strcat(path, ARRAY_SIZE(path), "include_all.c");

	RTFileCache *cache = build_virtual_source(path);
	RTFile *file = RT_file_new(path, cache);
	RTCParser *parser = RT_parser_new(file);

	RTToken *conf_tp_size = RT_token_new_virtual_identifier(parser->context, "conf::tp_size");
	status |= (!DNA_sdna_write_token(sdna, &ptr, ptr, conf_tp_size)) ? 0xe0 : 0x00;
	status |= (!DNA_sdna_write_type(sdna, &ptr, ptr, parser->configuration.tp_size)) ? 0xe0 : 0x00;
	RTToken *conf_tp_enum = RT_token_new_virtual_identifier(parser->context, "conf::tp_enum");
	status |= (!DNA_sdna_write_token(sdna, &ptr, ptr, conf_tp_enum)) ? 0xe0 : 0x00;
	status |= (!DNA_sdna_write_type(sdna, &ptr, ptr, parser->configuration.tp_enum)) ? 0xe0 : 0x00;

	if (RT_parser_do(parser)) {
		LISTBASE_FOREACH(const RTNode *, node, &parser->nodes) {
			if (ELEM(node->kind, NODE_OBJECT) && ELEM(node->type, OBJ_TYPEDEF)) {
				const RTObject *object = RT_node_object(node);

				if (object->type->kind != TP_STRUCT) {
					continue;
				}

				status |= (!DNA_sdna_write_token(sdna, &ptr, ptr, object->identifier)) ? 0xe0 : 0x00;
				status |= (!DNA_sdna_write_type(sdna, &ptr, ptr, object->type)) ? 0xe0 : 0x00;
			}
		}
	}

	RT_parser_free(parser);
	RT_file_free(file);
	RT_fcache_free(cache);

	LIB_strcpy(path, ARRAY_SIZE(path), binary);
	LIB_strcat(path, ARRAY_SIZE(path), "/");
	LIB_strcat(path, ARRAY_SIZE(path), "verify.c");

	if (!build_verify_c(path, sdna)) {
		DNA_sdna_free(sdna);
		return -1;
	}

	LIB_strcpy(path, ARRAY_SIZE(path), binary);
	LIB_strcat(path, ARRAY_SIZE(path), "/");
	LIB_strcat(path, ARRAY_SIZE(path), "dna.c");

	if (!build_dna_c(path, sdna)) {
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
