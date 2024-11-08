#include "MEM_guardedalloc.h"

#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_main.h"

#include "gtest/gtest.h"

namespace {

TEST(IDFree, Simple) {
	KER_idtype_init();

	Main *main = KER_main_new();
	do {
		ID *id = reinterpret_cast<ID *>(KER_libblock_alloc(main, ID_LINK_PLACEHOLDER, "ID", 0));
		EXPECT_NE(id, nullptr);
		KER_id_free_ex(main, id, 0, true);
	} while (false);
	KER_main_free(main);
}

}  // namespace
