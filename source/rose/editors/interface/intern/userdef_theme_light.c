#include "DNA_userdef_types.h"

#include "LIB_utildefines.h"

#define RGBA(c) {((c) >> 24) & 0xff, ((c) >> 16) & 0xff, ((c) >> 8) & 0xff, (c) & 0xff}
#define RGB(c) {((c) >> 16) & 0xff, ((c) >> 8) & 0xff, (c) & 0xff}

/* clang-format off */

const Theme U_theme_xp = {
	.name = "Light",
	.tui = {
		/** The colors that will be used for separator buttons. */
		.wcol_sepr = {
			.outline = RGBA(0x808080ff),       // medium gray
			.inner = RGBA(0xc0c0c0ff),         // light gray
			.inner_sel = RGBA(0xe0e0e0ff),     // lighter gray on select
			.text = RGBA(0x000000ff),          // black text
			.text_sel = RGBA(0xffffff00),      // transparent white text for highlight
			.roundness = 0.05f,
		},
		/** The colors that will be used for regular buttons. */
		.wcol_but = {
			.outline = RGBA(0x808080ff),
			.inner = RGBA(0xd4d0c8ff),         // classic button gray
			.inner_sel = RGBA(0x316ac5ff),     // XP blue selection
			.text = RGBA(0x000000ff),
			.text_sel = RGBA(0xffffffff),
			.roundness = 0.05f,
		},
		/** The colors that will be used for text buttons. */
		.wcol_txt = {
			.outline = RGBA(0x808080ff),
			.inner = RGBA(0xd4d0c800),
			.inner_sel = RGBA(0x316ac5ff),
			.text = RGBA(0x000000ff),
			.text_sel = RGBA(0xffffffff),
			.roundness = 0.05f,
		},
		/** The colors that will be used for text/number edit buttons. */
		.wcol_edit = {
			.outline = RGBA(0x808080ff),
			.inner = RGBA(0xffffffcc),         // near white
			.inner_sel = RGBA(0x316ac5cc),
			.text = RGBA(0x000000ff),
			.text_sel = RGBA(0xaa80ffcc),
			.roundness = 0.05f,
		},
		/** The colors that will be used for scrollbar buttons. */
		.wcol_scroll = {
			.outline = RGBA(0x808080ff),
			.inner = RGBA(0xffffff7f),
			.inner_sel = RGBA(0xffffff2f),
			.text = RGBA(0x000000cf),
			.text_sel = RGBA(0x000000ff),
			.roundness = 0.05f,
		},
		.text_cur = RGBA(0x000000ff),
	},
	/** The colors that will be used for the PopUp space. */
	.space_empty = {
		.back = RGBA(0xd4d0c8ff),
		.text = RGBA(0x000000ff),
		.text_hi = RGBA(0x316ac5ff),
		.header = RGBA(0xb0b0b0ff),
		.header_hi = RGBA(0xa0a0a0ff),
	},
	/** The colors that will be used for the UserPref space. */
	.space_file = {
		.back = RGBA(0xc0c0c0ff),
		.text = RGBA(0x000000ff),
		.text_hi = RGBA(0x316ac5ff),
		.header = RGBA(0xb0b0b0ff),
		.header_hi = RGBA(0x909090ff),
	},
	/** The colors that will be used for the View3D space. */
	.space_view3d = {
		.back = RGBA(0xc0c0c0ff),
		.text = RGBA(0x000000ff),
		.text_hi = RGBA(0x316ac5ff),
		.header = RGBA(0xb0b0b0ff),
		.header_hi = RGBA(0x909090ff),
	},
	/** The colors that will be used for the TopBar space. */
	.space_topbar = {
		.back = RGBA(0xd4d0c8ff),
		.text = RGBA(0x000000ff),
		.text_hi = RGBA(0x316ac5ff),
		.header = RGBA(0xb0b0b0ff),
		.header_hi = RGBA(0xa0a0a0ff),
	},
	/** The colors that will be used for the StatusBar space. */
	.space_statusbar = {
		.back = RGBA(0xd4d0c8ff),
		.text = RGBA(0x000000ff),
		.text_hi = RGBA(0x316ac5ff),
		.header = RGBA(0xb0b0b0ff),
		.header_hi = RGBA(0xa0a0a0ff),
	},
	/** The colors that will be used for the UserPref space. */
	.space_properties = {
		.back = RGBA(0xc0c0c0ff),
		.text = RGBA(0x000000ff),
		.text_hi = RGBA(0x316ac5ff),
		.header = RGBA(0xb0b0b0ff),
		.header_hi = RGBA(0x909090ff),
	},
};

/* clang-format on */
