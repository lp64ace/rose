#include "DNA_userdef_types.h"

#include "LIB_utildefines.h"

#define RGBA(c) \
	{ ((c) >> 24) & 0xff, ((c) >> 16) & 0xff, ((c) >> 8) & 0xff, (c) & 0xff }
#define RGB(c) \
	{ ((c) >> 16) & 0xff, ((c) >> 8) & 0xff, (c) & 0xff }

/* clang-format off */

const Theme U_theme_default = {
	.name = "Default",
	.tui = {
		/** The colors that will be used for regural buttons. */
		.wcol_but = {
			.outline = RGBA(0x333333ff),
			.inner = RGBA(0x222222ff),
			.inner_sel = RGBA(0x444444ff),
			.text = RGBA(0xe0e0e0ff),
			.text_sel = RGBA(0x00ffccff),
			.roundness = 0,
		},
		/** The colors that will be used for text buttons. */
		.wcol_txt = {
			.outline = RGBA(0x00000000),
			.inner = RGBA(0x00000000),
			.inner_sel = RGBA(0x00000000),
			.text = RGBA(0xe0e0e0ff),
			.text_sel = RGBA(0x00ffccff),
            .roundness = 0,
		},
		/** The colors that will be used for text/number edit buttons. */
		.wcol_edit = {
			.outline = RGBA(0x333333ff),
			.inner = RGBA(0x222222ff),
			.inner_sel = RGBA(0x444444ff),
			.text = RGBA(0xe0e0e0ff),
			.text_sel = RGBA(0x2222e0ee),
            .roundness = 0,
		},
		.text_cur = RGBA(0xffffffff),
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
		.header = RGBA(0x2a2a2aff),
		.header_hi = RGBA(0x333333ff),
	},
};

/* clang-format on */