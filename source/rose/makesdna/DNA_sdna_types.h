#pragma once

#ifdef __cplusplus
extern "C" {
#endif

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
	
	int fields_len;
	struct DNAField *fields;
} DNAStruct;

typedef struct SDNA {
	const char *data;
	
	int data_len;
	int data_alloc;
	
	int types_len;
	struct DNAStruct *types;
} SDNA;

#ifdef __cplusplus
}
#endif
