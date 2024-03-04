#include "LIB_ghash.h"
#include "LIB_utildefines.h"

#include <map>
#include <set>

#include "testing.h"

namespace {

/* -------------------------------------------------------------------- */
/** \name GHash
 * \{ */

TEST(GHash, Creation) {
	GHash *container = LIB_ghash_ptr_new("rose::test::GHash");

	LIB_ghash_free(container, NULL, NULL);
}

TEST(GHash, Insertions) {
	GHash *container = LIB_ghash_ptr_new("rose::test::GHash");
	
	for(int i = 1; i < 10; i++) {
		void *key = POINTER_FROM_UINT(i);
		void *val = POINTER_FROM_UINT(10 - i);
		
		LIB_ghash_insert(container, key, val);
	}
	
	for(int i = 1; i < 10; i++) {
		void *key = POINTER_FROM_UINT(i);
		void *val = POINTER_FROM_UINT(10 - i);
		
		EXPECT_EQ(val, LIB_ghash_lookup(container, key));
	}

	LIB_ghash_free(container, NULL, NULL);
}

TEST(GHash, Deletions) {
	GHash *container = LIB_ghash_ptr_new("rose::test::GHash");

	for(int i = 1; i < 10; i++) {
		void *key = POINTER_FROM_UINT(i);
		void *val = POINTER_FROM_UINT(10 - i);
		
		LIB_ghash_insert(container, key, val);
	}
	
	for(int i = 1; i < 10; i++) {
		void *key = POINTER_FROM_UINT(i);
		void *val = POINTER_FROM_UINT(10 - i);
		
		EXPECT_EQ(val, LIB_ghash_popkey(container, key, NULL));
	}

	LIB_ghash_free(container, NULL, NULL);
}

TEST(GHash, Master) {
	GHash *container = LIB_ghash_ptr_new("rose::test::GHash");

	size_t length = 65535;

	std::map<int, int> reference;
	
	for (size_t i = 0; i < length; i++) {
		int key = rand() % 10;

		if ((i & 1) == 0) {
			int val = rand() % 10;
			
			if(reference.find(key) == reference.end()) {
				EXPECT_EQ(true, LIB_ghash_reinsert(container, POINTER_FROM_UINT(key), POINTER_FROM_UINT(val), NULL, NULL));
			} else {
				void *old = LIB_ghash_lookup(container, POINTER_FROM_UINT(key));
				
				EXPECT_EQ(POINTER_FROM_UINT(reference[key]), old);
				EXPECT_EQ(false, LIB_ghash_reinsert(container, POINTER_FROM_UINT(key), POINTER_FROM_UINT(val), NULL, NULL));
			}
			reference[key] = val;
		}
		else {
			if (reference.find(key) == reference.end()) {
				EXPECT_EQ(NULL, LIB_ghash_lookup(container, POINTER_FROM_UINT(key)));
			}
			else {
				void *val = LIB_ghash_lookup(container, POINTER_FROM_UINT(key));
				
				EXPECT_EQ(POINTER_FROM_UINT(reference[key]), val);
			}
		}
	}

	LIB_ghash_free(container, NULL, NULL);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name GSet
 * \{ */

TEST(GSet, Creation) {
	GSet *container = LIB_gset_ptr_new("rose::test::GSet");

	LIB_gset_free(container, NULL);
}

TEST(GSet, Insertions) {
	GSet *container = LIB_gset_ptr_new("rose::test::GSet");

	EXPECT_EQ(true, LIB_gset_add(container, POINTER_FROM_UINT(1)));
	EXPECT_EQ(true, LIB_gset_add(container, POINTER_FROM_UINT(3)));
	EXPECT_EQ(true, LIB_gset_add(container, POINTER_FROM_UINT(8)));
	EXPECT_EQ(true, LIB_gset_add(container, POINTER_FROM_UINT(4)));
	EXPECT_EQ(true, LIB_gset_add(container, POINTER_FROM_UINT(7)));
	EXPECT_EQ(true, LIB_gset_add(container, POINTER_FROM_UINT(2)));
	EXPECT_EQ(true, LIB_gset_add(container, POINTER_FROM_UINT(5)));
	EXPECT_EQ(true, LIB_gset_add(container, POINTER_FROM_UINT(6)));
	EXPECT_EQ(true, LIB_gset_add(container, POINTER_FROM_UINT(9)));

	EXPECT_EQ(9u, LIB_gset_len(container));

	EXPECT_EQ(false, LIB_gset_add(container, POINTER_FROM_UINT(1)));
	EXPECT_EQ(false, LIB_gset_add(container, POINTER_FROM_UINT(3)));
	EXPECT_EQ(false, LIB_gset_add(container, POINTER_FROM_UINT(8)));
	EXPECT_EQ(false, LIB_gset_add(container, POINTER_FROM_UINT(4)));
	EXPECT_EQ(false, LIB_gset_add(container, POINTER_FROM_UINT(7)));
	EXPECT_EQ(false, LIB_gset_add(container, POINTER_FROM_UINT(2)));
	EXPECT_EQ(false, LIB_gset_add(container, POINTER_FROM_UINT(5)));
	EXPECT_EQ(false, LIB_gset_add(container, POINTER_FROM_UINT(6)));
	EXPECT_EQ(false, LIB_gset_add(container, POINTER_FROM_UINT(9)));

	EXPECT_EQ(9u, LIB_gset_len(container));

	LIB_gset_free(container, NULL);
}

TEST(GSet, Deletions) {
	GSet *container = LIB_gset_ptr_new("rose::test::GSet");

	EXPECT_EQ(true, LIB_gset_add(container, POINTER_FROM_UINT(1)));
	EXPECT_EQ(true, LIB_gset_add(container, POINTER_FROM_UINT(3)));
	EXPECT_EQ(true, LIB_gset_add(container, POINTER_FROM_UINT(8)));
	EXPECT_EQ(true, LIB_gset_add(container, POINTER_FROM_UINT(4)));
	EXPECT_EQ(true, LIB_gset_add(container, POINTER_FROM_UINT(7)));
	EXPECT_EQ(true, LIB_gset_add(container, POINTER_FROM_UINT(2)));
	EXPECT_EQ(true, LIB_gset_add(container, POINTER_FROM_UINT(5)));
	EXPECT_EQ(true, LIB_gset_add(container, POINTER_FROM_UINT(6)));
	EXPECT_EQ(true, LIB_gset_add(container, POINTER_FROM_UINT(9)));

	EXPECT_EQ(9u, LIB_gset_len(container));

	EXPECT_EQ(true, LIB_gset_remove(container, POINTER_FROM_UINT(1), NULL));
	EXPECT_EQ(true, LIB_gset_remove(container, POINTER_FROM_UINT(3), NULL));
	EXPECT_EQ(true, LIB_gset_remove(container, POINTER_FROM_UINT(8), NULL));
	EXPECT_EQ(true, LIB_gset_remove(container, POINTER_FROM_UINT(4), NULL));
	EXPECT_EQ(true, LIB_gset_remove(container, POINTER_FROM_UINT(7), NULL));
	EXPECT_EQ(true, LIB_gset_remove(container, POINTER_FROM_UINT(2), NULL));
	EXPECT_EQ(true, LIB_gset_remove(container, POINTER_FROM_UINT(5), NULL));
	EXPECT_EQ(true, LIB_gset_remove(container, POINTER_FROM_UINT(6), NULL));
	EXPECT_EQ(true, LIB_gset_remove(container, POINTER_FROM_UINT(9), NULL));

	EXPECT_EQ(0u, LIB_gset_len(container));

	LIB_gset_free(container, NULL);
}

TEST(GSet, Master) {
	GSet *container = LIB_gset_ptr_new("rose::test::GSet");

	size_t length = 65535;

	std::set<int> reference;

	for (size_t i = 0; i < length; i++) {
		int val = rand() % 10;

		if ((i & 1) == 0) {
			EXPECT_EQ(reference.insert(val).second, LIB_gset_add(container, POINTER_FROM_UINT(val)));
		}
		else {
			if (reference.find(val) == reference.end()) {
				EXPECT_EQ(NULL, LIB_gset_lookup(container, POINTER_FROM_UINT(val)));
			}
			else {
				void *key = LIB_gset_lookup(container, POINTER_FROM_UINT(val));

				EXPECT_EQ(POINTER_FROM_UINT(val), key);
				EXPECT_EQ(true, LIB_gset_remove(container, POINTER_FROM_UINT(val), NULL));

				reference.erase(val);
			}
		}
	}

	LIB_gset_free(container, NULL);
}

/* \} */

}  // namespace
