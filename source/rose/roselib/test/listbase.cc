#include "MEM_guardedalloc.h"

#include "LIB_listbase.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "gtest/gtest.h"

#define ASSERT_LISTBASE_VALID(listbase)                                     \
	{                                                                       \
		if ((listbase)->first) {                                            \
			Link *prev = NULL, *link = (Link *)(listbase)->first;           \
			do {                                                            \
				ASSERT_EQ(link->prev, prev);                                \
			} while ((void)(prev = link), (link = link->next));             \
		}                                                                   \
		if ((listbase)->last) {                                             \
			Link *next = NULL, *link = (Link *)(listbase)->last;            \
			do {                                                            \
				ASSERT_EQ(link->next, next);                                \
			} while ((void)(next = link), (link = link->prev));             \
		}                                                                   \
		ASSERT_TRUE((listbase)->first != NULL || (listbase)->last == NULL); \
	}                                                                       \
	(void)0

namespace {

TEST(ListBase, AddTail) {
	ListBase lb;

	Link *link1 = (Link *)MEM_callocN(sizeof(Link), "link1");
	Link *link2 = (Link *)MEM_callocN(sizeof(Link), "link2");

	LIB_listbase_clear(&lb);
	LIB_addtail(&lb, link1);
	LIB_addtail(&lb, link2);
	ASSERT_LISTBASE_VALID(&lb);

	LIB_freelistN(&lb);
}

TEST(ListBase, AddHead) {
	ListBase lb;

	Link *link1 = (Link *)MEM_callocN(sizeof(Link), "link1");
	Link *link2 = (Link *)MEM_callocN(sizeof(Link), "link2");

	LIB_listbase_clear(&lb);
	LIB_addhead(&lb, link1);
	LIB_addhead(&lb, link2);
	ASSERT_LISTBASE_VALID(&lb);

	LIB_freelistN(&lb);
}

TEST(ListBase, StringSearch) {
	typedef struct mLink {
		struct mLink *prev, *next;

		char name[64];
	} mLink;

	const char *const link1_name = "Link1";
	const char *const link2_name = "Link2";

	ListBase lb;

	mLink *link1 = (mLink *)MEM_callocN(sizeof(mLink), "link1");
	LIB_strcpy(link1->name, ARRAY_SIZE(link1->name), link1_name);
	mLink *link2 = (mLink *)MEM_callocN(sizeof(mLink), "link2");
	LIB_strcpy(link2->name, ARRAY_SIZE(link2->name), link2_name);

	LIB_listbase_clear(&lb);
	LIB_addhead(&lb, link1);
	{
		ASSERT_EQ(LIB_findstr(&lb, link1_name, offsetof(mLink, name)), link1);
		ASSERT_EQ(LIB_findstr(&lb, link2_name, offsetof(mLink, name)), nullptr);
	}
	LIB_addhead(&lb, link2);
	{
		ASSERT_EQ(LIB_findstr(&lb, link1_name, offsetof(mLink, name)), link1);
		ASSERT_EQ(LIB_findstr(&lb, link2_name, offsetof(mLink, name)), link2);
	}
	ASSERT_LISTBASE_VALID(&lb);

	LIB_freelistN(&lb);
}

TEST(ListBase, PointerSearch) {
	typedef struct mLink {
		struct mLink *prev, *next;

		const void *ptr;
	} mLink;

	const void *const link1_ptr = (const void *const)0x007f3ac8;
	const void *const link2_ptr = (const void *const)0x007f3ce3;

	ListBase lb;

	mLink *link1 = (mLink *)MEM_callocN(sizeof(mLink), "link1");
	link1->ptr = link1_ptr;
	mLink *link2 = (mLink *)MEM_callocN(sizeof(mLink), "link2");
	link2->ptr = link2_ptr;

	LIB_listbase_clear(&lb);
	LIB_addhead(&lb, link1);
	{
		ASSERT_EQ(LIB_findptr(&lb, link1_ptr, offsetof(mLink, ptr)), link1);
		ASSERT_EQ(LIB_findptr(&lb, link2_ptr, offsetof(mLink, ptr)), nullptr);
	}
	LIB_addhead(&lb, link2);
	{
		ASSERT_EQ(LIB_findptr(&lb, link1_ptr, offsetof(mLink, ptr)), link1);
		ASSERT_EQ(LIB_findptr(&lb, link2_ptr, offsetof(mLink, ptr)), link2);
	}
	ASSERT_LISTBASE_VALID(&lb);

	LIB_freelistN(&lb);
}

}  // namespace
