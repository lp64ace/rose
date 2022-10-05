#include "lib/lib_mempool.h"
#include "lib/lib_alloc.h"
#include "lib/lib_utildefines.h"
#include "lib/lib_error.h"

#include <stdlib.h>

#if defined(__BIG_ENDIAN__)
/* Big Endian */
#  define MAKE_ID(a, b, c, d) ((int)(a) << 24 | (int)(b) << 16 | (c) << 8 | (d))
#  define MAKE_ID_8(a, b, c, d, e, f, g, h) \
    ((int64_t)(a) << 56 | (int64_t)(b) << 48 | (int64_t)(c) << 40 | (int64_t)(d) << 32 | \
     (int64_t)(e) << 24 | (int64_t)(f) << 16 | (int64_t)(g) << 8 | (h))
#elif defined(__LITTLE_ENDIAN__)
/* Little Endian */
#  define MAKE_ID(a, b, c, d) ((int)(d) << 24 | (int)(c) << 16 | (b) << 8 | (a))
#  define MAKE_ID_8(a, b, c, d, e, f, g, h) \
    ((int64_t)(h) << 56 | (int64_t)(g) << 48 | (int64_t)(f) << 40 | (int64_t)(e) << 32 | \
     (int64_t)(d) << 24 | (int64_t)(c) << 16 | (int64_t)(b) << 8 | (a))
#else
#  error Either __BIG_ENDIAN__ or __LITTLE_ENDIAN__ must be defined.
#endif

/**
 * Important that this value is an is _not_  aligned with `sizeof(void *)`.
 * So having a pointer to 2/4/8... aligned memory is enough to ensure
 * the `freeword` will never be used.
 * To be safe, use a word that's the same in both directions. */
#define FREEWORD \
  ((sizeof(void *) > sizeof(int32_t)) ? MAKE_ID_8('e', 'e', 'r', 'f', 'f', 'r', 'e', 'e') : \
                                        MAKE_ID('e', 'f', 'f', 'e'))

#define USE_CHUNK_POW2

/**
* A free element from #LIB_mempool_chunk. Data is cast to this type and stored in
* #LIB_mempool.free as a single linked list, each item #LIB_mempool.esize large.
*
* Each element represents a block which LIB_mempool_alloc may return.
*/
typedef struct LIB_freenode {
        struct LIB_freenode *mNext;
        // Used to identify this as a freed node.
        intptr_t mFreeword;
} LIB_freenode;

/**
* A chunk of memory in the mempool stored in
* #LIB_mempool.chunks as a double linked list.
*/
typedef struct LIB_mempool_chunk {
        struct LIB_mempool_chunk *mNext;
} LIB_mempool_chunk;

/**
 * The mempool, stores and tracks memory \a chunks and elements within those chunks \a free.
 */
struct LIB_mempool {
        // Single linked list of allocated chunks.
        LIB_mempool_chunk *mChunks;
        // Keep a pointer to the last, so we can append new chunks there
        // this is needed for iteration so we can loop over chunks in the order added.
        LIB_mempool_chunk *mChunkTail;

        // Element size in bytes.
        size_t mElemSize;
        // Chunk size in bytes.
        size_t mChunkSize;
        // Number of elements per chunk.
        size_t mPerChunk;

        unsigned int mFlag;

        // Free element list. Interleaved into chunk data.
        LIB_freenode *mFree;
        // Use to know how many chunks to keep for #LIB_mempool_clear.
        unsigned int mMaxChunks;
        // Number of elements currently in use.
        unsigned int mTotUsed;
};

 /**
  * The 'used' word just needs to be set to something besides FREEWORD.
  */
#define USEDWORD MAKE_ID('u', 's', 'e', 'd')

#define MEMPOOL_ELEM_SIZE_MIN (sizeof(void *) * 2)

#define CHUNK_DATA(chunk) ((void *)((chunk) + 1))

#define NODE_STEP_NEXT(node) ((void *)((char *)(node) + esize))
#define NODE_STEP_PREV(node) ((void *)((char *)(node)-esize))

/** Extra bytes implicitly used for every chunk alloc. */
#define CHUNK_OVERHEAD (size_t)(MEM_SIZE_OVERHEAD + sizeof(LIB_mempool_chunk))

#ifdef USE_CHUNK_POW2
static size_t power_of_2_max_u ( size_t x ) {
        x -= 1;
        x = x | ( x >> 1 );
        x = x | ( x >> 2 );
        x = x | ( x >> 4 );
        x = x | ( x >> 8 );
        x = x | ( x >> 16 );
        x = x | ( x >> 32 );
        x = x | ( x >> 64 );
        x = x | ( x >> 128 );
        x = x | ( x >> 256 );
        return x + 1;
}
#endif

inline LIB_mempool_chunk *mempool_chunk_find ( LIB_mempool_chunk *head , size_t index ) {
        while ( index-- && head ) {
                head = head->mNext;
        }
        return head;
}

/**
* \return the number of chunks to allocate based on how many elements are needed.
*
* \note for small pools 1 is a good default, the elements need to be initialized,
* adding overhead on creation which is redundant if they aren't used.
*/
inline size_t mempool_maxchunks ( const size_t elem_num , const size_t pchunk ) {
        return ( elem_num <= pchunk ) ? 1 : ( ( elem_num / pchunk ) + 1 );
}

static LIB_mempool_chunk *mempool_chunk_alloc ( LIB_mempool *pool ) {
        return ( LIB_mempool_chunk * ) MEM_callocN ( sizeof ( LIB_mempool_chunk ) + ( size_t ) pool->mChunkSize , __func__ );
}

/**
 * Initialize a chunk and add into \a pool->chunks
 *
 * \param pool: The pool to add the chunk into.
 * \param mpchunk: The new uninitialized chunk (can be malloc'd)
 * \param last_tail: The last element of the previous chunk
 * (used when building free chunks initially)
 * \return The last chunk,
 */
static LIB_freenode *mempool_chunk_add ( LIB_mempool *pool ,
                                         LIB_mempool_chunk *mpchunk ,
                                         LIB_freenode *last_tail ) {
        const size_t esize = pool->mElemSize;
        LIB_freenode *curnode = ( LIB_freenode * ) CHUNK_DATA ( mpchunk );
        size_t j;

        /* append */
        if ( pool->mChunkTail ) {
                pool->mChunkTail->mNext = mpchunk;
        } else {
                LIB_assert ( pool->mChunks == NULL );
                pool->mChunks = mpchunk;
        }

        mpchunk->mNext = NULL;
        pool->mChunkTail = mpchunk;

        if ( UNLIKELY ( pool->mFree == NULL ) ) {
                pool->mFree = curnode;
        }

        /* loop through the allocated data, building the pointer structures */
        j = pool->mPerChunk;
        if ( pool->mFlag & LIB_MEMPOOL_ALLOW_ITER ) {
                while ( j-- ) {
                        curnode->mNext = ( LIB_freenode * ) NODE_STEP_NEXT ( curnode );
                        curnode->mFreeword = FREEWORD;
                        curnode = curnode->mNext;
                }
        } else {
                while ( j-- ) {
                        curnode->mNext = ( LIB_freenode * ) NODE_STEP_NEXT ( curnode );
                        curnode = curnode->mNext;
                }
        }

        /* terminate the list (rewind one)
         * will be overwritten if 'curnode' gets passed in again as 'last_tail' */
        curnode = ( LIB_freenode * ) NODE_STEP_PREV ( curnode );
        curnode->mNext = NULL;

        /* final pointer in the previously allocated chunk is wrong */
        if ( last_tail ) {
                last_tail->mNext = ( LIB_freenode * ) CHUNK_DATA ( mpchunk );
        }

        return curnode;
}

static void mempool_chunk_free ( LIB_mempool_chunk *mpchunk ) {
        MEM_freeN ( mpchunk );
}

static void mempool_chunk_free_all ( LIB_mempool_chunk *mpchunk ) {
        LIB_mempool_chunk *mpchunk_next;

        for ( ; mpchunk; mpchunk = mpchunk_next ) {
                mpchunk_next = mpchunk->mNext;
                mempool_chunk_free ( mpchunk );
        }
}

LIB_mempool *LIB_mempool_create ( size_t esize , size_t elem_num , size_t pchunk , unsigned int flag ) {
        LIB_mempool *pool;
        LIB_freenode *last_tail = NULL;
        size_t i , maxchunks;

        /* allocate the pool structure */
        pool = ( LIB_mempool * ) MEM_mallocN ( sizeof ( LIB_mempool ) , "memory pool" );

        /* set the elem size */
        if ( esize < ( int ) MEMPOOL_ELEM_SIZE_MIN ) {
                esize = ( int ) MEMPOOL_ELEM_SIZE_MIN;
        }

        if ( flag & LIB_MEMPOOL_ALLOW_ITER ) {
                esize = ROSE_MAX ( esize , sizeof ( LIB_freenode ) );
        }

        maxchunks = mempool_maxchunks ( elem_num , pchunk );

        pool->mChunks = NULL;
        pool->mChunkTail = NULL;
        pool->mElemSize = esize;

        /* Optimize chunk size to powers of 2, accounting for slop-space. */
#ifdef USE_CHUNK_POW2
        {
                LIB_assert ( power_of_2_max_u ( pchunk * esize ) > CHUNK_OVERHEAD );
                pchunk = ( power_of_2_max_u ( pchunk * esize ) - CHUNK_OVERHEAD ) / esize;
        }
#endif

        pool->mChunkSize = esize * pchunk;

        /* Ensure this is a power of 2, minus the rounding by element size. */
#if defined(USE_CHUNK_POW2) && !defined(NDEBUG)
        {
                size_t final_size = ( size_t ) MEM_SIZE_OVERHEAD + ( size_t ) sizeof ( LIB_mempool_chunk ) + pool->mChunkSize;
                LIB_assert ( ( ( size_t ) power_of_2_max_u ( final_size ) - final_size ) < pool->mElemSize );
        }
#endif

        pool->mPerChunk = pchunk;
        pool->mFlag = flag;
        pool->mFree = NULL; /* mempool_chunk_add assigns */
        pool->mMaxChunks = maxchunks;
        pool->mTotUsed = 0;

        if ( elem_num ) {
                /* Allocate the actual chunks. */
                for ( i = 0; i < maxchunks; i++ ) {
                        LIB_mempool_chunk *mpchunk = mempool_chunk_alloc ( pool );
                        last_tail = mempool_chunk_add ( pool , mpchunk , last_tail );
                }
        }

        return pool;
}

void *LIB_mempool_alloc ( LIB_mempool *pool ) {
        LIB_freenode *free_pop;

        if ( UNLIKELY ( pool->mFree == NULL ) ) {
                /* Need to allocate a new chunk. */
                LIB_mempool_chunk *mpchunk = mempool_chunk_alloc ( pool );
                mempool_chunk_add ( pool , mpchunk , NULL );
        }

        free_pop = pool->mFree;

        LIB_assert ( pool->mChunkTail->mNext == NULL );

        if ( pool->mFlag & LIB_MEMPOOL_ALLOW_ITER ) {
                free_pop->mFreeword = USEDWORD;
        }

        pool->mFree = free_pop->mNext;
        pool->mTotUsed++;

        return ( void * ) free_pop;
}

void *LIB_mempool_calloc ( LIB_mempool *pool ) {
        void *retval = LIB_mempool_alloc ( pool );
        memset ( retval , 0 , ( size_t ) pool->mElemSize );
        return retval;
}

void LIB_mempool_free ( LIB_mempool *pool , void *addr ) {
        LIB_freenode *newhead = ( LIB_freenode * ) addr;

        if ( pool->mFlag & LIB_MEMPOOL_ALLOW_ITER ) {
                newhead->mFreeword = FREEWORD;
        }

        newhead->mNext = pool->mFree;
        pool->mFree = newhead;

        pool->mTotUsed--;

        /* Nothing is in use; free all the chunks except the first. */
        if ( UNLIKELY ( pool->mTotUsed == 0 ) && ( pool->mChunks->mNext ) ) {
                const size_t esize = pool->mElemSize;
                LIB_freenode *curnode;
                size_t j;
                LIB_mempool_chunk *first;

                first = pool->mChunks;
                mempool_chunk_free_all ( first->mNext );
                first->mNext = NULL;
                pool->mChunkTail = first;

                curnode = ( LIB_freenode * ) CHUNK_DATA ( first );
                pool->mFree = curnode;

                j = pool->mPerChunk;
                while ( j-- ) {
                        curnode->mNext = ( LIB_freenode * ) NODE_STEP_NEXT ( curnode );
                        curnode = curnode->mNext;
                }
                curnode = ( LIB_freenode * ) NODE_STEP_PREV ( curnode );
                curnode->mNext = NULL; /* terminate the list */
        }
}

void LIB_mempool_clear_ex ( LIB_mempool *pool , const size_t totelem_reserve ) {
        LIB_mempool_chunk *mpchunk;
        LIB_mempool_chunk *mpchunk_next;
        size_t maxchunks;

        LIB_mempool_chunk *chunks_temp;
        LIB_freenode *last_tail = NULL;

        if ( totelem_reserve == size_t ( -1 ) ) {
                maxchunks = pool->mMaxChunks;
        } else {
                maxchunks = mempool_maxchunks ( totelem_reserve , pool->mPerChunk );
        }

        /* Free all after 'pool->maxchunks'. */
        mpchunk = mempool_chunk_find ( pool->mChunks , maxchunks - 1 );
        if ( mpchunk && mpchunk->mNext ) {
                /* terminate */
                mpchunk_next = mpchunk->mNext;
                mpchunk->mNext = NULL;
                mpchunk = mpchunk_next;

                do {
                        mpchunk_next = mpchunk->mNext;
                        mempool_chunk_free ( mpchunk );
                } while ( ( mpchunk = mpchunk_next ) );
        }

        /* re-initialize */
        pool->mFree = NULL;
        pool->mTotUsed = 0;

        chunks_temp = pool->mChunks;
        pool->mChunks = NULL;
        pool->mChunkTail = NULL;

        while ( ( mpchunk = chunks_temp ) ) {
                chunks_temp = mpchunk->mNext;
                last_tail = mempool_chunk_add ( pool , mpchunk , last_tail );
        }
}

void LIB_mempool_clear ( LIB_mempool *pool ) {
        LIB_mempool_clear_ex ( pool , size_t ( -1 ) );
}

size_t LIB_mempool_len ( const LIB_mempool *pool ) {
        return ( size_t ) pool->mTotUsed;
}

void *LIB_mempool_findelem ( LIB_mempool *pool , size_t index ) {
        LIB_assert ( pool->mFlag & LIB_MEMPOOL_ALLOW_ITER );

        if ( index < pool->mTotUsed ) {
                /* We could have some faster mem chunk stepping code inline. */
                LIB_mempool_iter iter;
                void *elem;
                LIB_mempool_iternew ( pool , &iter );
                for ( elem = LIB_mempool_iterstep ( &iter ); index-- != 0; elem = LIB_mempool_iterstep ( &iter ) ) {
                        // pass
                }
                return elem;
        }

        return NULL;
}

void LIB_mempool_as_table ( LIB_mempool *pool , void **data ) {
        LIB_mempool_iter iter;
        void *elem;
        void **p = data;
        LIB_assert ( pool->mFlag & LIB_MEMPOOL_ALLOW_ITER );
        LIB_mempool_iternew ( pool , &iter );
        while ( ( elem = LIB_mempool_iterstep ( &iter ) ) ) {
                *p++ = elem;
        }
        LIB_assert ( ( size_t ) ( p - data ) == pool->mTotUsed );
}

void LIB_mempool_iternew ( LIB_mempool *pool , LIB_mempool_iter *iter ) {
        LIB_assert ( pool->mFlag & LIB_MEMPOOL_ALLOW_ITER );

        iter->Pool = pool;
        iter->Chunk = pool->mChunks;
        iter->CurIndex = 0;
}

void *LIB_mempool_iterstep ( LIB_mempool_iter *iter ) {
        if ( UNLIKELY ( iter->Chunk == NULL ) ) {
                return NULL;
        }

        const size_t esize = iter->Pool->mElemSize;
        LIB_freenode *curnode = ( LIB_freenode * ) POINTER_OFFSET ( CHUNK_DATA ( iter->Chunk ) , ( esize * iter->CurIndex ) );
        LIB_freenode *ret;
        do {
                ret = curnode;

                if ( ++iter->CurIndex != iter->Pool->mPerChunk ) {
                        curnode = ( LIB_freenode * ) POINTER_OFFSET ( curnode , esize );
                } else {
                        iter->CurIndex = 0;
                        iter->Chunk = iter->Chunk->mNext;
                        if ( UNLIKELY ( iter->Chunk == NULL ) ) {
                                return ( ret->mFreeword == FREEWORD ) ? NULL : ret;
                        }
                        curnode = ( LIB_freenode * ) CHUNK_DATA ( iter->Chunk );
                }
        } while ( ret->mFreeword == FREEWORD );

        return ret;
}

void LIB_mempool_destroy ( LIB_mempool *pool ) {
        mempool_chunk_free_all ( pool->mChunks );
        MEM_freeN ( pool );
}
