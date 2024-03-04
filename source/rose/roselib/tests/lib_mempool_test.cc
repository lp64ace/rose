#include "LIB_mempool.h"

#include "testing.h"

namespace {

TEST(MemPool, Creation) {
	LIB_mempool *allocator = LIB_mempool_create(sizeof(int), 16, 64, ROSE_MEMPOOL_NOP);
	
	LIB_mempool_destroy(allocator);
}

TEST(MemPool, Allocations) {
	LIB_mempool *allocator = LIB_mempool_create(sizeof(int), 16, 64, ROSE_MEMPOOL_NOP);
	
	for(size_t i = 1; i < 65535; i++) {
		EXPECT_NE(reinterpret_cast<void *>(NULL), LIB_mempool_alloc(allocator));
		
		EXPECT_EQ(i, LIB_mempool_length(allocator));
	}
	
	LIB_mempool_destroy(allocator);
}

TEST(MemPool, Deallocations) {
	LIB_mempool *allocator = LIB_mempool_create(sizeof(int), 16, 64, ROSE_MEMPOOL_NOP);
	
	for(size_t i = 0; i < 65535; i++) {
		void *ptr = LIB_mempool_alloc(allocator);
		
		EXPECT_EQ((size_t)1, LIB_mempool_length(allocator));
		
		if(ptr) {
			LIB_mempool_free(allocator, ptr);
		}
		
		EXPECT_EQ((size_t)0, LIB_mempool_length(allocator));
	}
	
	LIB_mempool_destroy(allocator);
}

}  // namespace
