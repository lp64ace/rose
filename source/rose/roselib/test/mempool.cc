#include "MEM_guardedalloc.h"

#include "LIB_mempool.h"
#include "LIB_utildefines.h"

#include "gtest/gtest.h"

namespace {

constexpr size_t __esize = sizeof(int[64]);
constexpr size_t __clength = 64;
constexpr size_t __elength = 64;

TEST(MemPool, SimpleMalloc) {
	MemPool *pool = LIB_memory_pool_create(__esize, __clength, __elength, ROSE_MEMPOOL_NOP);

	ASSERT_NE(LIB_memory_pool_malloc(pool), nullptr);
	ASSERT_NE(LIB_memory_pool_malloc(pool), nullptr);
	ASSERT_NE(LIB_memory_pool_malloc(pool), nullptr);

	LIB_memory_pool_destroy(pool);
}

TEST(MemPool, Malloc) {
	MemPool *pool = LIB_memory_pool_create(__esize, __clength, __elength, ROSE_MEMPOOL_NOP);

	for (size_t index = 0; index < __elength * 2; index++) {
		ASSERT_NE(LIB_memory_pool_malloc(pool), nullptr);
		ASSERT_NE(LIB_memory_pool_malloc(pool), nullptr);
		ASSERT_NE(LIB_memory_pool_malloc(pool), nullptr);
	}

	LIB_memory_pool_destroy(pool);
}

TEST(MemPool, SimpleFree) {
	MemPool *pool = LIB_memory_pool_create(__esize, __clength, __elength, ROSE_MEMPOOL_NOP);

	void *ptr1 = LIB_memory_pool_malloc(pool);
	void *ptr2 = LIB_memory_pool_malloc(pool);
	void *ptr3 = LIB_memory_pool_malloc(pool);
	ASSERT_NE(ptr1, nullptr);
	ASSERT_NE(ptr2, nullptr);
	ASSERT_NE(ptr3, nullptr);
	LIB_memory_pool_free(pool, ptr1);
	LIB_memory_pool_free(pool, ptr2);
	LIB_memory_pool_free(pool, ptr3);

	LIB_memory_pool_destroy(pool);
}

TEST(MemPool, Free) {
	void *ptr[__clength * 4];

	MemPool *pool = LIB_memory_pool_create(__esize, __clength, __elength, ROSE_MEMPOOL_NOP);
	for (size_t index = 0; index < __clength * 4; index++) {
		ASSERT_NE(ptr[index] = LIB_memory_pool_malloc(pool), nullptr);
	}
	for (size_t index = 0; index < __clength * 4; index++) {
		LIB_memory_pool_free(pool, ptr[index]);
	}
	for (size_t index = 0; index < __clength * 4; index++) {
		ASSERT_NE(ptr[index] = LIB_memory_pool_malloc(pool), nullptr);
	}
	LIB_memory_pool_destroy(pool);
}

TEST(MemPool, SimpleClear) {
	MemPool *pool = LIB_memory_pool_create(__esize, __clength, __elength, ROSE_MEMPOOL_NOP);
	for (size_t index = 0; index < __clength * 4; index++) {
		ASSERT_NE(LIB_memory_pool_malloc(pool), nullptr);
	}
	LIB_memory_pool_clear(pool, __elength);
	LIB_memory_pool_destroy(pool);
}

TEST(MemPool, Clear) {
	MemPool *pool = LIB_memory_pool_create(__esize, __clength, __elength, ROSE_MEMPOOL_NOP);
	for (size_t index = 0; index < __clength * 4; index++) {
		ASSERT_NE(LIB_memory_pool_malloc(pool), nullptr);
	}
	LIB_memory_pool_clear(pool, __elength);
	for (size_t index = 0; index < __clength * 4; index++) {
		ASSERT_NE(LIB_memory_pool_malloc(pool), nullptr);
	}
	LIB_memory_pool_destroy(pool);
}

}  // namespace
