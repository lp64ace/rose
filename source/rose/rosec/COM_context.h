#ifndef COM_CONTEXT_H
#define COM_CONTEXT_H

#include "LIB_memarena.h"
#include "LIB_mempool.h"
#include "LIB_listbase.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Compilation {
	/**
	 * Every node is allocated using this allocator and everything is deleted when
	 * the lifetime of this compilation expires.
	 */
	struct MemPool *node;
	/**
	 * Every declaration is allocated using this allocator and everything is deleted when
	 * the lifetime of this compilation expires.
	 */
	struct MemPool *decl;
	/**
	 * Every token is allocated using this allocator and everything is deleted when
	 * the lifetime of this compilation expires.
	 */
	struct MemPool *scope;
	/**
	 * Every token is allocated using this allocator and everything is deleted when
	 * the lifetime of this compilation expires.
	 */
	struct MemPool *token;
	/**
	 * Every type is allocated using this allocator and everything is deleted when
	 * the lifetime of this compilation expires.
	 */
	struct MemPool *type;
	
	/**
	 * String literals, array initializers, etc. must be allocated with this memory arena,
	 * because their size may vary.
	 */
	struct MemArena *blob;
	
	ListBase tokens;
	ListBase nodes;
} Compilation;

Compilation *COM_new(void);

/** Preprocessing handles #include, #define, #endif, etc. produces a translation unit that's a "flattened" source file. */
bool COM_preprocess(Compilation *compilation);
/** Syntax Analysis turns tokens into an Abstract Syntax Tree. */
bool COM_syntax(Compilation *compilation);
/** Type checking, scope validation, etc. and annotates the Abstract Syntax Tree with types. */
bool COM_sematic(Compilation *compilation);
/** Converts the Abstract Syntax Tree into target-specific Assembly. */
bool COM_generate(Compilation *compilation);
/** Translates human readable assembly to machine instructions. */
bool COM_assembly(Compilation *compilation);

void COM_free(Compilation *);

#ifdef __cplusplus
}
#endif

#endif