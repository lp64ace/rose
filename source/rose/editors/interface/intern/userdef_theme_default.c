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
			.outline = RGBA(0xff0000ff),
			.inner = RGBA(0x00ff00ff),
			.inner_sel = RGBA(0xffff00ff),
			.text = RGBA(0x00ccffff),
			.text_sel = RGBA(0x000000ff),
			.roundness = 0.2f,
		},
		/** The colors that will be used for text buttons. */
		.wcol_txt = {
			.outline = RGBA(0xff0000ff),
			.inner = RGBA(0x00ff00ff),
			.inner_sel = RGBA(0xffff00ff),
			.text = RGBA(0x00ccffff),
			.text_sel = RGBA(0x000000ff),
            .roundness = 0.2f,
		},
		/** The colors that will be used for text/number edit buttons. */
		.wcol_edit = {
			.outline = RGBA(0xff0000ff),
			.inner = RGBA(0x00ff00ff),
			.inner_sel = RGBA(0xffff00ff),
			.text = RGBA(0x00ccffff),
			.text_sel = RGBA(0x000000ff),
            .roundness = 0.2f,
		},
		.text_cur = RGBA(0xff00ffff),
	},
	/** The colors that will be used for the View3D space. */
	.space_view3d = {
		.back = RGBA(0x0000ffff),
		.text = RGBA(0xffffffff),
		.text_hi = RGBA(0xffffffff),
		.header = RGBA(0xccccccff),
		.header_hi = RGBA(0xddddddff),
	},
	/** The colors that will be used for the TopBar space. */
	.space_topbar = {
		.back = RGBA(0xffffffff),
		.text = RGBA(0xffffffff),
		.text_hi = RGBA(0xffffffff),
		.header = RGBA(0xff0000ff),
		.header_hi = RGBA(0xddddddff),
	},
	/** The colors that will be used for the StatusBar space. */
	.space_statusbar = {
		.back = RGBA(0xffffffff),
		.text = RGBA(0xffffffff),
		.text_hi = RGBA(0xffffffff),
		.header = RGBA(0xff0000ff),
		.header_hi = RGBA(0xddddddff),
	},
};

/* clang-format on */
