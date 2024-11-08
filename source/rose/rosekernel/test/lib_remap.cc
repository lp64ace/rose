#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_windowmanager_types.h"

#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_lib_remap.h"
#include "KER_main.h"

#include "gtest/gtest.h"

namespace {

TEST(IDRemap, Unlink) {
	KER_idtype_init();

	Main *main = KER_main_new();
	do {
		WindowManager *wm = reinterpret_cast<WindowManager *>(KER_libblock_alloc(main, ID_WM, "WindowManager", 0));
		wmWindow *win;
		EXPECT_NE(wm, nullptr);
		LIB_addtail(&wm->windows, win = reinterpret_cast<wmWindow *>(MEM_mallocN(sizeof(wmWindow), "wmWindow")));
		do {
			win->screen = reinterpret_cast<Screen *>(KER_libblock_alloc(main, ID_SCR, "Screen", 0));
			EXPECT_NE(win->screen, nullptr);
			KER_libblock_unlink(main, win->screen);
		} while (false);
		EXPECT_EQ(win->screen, nullptr);
		LIB_freelistN(&wm->windows);
	} while (false);
	KER_main_free(main);
}

TEST(IDRemap, UnlinkMultiple) {
	KER_idtype_init();

	Main *main = KER_main_new();
	do {
		WindowManager *wm = reinterpret_cast<WindowManager *>(KER_libblock_alloc(main, ID_WM, "WindowManager", 0));
		wmWindow *win1, *win2;
		EXPECT_NE(wm, nullptr);
		LIB_addtail(&wm->windows, win1 = reinterpret_cast<wmWindow *>(MEM_mallocN(sizeof(wmWindow), "wmWindow")));
		LIB_addtail(&wm->windows, win2 = reinterpret_cast<wmWindow *>(MEM_mallocN(sizeof(wmWindow), "wmWindow")));
		do {
			Screen *scr = reinterpret_cast<Screen *>(KER_libblock_alloc(main, ID_SCR, "Screen", 0));
			EXPECT_NE(win1->screen = scr, nullptr);
			EXPECT_NE(win2->screen = scr, nullptr);
			/** Since #scr starts with a single user, we need to add one more since we reference it from two windows! */
			id_us_add(reinterpret_cast<ID *>(scr));
			KER_libblock_unlink(main, scr);
		} while (false);
		EXPECT_EQ(win1->screen, nullptr);
		EXPECT_EQ(win2->screen, nullptr);
		LIB_freelistN(&wm->windows);
	} while (false);
	KER_main_free(main);
}

}  // namespace
