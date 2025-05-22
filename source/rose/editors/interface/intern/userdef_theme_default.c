#include "DNA_userdef_types.h"

#include "LIB_utildefines.h"

#define RGBA(c) {((c) >> 24) & 0xff, ((c) >> 16) & 0xff, ((c) >> 8) & 0xff, (c) & 0xff}
#define RGB(c) {((c) >> 16) & 0xff, ((c) >> 8) & 0xff, (c) & 0xff}

/* clang-format off */

const Theme U_theme_default = {
	.name = "Default",
	.tui = {
		/** The colors that will be used for separator buttons. */
		.wcol_sepr = {
			.text = RGBA(0x777777aa),
			.roundness = 0.05f,
		},
		/** The colors that will be used for regural buttons. */
		.wcol_but = {
			.outline = RGBA(0x222222ff),
			.inner = RGBA(0x222222ff),
			.inner_sel = RGBA(0x444444ff),
			.text = RGBA(0x80b0faff),
			.roundness = 0.05f,
		},
		/** The colors that will be used for text buttons. */
		.wcol_txt = {
			.outline = RGBA(0x00000000),
			.inner = RGBA(0x00000000),
			.inner_sel = RGBA(0xe0e0e055),
			.text = RGBA(0xfaa070ee),
			.roundness = 0.05f,
		},
		/** The colors that will be used for text/number edit buttons. */
		.wcol_edit = {
			.outline = RGB(0x222222ff),
			.inner = RGBA(0x0a0a0aff),
			.inner_sel = RGBA(0x3a3a3a3f),
			.text = RGBA(0xd0fab0ff),
			.text_sel = RGBA(0xfceeee3f),
			.roundness = 0.05f,
		},
		/** The colors that will be used for scrollbar buttons. */
		.wcol_scroll = {
			.outline = RGB(0x222222ff),
			.inner = RGBA(0x1f1f1fff),
			.inner_sel = RGBA(0x2f2f2fff),
			.text = RGBA(0x3f3f3fee),
			.text_sel = RGBA(0x7f7f7fee),
			.roundness = 0.05f,
		},
		.text_cur = RGBA(0xff0000ff),
	},
	/** The colors that will be used for the PopUp space. */
	.space_empty = {
		.back = RGBA(0x2a2a2aff),
		.text = RGBA(0xe0e0e0ff),
		.text_hi = RGBA(0xffffffff),
		.header = RGBA(0x1a1a1aff),
		.header_hi = RGBA(0x2a2a2aff),
	},
	/** The colors that will be used for the View3D space. */
	.space_view3d = {
		.back = RGBA(0x101010ff),
		.text = RGBA(0xe0e0e0ff),
		.text_hi = RGBA(0xffffffff),
		.header = RGBA(0x1a1a1aff),
		.header_hi = RGBA(0x2a2a2aff),
	},
	/** The colors that will be used for the TopBar space. */
	.space_topbar = {
		.back = RGBA(0x1a1a1aff),
		.text = RGBA(0xe0e0e0ff),
		.text_hi = RGBA(0x00ffccff),
		.header = RGBA(0x2a2a2aff),
		.header_hi = RGBA(0x333333ff),
	},
	/** The colors that will be used for the StatusBar space. */
	.space_statusbar = {
		.back = RGBA(0x1a1a1aff),
		.text = RGBA(0xe0e0e0ff),
		.text_hi = RGBA(0x00ffccff),
		.header = RGBA(0x1a1a1aff),
		.header_hi = RGBA(0x333333ff),
	},
	/** The colors that will be used for the UserPref space. */
	.space_userpref = {
		.back = RGBA(0x101010ff),
		.text = RGBA(0xe0e0e0ff),
		.text_hi = RGBA(0xffffffff),
		.header = RGBA(0x1a1a1aff),
		.header_hi = RGBA(0x2a2a2aff),
	},
	/** The colors that will be used for the File space. */
	.space_file = {
		.back = RGBA(0x101010ff),
		.text = RGBA(0xe0e0e0ff),
		.text_hi = RGBA(0xffffffff),
		.header = RGBA(0x1a1a1aff),
		.header_hi = RGBA(0x2a2a2aff),
	},
};

/* clang-format on */
