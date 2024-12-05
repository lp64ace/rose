#ifndef DNA_SCREEN_TYPES_H
#define DNA_SCREEN_TYPES_H

#include "DNA_ID.h"
#include "DNA_listbase.h"
#include "DNA_vector_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct GHash;
struct SpaceType;
struct ARegionType;

/**
 * TODO: Doing this is quite ugly :)
 * Once the top-bar is merged Screen should be refactored to use ScrAreaMap.
 */
#define AREAMAP_FROM_SCREEN(screen) ((ScrAreaMap *)&(screen)->vertbase)

typedef struct Screen {
	ID id;

	/**
	 * Screens have vertices/edges to define areas.
	 *
	 * TODO: Should become ScrAreaMap now.
	 * NOTE: KEEP ORDER IN SYNC WITH #ScrAreaMap! (see AREAMAP_FROM_SCREEN macro above).
	 */
	ListBase vertbase;
	ListBase edgebase;
	ListBase areabase;
	/* End variables that must be in sync with #ScrAreaMap. */

	/** Screen level regions (menus), runtime only. */
	ListBase regionbase;

	/** General flags. */
	int flag;
	/** Window-ID from WM, starts with 1. */
	int winid;
	/** User-setting for which editors get redrawn during animation playback. */
	int redraws_flag;

	/** Temp screen in a temp window, don't save (like user-preferences). */
	char temp;
	/** Notifier for drawing edges. */
	char do_draw;
	/** Notifier for scale screen, changed screen, etc. */
	char do_refresh;
	/** Set to delay screen handling after switching back from maximized area. */
	char skip_handling;

	/** Active region that has mouse focus. */
	struct ARegion *active_region;
} Screen;

typedef struct ScrVert {
	struct ScrVert *prev, *next, *newv;
	/**
	 * In general operations happen in pixel space but we keep everything in floating values for scaling.
	 */
	vec2f vec;
	int flag;
} ScrVert;

typedef struct ScrEdge {
	struct ScrEdge *prev, *next;
	struct ScrVert *v1, *v2;
	int flag;
} ScrEdge;

/** NOTE: KEEP ORDER IN SYNC WITH LISTBASES IN Screen! */
typedef struct ScrAreaMap {
	ListBase vertbase;
	ListBase edgebase;
	ListBase areabase;
} ScrAreaMap;

typedef struct ScrGlobalAreaData {
	int height;
	int size_min;
	int size_max;
	int alignment;
	int flag;
} ScrGlobalAreaData;

/** #ScrGlobalAreaData->alignment */
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

	/** Ordered, v1 := bottom-left, v2 := top-left, v3 := top-right, v4 := bottom-right. */
	struct ScrVert *v1, *v2, *v3, *v4;
	/** Rectangle bounded by the vertices, v1, v2, v3, v4. */
	rcti totrct;

	int sizex;
	int sizey;

	int spacetype;
	int flag;

	/** Callbacks for this space type. */
	struct SpaceType *type;
	struct ScrGlobalAreaData *global;

	/**
	 * A list of space links (editors) that were open in this area before.
	 * When changing the editor type, we try to reuse old editor data from this list.
	 * The first item is the active/visible one.
	 */
	ListBase spacedata;
	/**
	 * The region list is the one from the active/visible editor.
	 * See the #spacedata list above.
	 */
	ListBase regionbase;
	ListBase handlers;
} ScrArea;

/** #ScrArea->flag */
enum {
	AREA_FLAG_REGION_SIZE_UPDATE = 1 << 0,
};

typedef struct ARegionRuntime {
	struct GHash *block_name_map;
} ARegionRuntime;

typedef struct ARegion {
	struct ARegion *prev, *next;

	rcti winrct;
	rcti drwrct;

	int sizex;
	int sizey;

	int alignment;
	int regiontype;
	int flag;

	short visible;
	short overlap;

	/** Callbacks for this region type. */
	struct ARegionType *type;
	struct wmDrawBuffer *draw_buffer;

	ListBase uiblocks;
	ListBase uilists;
	ListBase handlers;

	ARegionRuntime runtime;
} ARegion;

/** #ARegion->alignment */
enum {
	RGN_ALIGN_NONE = 0,
	RGN_ALIGN_TOP,
	RGN_ALIGN_BOTTOM,
	RGN_ALIGN_LEFT,
	RGN_ALIGN_RIGHT,
	RGN_ALIGN_HSPLIT,
	RGN_ALIGN_VSPLIT,
	RGN_ALIGN_FLOAT,
	RGN_ALIGN_QSPLIT,

	/**
	 * Region is split into the previous one, they share the same space allong a common edge.
	 */
	RGN_SPLIT_PREV = 1 << 5,
	/**
	 * Always let scaling this region scale the previous region instead.
	 * Useful to let regions appear like they are one.
	 */
	RGN_SPLIT_SCALE_PREV = 1 << 6,
};

/** Mask out flags so we can check the alignment. */
#define RGN_ALIGN_ENUM_FROM_MASK(align) ((align) & ((1 << 4) - 1))
#define RGN_ALIGN_FLAG_FROM_MASK(align) ((align) & ~((1 << 4) - 1))

/** #ARegion->regiontype */
enum {
	RGN_TYPE_ANY = -1,
	RGN_TYPE_WINDOW,
	RGN_TYPE_HEADER,
	RGN_TYPE_FOOTER,
	/** Use for temporary regions, for example pop-ups! */
	RGN_TYPE_TEMPORARY,
};

/** #ARegion->flag */
enum {
	RGN_FLAG_HIDDEN = 1 << 0,
	RGN_FLAG_HIDDEN_BY_USER = 1 << 1,
	RGN_FLAG_TOO_SMALL = 1 << 2,
	RGN_FLAG_DYNAMIC_SIZE = 1 << 3,
	RGN_FLAG_ALWAYS_REBUILD = 1 << 4,
	RGN_FLAG_REDRAW = 1 << 5,
	RGN_FLAG_LAYOUT = 1 << 6,
	RGN_FLAG_TEMP_REGIONDATA = 1 << 7,
	RGN_FLAG_NO_USER_RESIZE = 1 << 8,
	RGN_FLAG_SIZE_CLAMP_X = 1 << 9,
	RGN_FLAG_SIZE_CLAMP_Y = 1 << 10,
};

#define AREAMINX 32
#define PIXELSIZE 1
#define WIDGET_UNIT 24

#define UI_UNIT_X (PIXELSIZE * WIDGET_UNIT)
#define UI_UNIT_Y (PIXELSIZE * WIDGET_UNIT)
#define UI_TEXT_MARGIN_X 4
#define UI_MENU_PADDING 0

#ifdef __cplusplus
}
#endif

#endif	// DNA_SCREEN_TYPES_H
