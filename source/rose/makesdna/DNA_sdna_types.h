#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * NOTE! This stores structure information about the current version of the application.
 * These structure need to be immutable accross all versions and architectures this program 
 * is built on.
 *
 * This means that size_t and ptrdiff_t types are not allowed, defines that may change with 
 * time are also not allowed, so we do not use #MAX_NAME here we use 64 instead, and we 
 * introduce the limitation that no structure name can exceed the 64 charachters.
 */

typedef struct DNAField {
	char name[64];
	/**
	 * If this field is an array field this is the type of the array elements, 
	 * in the case where the elements are pointers, this is the pointee type 
	 * and the "pointer" is stored as a flag #DNA_FIELD_IS_POINTER.
	 *
	 * Use with caution this might not exist in SDNA.
	 */
	char type[64];
	
	int offset;
	int size;
	int align;
	int array;
	
	int flags;
} DNAField;

/** #DNAField->flags */
enum {
	DNA_FIELD_IS_POINTER = (1 << 0),
	DNA_FIELD_IS_ARRAY = (1 << 1),
	DNA_FIELD_IS_FUNCTION = (1 << 2),
};

typedef struct DNAStruct {
	char name[64];
	
	int size;
	int align;
	
	int fields_len;
	struct DNAField *fields;
} DNAStruct;

typedef struct SDNA {
	const char *data;
	
	int data_len;
	int data_alloc;
	
	int flags;
	
	int types_len;
	struct DNAStruct *types;
} SDNA;

/** #SDNA->flags */
enum {
	SDNA_ENDIAN_SWAP = (1 << 0),
};

typedef struct RHead {
	/** The type of data that follows. */
	int code;
	/** The length of the data that follows. */
	int length;
	/** The address of the data when the file was written. */
	const void *address;
	/** The index of the structure type in SDNA->types. */
	int struct_nr;
	/** The number of different structures stored. */
	int nr;
} RHead;

#ifdef __cplusplus
}
#endif
