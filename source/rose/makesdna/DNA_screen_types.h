#pragma once

#include "DNA_ID.h"
#include "DNA_listbase.h"
#include "DNA_vec_types.h"
#include "DNA_view2d_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AREAMINX 29
#define HEADER_PADDING_Y 6
#define WIDGET_UNIT 20
#define HEADERY 24
#define PIXELSIZE 1
#define UI_SCALE_FAC 1

/**
 * TODO: Doing this is quite ugly :)
 * Once the top-bar is merged Screen should be refactored to use ScrAreaMap.
 */
#define AREAMAP_FROM_SCREEN(screen) ((ScrAreaMap *)&(screen)->vertbase)

typedef struct Screen {
	ID id;

	/* NOTE: KEEP ORDER IN SYNC WITH #ScrAreaMap! (see AREAMAP_FROM_SCREEN macro above). */

	/** Screens have vertices/edges to define areas. */
	ListBase vertbase;
	ListBase edgebase;
	ListBase areabase;

	/* End variables that must be in sync with #ScrAreaMap. */

	/** Screen level regions (menus), runtime only. */
	ListBase regionbase;

	int winid;
	int flags;
} Screen;

DNA_ACTION_DEFINE(Screen, DNA_ACTION_STORE);

/** #Screen->flag */
enum {
	SCREEN_REFRESH = 1 << 0,
};

typedef struct ScrAreaMap {
	ListBase vertbase;
	ListBase edgebase;
	ListBase areabase;
} ScrAreaMap;

DNA_ACTION_DEFINE(ScrAreaMap, DNA_ACTION_RUNTIME);

typedef struct ScrVert {
	struct ScrVert *prev, *next, *newv;

	vec2s vec;

	int flag;
} ScrVert;

DNA_ACTION_DEFINE(ScrVert, DNA_ACTION_STORE);

typedef struct ScrEdge {
	struct ScrEdge *prev, *next;

	ScrVert *v1, *v2;

	int border;
	int flag;
} ScrEdge;

DNA_ACTION_DEFINE(ScrEdge, DNA_ACTION_STORE);

typedef struct ScrGlobalAreaData {
	/**
	 * Global areas have a staic (non-dynamic) size. That means, changing the wndow size doesn't affect their size at all.
	 * However, we can still 'collapse' them, by changing this value.
	 */
	int cur_fixed_height;

	/** For global areas, this is the min and max size they can use depending on if they are 'collapsed' or not. */
	int size_min;
	int size_max;

	int align;
	int flag;
} ScrGlobalAreaData;

DNA_ACTION_DEFINE(ScrGlobalAreaData, DNA_ACTION_RUNTIME);

/** #ScrGlobalAreaData->align */
enum {
	GLOBAL_AREA_ALIGN_TOP = 0,
	GLOBAL_AREA_ALIGN_BOTTOM = 1,
};

/** #ScrGlobalAreaData->flag */
enum {
	GLOBAL_AREA_IS_HIDDEN = 1 << 0,
};

typedef struct ScrArea {
	struct ScrArea *prev, *next;

	ScrVert *v1, *v2, *v3, *v4;

	/** Rect bound by v1 v2 v3 v4. */
	rcti totrct;

	int winx;
	int winy;

	/** Non-NULL if this area is a global area. */
	ScrGlobalAreaData *global;

	int spacetype;
	int flag;

	/** Callbacks for this space type. */
	struct SpaceType *type;

	ListBase spacedata;
	ListBase regionbase;
	ListBase handlers;
} ScrArea;

DNA_ACTION_DEFINE(ScrArea, DNA_ACTION_STORE);

/** #ScrArea->flag */
enum {
	/** Update size of regions within the area. */
	AREA_FLAG_REGION_SIZE_UPDATE = (1 << 0),
};

typedef struct ARegion_Runtime {
	int offset_x;
	int offset_y;
} ARegion_Runtime;

typedef struct ARegion {
	struct ARegion *prev, *next;

	/* 2D-View scrolling and zoom information about the region (most regions are 2d anyways). */
	View2D v2d;

	rcti winrct;
	rcti drwrct;

	int winx;
	int winy;

	int alignment;
	int flag;
	int regiontype;

	char pad[4];

	/**
	 * Current split size in unscaled pixels (if zero it uses regiontype).
	 * To convert to pixels use: `UI_SCALE_FAC * region->sizex + 0.5f`.
	 * However to get the current region size, you should usually use winx/winy from above, not this!
	 */
	int sizex;
	int sizey;

	/** Callbacks for this region type. */
	struct ARegionType *type;

	ListBase handlers;

	struct wmDrawBuffer *draw_buffer;

	ARegion_Runtime runtime;
} ARegion;

DNA_ACTION_DEFINE(ARegion, DNA_ACTION_STORE);

/** #ARegion->alignment */
enum {
	RGN_ALIGN_NONE = 0,
	RGN_ALIGN_TOP = 1,
	RGN_ALIGN_BOTTOM = 2,
	RGN_ALIGN_LEFT = 3,
	RGN_ALIGN_RIGHT = 4,
	RGN_ALIGN_HSPLIT = 5,
	RGN_ALIGN_VSPLIT = 6,
	RGN_ALIGN_FLOAT = 7,
	RGN_ALIGN_QSPLIT = 8,

	/**
	 * Region is split into the previous one, they share the same space along a common edge.
	 * Includes the #RGN_ALIGN_HIDE_WITH_PREV behavior.
	 */
	RGN_SPLIT_PREV = 1 << 5,
	/**
	 * Always let scaling this region scale the previous region instead.
	 * Useful to let regions appear like they are one (while having independent layout, scrolling, etc.).
	 */
	RGN_SPLIT_SCALE_PREV = 1 << 6,
	/**
	 * Whenever the previous region is hidden, this region becomes invisible too.
	 * The flag #RGN_FLAG_HIDDEN should only be set for the previous region, not this.
	 */
	RGN_SPLIT_HIDE_WITH_PREV = 1 << 7,
};

/** Mask out flags so we can check the alignment. */
#define RGN_ALIGN_ENUM_FROM_MASK(align) ((align) & ((1 << 4) - 1))
#define RGN_ALIGN_FLAG_FROM_MASK(align) ((align) & ~((1 << 4) - 1))

/** #ARegion->flag */
enum {
	RGN_FLAG_HIDDEN = (1 << 0),
	RGN_FLAG_TOO_SMALL = (1 << 1),
	/** Enable dynamically changing the region size in the #ARegionType::layout() callback. */
	RGN_FLAG_DYNAMIC_SIZE = (1 << 2),
	/** Size has been clamped (floating regions only). */
	RGN_FLAG_SIZE_CLAMP_X = (1 << 5),
	RGN_FLAG_SIZE_CLAMP_Y = (1 << 6),
};

/** #ARegion->type */
enum {
	RGN_TYPE_WINDOW = 0,
	RGN_TYPE_HEADER = 1,
	RGN_TYPE_TOOL_HEADER = 2,
	RGN_TYPE_FOOTER = 2,
};

#define RGN_TYPE_ANY -1

#ifdef __cplusplus
}
#endif
