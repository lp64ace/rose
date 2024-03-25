#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "MEM_alloc.h"

#include "LIB_assert.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "DNA_sdna_types.h"

static const char *includefiles[] = {
#include "dna_includes_as_strings.h"
};

#define DEBUG_PRINT_LEVEL 0

/* -------------------------------------------------------------------- */
/** \name Variables
 * \{ */

int numtypes;

struct SDNA_Struct *types;

/* \} */

/* -------------------------------------------------------------------- */
/** \name Utils
 * \{ */

bool has_type(const char *name) {
	if (name[0] == '\0') {
		return false;
	}

	for (int index = 0; index < numtypes; index++) {
		if (STREQLEN(types[index].name, name, ARRAY_SIZE(types[index].name))) {
			return types[index].type != -1;
		}
	}

	return false;
}

int add_type(const char *name, size_t size) {
	if (name[0] == '\0') {
		return -1;
	}

	int index = numtypes;

	for (int i = 0; i < numtypes; i++) {
		if (STREQLEN(types[i].name, name, ARRAY_SIZE(types[i].name))) {
			/** We have already added it as a opaque type, update it if possible! */
			if (types[i].type == -1) {
				index = i;
			}
			else {
				/** Try to update the size, otherwise just return that it exists. */
				if (size != LIB_NPOS && size) {
					types[i].size = size;
				}
				return i;
			}
		}
	}

	if (index == numtypes) {
		numtypes++;
	}

	types = MEM_reallocN(types, sizeof(SDNA_Struct) * numtypes);

	SDNA_Struct *container = &types[index];

	LIB_strncpy(container->name, name, ARRAY_SIZE(container->name));
	/** Usually structs will be initialized with zero size since we parse the name first before we iterate the members. */
	container->type = (size == LIB_NPOS) ? -1 : index;
	container->size = (size == LIB_NPOS) ? 0 : size;
	container->nmembers = 0;
	container->members = NULL;

	return index;
}

int add_type_opaque(char *name) {
	return add_type(name, LIB_NPOS);
}

SDNA_StructMember *add_member(int container_type, int member_type, int ptr, size_t length) {
	SDNA_StructMember *member = MEM_mallocN(sizeof(SDNA_StructMember), "makesdna::StructMember");

	member->type = member_type;
	member->ptr = ptr;
	member->length = length;

	SDNA_Struct *container = &types[container_type];

	int index = container->nmembers++;
	/** Resize to store the new member. */
	container->members = MEM_reallocN_id(container->members, sizeof(*container->members) * container->nmembers, "makesdna::Struct::Members");
	container->members[index] = member;

	return index;
}

SDNA_StructFunction *add_function(int container_type, int return_type, int ptr, size_t length) {
	SDNA_StructFunction *func = MEM_mallocN(sizeof(SDNA_StructFunction), "makesdna::StructFunction");

	func->type = return_type;
	func->ptr = ptr | SDNA_PTR_FUNCTION;
	func->arr_length = length;

	func->nargs = 0;
	func->type_args = NULL;
	func->ptr_args = NULL;
	func->len_args = NULL;

	SDNA_Struct *container = &types[container_type];

	int index = container->nmembers++;
	/** Resize to store the new member. */
	container->members = MEM_reallocN_id(container->members, sizeof(*container->members) * container->nmembers, "makesdna::Struct::Members");
	container->members[index] = func;

	return index;
}

void add_function_argument(struct SDNA_StructFunction *func, int arg_type, int arg_ptr, size_t arg_len) {
	int index = func->nargs++;

	func->type_args = MEM_reallocN_id(func->type_args, sizeof(*func->type_args) * func->nargs, "makesdna::StructFunction::ArgumentTypes");
	func->ptr_args = MEM_reallocN_id(func->ptr_args, sizeof(*func->ptr_args) * func->nargs, "makesdna::StructFunction::ArgumentPtr");
	func->len_args = MEM_reallocN_id(func->len_args, sizeof(*func->len_args) * func->nargs, "makesdna::StructFunction::ArgumentLength");

	func->type_args[index] = arg_type;
	func->ptr_args[index] = arg_ptr;
	func->len_args[index] = arg_len;
}

size_t type_size_calculate(int type) {
	if (types[type].size != 0) {
		return types[type].size;
	}
	if (types[type].size == LIB_NPOS) {
		ROSE_assert_unreachable();
	}
	SDNA_Struct *t = &types[type];

	size_t sz = 0;
	for (int i = 0; i < t->nmembers; i++) {
		SDNA_StructMember *member = t->members[i];
		/** Layout matches so the number of function we have or the number of pointers is at the same position, no need to cast. */
		if (SDNA_PTR_LVL(member->ptr) || SDNA_PTR_IS_FUNCTION(member->ptr)) {
			sz += sizeof(void *) * ROSE_MAX(1, member->length);
		}
		else {
			sz += type_size_calculate(member->type) * ROSE_MAX(1, member->length);
		}
	}
	return t->size = sz;
}

/* \} */

static void *read_file_data(const char *filepath, int *r_length) {
#ifdef WIN32
	FILE *fp = fopen(filepath, "rb");
#else
	FILE *fp = fopen(filepath, "r");
#endif

	void *data = NULL;

	if (!fp) {
		return data = (void *)(*r_length = 0);
	}

	fseek(fp, 0L, SEEK_END);
	*r_length = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	if (*r_length == -1) {
		fclose(fp);
		return data = (void *)(*r_length = 0);
	}

	data = MEM_callocN(*r_length + 1, "makesdna::file_data");
	if (!data) {
		fclose(fp);
		return data = (void *)(*r_length = 0);
	}

	if (fread(data, *r_length, 1, fp) != 1) {
		MEM_freeN(data);
		fclose(fp);
		return data = (void *)(*r_length = 0);
	}

	fclose(fp);
	return data;
}

enum {
	LLVM_CHARACHTERS_NONE = 0,
	LLVM_CHARACHTERS_SPACES = (1 << 0),
	LLVM_CHARACHTERS_CURLY_BRACKETS = (1 << 1),
	LLVM_CHARACHTERS_BRACKETS = (1 << 2),
	LLVM_CHARACHTERS_PARENTHESIS = (1 << 3),
	LLVM_CHARACHTERS_ALPHA = (1 << 4),
	LLVM_CHARACHTERS_DIGITS = (1 << 5),
	LLVM_CHARACHTERS_UNDERSCORE = (1 << 5),
	LLVM_CHARACHTERS_SYMBOL = (1 << 6),

	/** This will return the full line. */
	LLVM_CHARACHTERS_LINE = (1 << 7),
	/** This will return any charachters that are not spaces. */
	LLVM_CHARACHTERS_NOSPACE = (1 << 8),
	/** This will return any charachters that are not '[' or ']'. */
	LLVM_CHARACHTERS_NOCRACKETS = (1 << 9),
	/** This allowed charachters that may have digits after and underscore */
	LLVM_CHARACHTERS_TEXT = LLVM_CHARACHTERS_ALPHA | LLVM_CHARACHTERS_UNDERSCORE | (1 << 4),
};

bool llvm_is_allowed(const char what, int offset, int flag) {
	if ((flag & LLVM_CHARACHTERS_LINE) != 0) {
		/** Empty lines are skipped, no reason to return an empty string. */
		return (what != '\n') || (offset == 0);
	}
	if ((flag & LLVM_CHARACHTERS_NOSPACE) != 0) {
		return !isspace(what);
	}
	if ((flag & LLVM_CHARACHTERS_NOCRACKETS) != 0) {
		return (what != '[' && what != ']');
	}
	if ((flag & LLVM_CHARACHTERS_SPACES) != 0 && isspace(what)) {
		return true;
	}
	if ((flag & LLVM_CHARACHTERS_CURLY_BRACKETS) != 0 && (what == '{' || what == '}')) {
		return true;
	}
	if ((flag & LLVM_CHARACHTERS_BRACKETS) != 0 && (what == '[' || what == ']')) {
		return true;
	}
	if ((flag & LLVM_CHARACHTERS_PARENTHESIS) != 0 && (what == '(' || what == ')')) {
		return true;
	}
	if ((flag & LLVM_CHARACHTERS_ALPHA) != 0 && isalpha(what)) {
		return true;
	}
	if ((flag & LLVM_CHARACHTERS_DIGITS) != 0 && isdigit(what)) {
		return true;
	}
	if ((flag & LLVM_CHARACHTERS_UNDERSCORE) != 0 && what == '_') {
		return true;
	}
	if ((flag & LLVM_CHARACHTERS_TEXT) != 0 && (offset > 0 && isdigit(what))) {
		return true;
	}
}

char *llvm_word(const char *buffer, int *ptr_itr, char *ptr_word, int flag) {
	char *word_itr = ptr_word;
	int itr;
	for (itr = *ptr_itr; buffer[itr] != '\0' && llvm_is_allowed(buffer[itr], itr - *ptr_itr, flag); itr++) {
		if (word_itr == ptr_word && isspace(buffer[itr])) { /** Do not include initial spaces in word. */
			continue;
		}
		*word_itr = buffer[itr];
		word_itr++;
	}
	*(word_itr) = '\0';
	while (isspace(*word_itr) && word_itr > ptr_word) { /** Remove trailing spaces from word. */
		*(word_itr--) = '\0';
	}
	*ptr_itr = itr;
	return ptr_word;
}

bool llvm_single(const char *buffer, int *ptr_itr, char *ptr_word) {
	int parenthesis_depth = 0;
	char *word_itr = ptr_word;
	int itr;
	for (itr = *ptr_itr; buffer[itr] != '\0' && parenthesis_depth >= 0 && (parenthesis_depth > 0 || buffer[itr] != ',') && buffer[itr] != '}'; itr++) {
		if (word_itr == ptr_word && isspace(buffer[itr])) { /** Do not include initial spaces in word. */
			continue;
		}
		if (buffer[itr] == '(') {
			parenthesis_depth++;
		}
		if (buffer[itr] == ')') {
			parenthesis_depth--;
		}
		*word_itr = buffer[itr];
		word_itr++;
	}
	if (parenthesis_depth < 0) {
		word_itr--;
	}
	while (isspace(*word_itr) && word_itr > ptr_word) { /** Remove trailing spaces from word. */
		word_itr--;
	}
	*word_itr = '\0';
	*ptr_itr = itr;
	return (ptr_word < word_itr);
}

void llvm_handle_single(char *llvm, int *ptr_itr, size_t *arr, int *type, int *ptr) {
	char *word = MEM_mallocN(sizeof(*word) * 1024, "llvm_word");

	/** Skip spaces */
	llvm_word(llvm, ptr_itr, word, LLVM_CHARACHTERS_SPACES);
	*arr = 0;
	if (llvm[*ptr_itr] == '[') {
		(*ptr_itr)++;
		llvm_word(llvm, ptr_itr, word, LLVM_CHARACHTERS_SPACES | LLVM_CHARACHTERS_DIGITS);
		*arr = atoll(word);
		llvm_word(llvm, ptr_itr, word, LLVM_CHARACHTERS_SPACES | LLVM_CHARACHTERS_ALPHA);
	}
	llvm_word(llvm, ptr_itr, word, LLVM_CHARACHTERS_SPACES);
	if (llvm[*ptr_itr] == '%') { /** Custom structure, skip the prefix. */
		(*ptr_itr)++;
		if (!STREQ(llvm_word(llvm, ptr_itr, word, LLVM_CHARACHTERS_ALPHA), "struct")) {
			ROSE_assert_unreachable();
		}
		(*ptr_itr)++;
	}
	llvm_word(llvm, ptr_itr, word, LLVM_CHARACHTERS_TEXT);
	*type = add_type_opaque(word);
	for (*ptr = 0; llvm[*ptr_itr] == '*'; (*ptr_itr)++) {
		(*ptr)++;
	}
	if (*arr != 0) {
		if (!llvm[*ptr_itr] == ']') {
			/** We cannot handle function in here! */
			ROSE_assert_unreachable();
		}
		(*ptr_itr)++;
	}

	MEM_freeN(word);
}

bool llvm_step_array(char *llvm, size_t *r_length) {
	char *word = MEM_mallocN(sizeof(*word) * 1024, "llvm_word");
	char *type = MEM_mallocN(sizeof(*type) * 1024, "llvm_word");

	int itr = 0;
	do {
		if (!STREQ(llvm_word(llvm, &itr, word, LLVM_CHARACHTERS_BRACKETS), "[")) {
			MEM_freeN(word);
			MEM_freeN(type);
			return false;
		}
		llvm_word(llvm, &itr, word, LLVM_CHARACHTERS_NOSPACE);
		*r_length = (*r_length == 0) ? atoll(word) : *r_length * atoll(word);
		llvm_word(llvm, &itr, word, LLVM_CHARACHTERS_SPACES);
		if (!STREQ(llvm_word(llvm, &itr, word, LLVM_CHARACHTERS_NOSPACE), "x")) {
			*r_length = 0;
			MEM_freeN(word);
			MEM_freeN(type);
			return false;
		}
		llvm_word(llvm, &itr, word, LLVM_CHARACHTERS_SPACES);
	} while (llvm[itr] == '[');
	llvm_word(llvm, &itr, type, LLVM_CHARACHTERS_NOCRACKETS);

	LIB_strncpy(llvm, type, 1024);

	MEM_freeN(word);
	MEM_freeN(type);
	return true;
}

void llvm_step_type(int struct_type, size_t member_length, char *llvm, size_t *arr, int *type, int *ptr) {
	char *word = MEM_mallocN(sizeof(*word) * 1024, "llvm_word");

	int itr = 0;
	llvm_handle_single(llvm, &itr, arr, type, ptr);
	ROSE_assert(*arr == 0);

	llvm_word(llvm, &itr, word, LLVM_CHARACHTERS_SPACES);
	if (STREQ(llvm_word(llvm, &itr, word, LLVM_CHARACHTERS_PARENTHESIS), "(")) {
		/** Function */
		struct SDNA_StructFunction *func = add_function(struct_type, *type, *ptr, member_length);

		while (llvm_single(llvm, &itr, word)) {
			int sitr = 0;
			llvm_handle_single(word, &sitr, arr, type, ptr);
			itr++;
		}
	}
	else {
		add_member(struct_type, *type, *ptr, member_length);
	}

	MEM_freeN(word);
}

void translate_llvm_struct(const char *name, const char *llvm) {
	if (has_type(name)) {
		return;
	}

	char *word = MEM_mallocN(sizeof(*word) * 1024, "llvm_word");

	/** We propably get the structures in order by in case we don't we mark any structures found in our members as 'opaque'. */
	int struct_type = add_type(name, 0);

	int itr = 1; /** First one is always { */
	while (llvm_single(llvm, &itr, word)) {
		size_t l1 = 0, l2 = 0;
		int type;
		int ptr;

		/** This is pretty ugly and introduces the limitation that we can't have functions as function's parameters which is ok for now! */
		llvm_step_array(word, &l1);
		llvm_step_type(struct_type, l1, word, &l2, &type, &ptr);

		itr++;
	}

	MEM_freeN(word);
}

void translate_llvm_output(const char *llvm) {
	char *line = MEM_mallocN(sizeof(*line) * 1024, "llvm_line");
	char *name = MEM_mallocN(sizeof(*name) * 1024, "llvm_name");
	char *word = MEM_mallocN(sizeof(*word) * 1024, "llvm_word");

	int itr = 0;
	while (llvm[itr] != '\0') {
		llvm_word(llvm, &itr, line, LLVM_CHARACHTERS_LINE);

		int litr = 1;
		if (line[0] == '%') {
			if (STREQ(llvm_word(line, &litr, word, LLVM_CHARACHTERS_ALPHA), "struct")) {
				if (line[litr] == '.') {
					litr++;
					llvm_word(line, &litr, name, LLVM_CHARACHTERS_TEXT);

					llvm_word(line, &litr, word, LLVM_CHARACHTERS_SPACES);
					if (!STREQ(llvm_word(line, &litr, word, LLVM_CHARACHTERS_NOSPACE), "=")) {
						continue;
					}
					llvm_word(line, &litr, word, LLVM_CHARACHTERS_SPACES);
					if (!STREQ(llvm_word(line, &litr, word, LLVM_CHARACHTERS_NOSPACE), "type")) {
						continue;
					}
					llvm_word(line, &litr, word, LLVM_CHARACHTERS_SPACES);
					if (!STREQ(llvm_word(line, &litr, word, LLVM_CHARACHTERS_NOSPACE), "{")) {
						continue;
					}

					translate_llvm_struct(name, line + litr);
				}
			}
		}
	}

	MEM_freeN(line);
	MEM_freeN(name);
	MEM_freeN(word);
}

int main(void) {
	char filename[1024];

	add_type("void", 0);
	add_type("i8", 1);
	add_type("u8", 1);
	add_type("i16", 2);
	add_type("u16", 2);
	add_type("i32", 4);
	add_type("u32", 4);
	add_type("i64", 8);
	add_type("u64", 8);

	add_type("float", 4);
	add_type("double", 8);

	int llvm_length;
	for (size_t i = 0; i < ARRAY_SIZE(includefiles); i++) {
		LIB_snprintf(filename, ARRAY_SIZE(filename), "%s.llvm", includefiles[i]);

		void *llvm = read_file_data(filename, &llvm_length);
		if (llvm_length == 0) {
			printf("Failed to read the DNA llvm output '%s'.\n", filename);
			exit(-1);
		}

		translate_llvm_output(llvm);

		MEM_freeN(llvm);
	}

	for (int i = 0; i < numtypes; i++) {
		if (types[i].size != LIB_NPOS) {
			/** dfs to update the sizes of the structures. */
			type_size_calculate(i);
		}
	}

	return 0;
}
