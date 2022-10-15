#pragma once

#include "lib_compiler.h"
#include "lib_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

	enum {
		LIB_MEMPOOL_NOP = 0 ,
		/** Allow iterating on this mempool.
		*
		* \note this requires that the first four bytes of the elements
		* never begin with 'free' (#FREEWORD).
		* \note order of iteration is only assured to be the
		* order of allocation when no chunks have been freed. */
		LIB_MEMPOOL_ALLOW_ITER = ( 1 << 0 ) ,
	};

	typedef struct LIB_mempool LIB_mempool;

	/** Create a mempool, capable of allocating space fast.
	* \param esize The size of each element in the mempool, this is static and cannot change.
	* \param elen The number of elements this mempool should be capable to store initally, you will still have to alloc each 
	* element but they will be already be reserved.
	* \param pchunk The number of elements each chunk should hold.
	* \param flag any compination of the LIB_MEMPOOL_* flags.
	* \return Returns the mempool. */
	LIB_mempool *LIB_mempool_create ( size_t esize , size_t elen , size_t pchunk , unsigned int flag ) ATTR_MALLOC ATTR_WARN_UNUSED_RESULT ATTR_RETURNS_NONNULL ;

	/** Allocate a new memory block in the mempool, 
	* never free this memory using MEM_freeN or free use #LIB_mempool_free instead.
	* \param pool The mempool in which we want to allocate memory in.
	* \return Returns the buffer to the allocated memory, 
	* the size of this memory will be the \a esize probvided in #LIB_mempool_create when the mempool was created. */
	void *LIB_mempool_alloc ( LIB_mempool *pool ) ATTR_MALLOC ATTR_WARN_UNUSED_RESULT ATTR_RETURNS_NONNULL ATTR_NONNULL ( 1 );

	/** Allocate a new memory block in the mempool an init with zero, 
	* never free this memory using MEM_freeN or free use #LIB_mempool_free instead.
	* \param pool The mempool in which we want to allocate memory in.
	* \return Returns the buffer to the allocated memory, 
	* the size of this memory will be the \a esize probvided in #LIB_mempool_create when the mempool was created. */
	void *LIB_mempool_calloc ( LIB_mempool *pool ) ATTR_MALLOC ATTR_WARN_UNUSED_RESULT ATTR_RETURNS_NONNULL ATTR_NONNULL ( 1 );

	/** Free an allocated element from the mempool.
	* \note Doesn't protect against double frees, take care! */
	void LIB_mempool_free ( LIB_mempool *pool , void *addr ) ATTR_NONNULL ( 1 , 2 ) ;

	/** Empty the pool, as if it were just created.
	* \param pool The pool to clear.
	* \param totelem_reserve Optionally reserve how many items should be kept from clearing. */
	void LIB_mempool_clear_ex ( LIB_mempool *pool , size_t totelem_reserve ) ATTR_NONNULL ( 1 );

	/** Empty the pool, as if it were just created. 
	* Same as calling #LIB_mempool_clear_ex ( pool , -1 ) */
	void LIB_mempool_clear ( LIB_mempool *pool ) ATTR_NONNULL ( 1 );

	/** Destroy and deallocate every element in the pool.
	* \param pool The mempool to destroy. */
	void LIB_mempool_destroy ( LIB_mempool *pool ) ATTR_NONNULL ( 1 );
	
	/* Fetch the number of elements currently active inside the mempool.
	* \param pool The mempool.
	* \return The number of elements in the mempool. */
	size_t LIB_mempool_len ( const LIB_mempool *pool ) ATTR_NONNULL ( 1 );

	/** Fetch an element in the mempool , LIB_MEMPOOL_ALLOW_ITER should have been specified 
	* when the mempool was generated.
	* \param pool The mempool to fetch the element from.
	* \param index The index of the element in the mempool.
	* \node Do not use to iterate elements in the mempool, use LIB_mempool_iterstep instead.
	* \return The data in the specified element. */
	void *LIB_mempool_findelem ( LIB_mempool *pool , size_t index ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 );

	/** Fill in \a data with pointers to each element of the mempool,
	* to create lookup table.
	*
	* \param pool: Pool to create a table from.
	* \param data: array of pointers at least the size of 'pool->totused' */
	void LIB_mempool_as_table ( LIB_mempool *pool , void **data ) ATTR_NONNULL ( 1 , 2 );
	
	// A version of #LIB_mempool_as_table that allocates and returns the data.
	void **LIB_mempool_as_tableN ( LIB_mempool *pool , const char *allocstr ) ATTR_MALLOC ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 , 2 );
	
	// Fill in \a data with the contents of the mempool.
	void LIB_mempool_as_array ( LIB_mempool *pool , void *data ) ATTR_NONNULL ( 1 , 2 );

	// A version of #BLI_mempool_as_array that allocates and returns the data.
	void *LIB_mempool_as_arrayN ( LIB_mempool *pool , const char *allocstr ) ATTR_MALLOC ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( 1 , 2 );

	typedef struct LIB_mempool_iter {
		struct LIB_mempool *Pool;
		struct LIB_mempool_chunk *Chunk;
		size_t CurIndex;
	} LIB_mempool_iter;

	// Initialize a new mempool iterator, #LIB_MEMPOOL_ALLOW_ITER flag must be set.
	void LIB_mempool_iternew ( LIB_mempool *pool , LIB_mempool_iter *iter ) ATTR_NONNULL ( );

	// Step over the iterator, returning the mempool item or NULL.
	void *LIB_mempool_iterstep ( LIB_mempool_iter *iter ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );


#ifdef __cplusplus
}
#endif
