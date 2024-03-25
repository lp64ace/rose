#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SDNA_StructMember {
	/** An index into #SDNA->types denoting the type of this member. */
	int type;
	/**
	 * This variable indicates the level of indirection (pointers) associated with the variable.
	 *
	 * e.g.
	 *  - For a struct member defined as `void ***member;` #ptr would be 3.
	 *  - For a struct member defined as `int member;` #ptr would be 0.
	 */
	int ptr;
	
	size_t length;
} SDNA_StructMember;

typedef struct SDNA_StructFunction {
	/** An index into #SDNA->types denoting the type of the return value of this function. */
	int type;
	/**
	 * This variable indicates the level of indirection (pointers) associated with
	 * the return variable.
	 */
	int ptr;
	
	size_t arr_length;
	size_t ret_length;
	
	/** Number of arguments this function has. */
	int nargs;
	/** Indices into #SDNA->types denoting the type of the i-th argument. */
	int *type_args;
	/** This variable indicates the level of indirection (pointers) associated with the i-th argument. */
	int *ptr_args;
	/** Indicates the length of the i-th argument if that argument is an array. */
	size_t *len_args;
} SDNA_StructFunction;

/** Special value stored in #SDNA_StructFunction->ptr to distinguish it from normal #SDNA_StructMember. */
#define SDNA_PTR_FUNCTION (1 << 30)

#define SDNA_PTR_LVL(ptr) ((ptr) & ~(SDNA_PTR_FUNCTION))
#define SDNA_PTR_IS_FUNCTION(ptr) (((ptr) & (SDNA_PTR_FUNCTION)) != 0)

typedef struct SDNA_Struct {
	/** An index into #SDNA->types. */
	int type;
	/** The name of this struct. */
	char name[64];
	
	/** Number of members this struct contains. */
	size_t nmembers;
	/** An array of #SDNA_StructMember and #SDNA_StructFunction denoting the members of this struct. */
	void **members;
	
	size_t size;
} SDNA_Struct;

typedef struct SDNA {
	/** Full copy of 'encoded' data (when data_alloc is set, otherwise borrowed). */
	const char *data;
	
	size_t data_len;
	size_t data_alloc;
	
	int type_len;
	
	SDNA_Struct *types;
	
	/** #GHash for faster lookups, we map the name of the structs to the #SDNA_Structs structures. */
	struct GHash *structs_map;
} SDNA;

#ifdef __cplusplus
}
#endif
