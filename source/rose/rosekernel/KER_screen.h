#ifndef KER_SCREEN_H
#define KER_SCREEN_H

#include "DNA_screen_types.h"
#include "DNA_space_types.h"

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ARegion;
struct ScrAreaMap;
struct Screen;
struct rContext;
struct WindowManager;
struct wmKeyConfig;

/* -------------------------------------------------------------------- */
/** \name SpaceType
 * \{ */

struct SpaceLink;

typedef struct SpaceType {
	struct SpaceType *prev, *next;

	char name[64];

	int spaceid;

	/** Initial allocation, after this WM will call init() too. */
	struct SpaceLink *(*create)(const struct ScrArea *area);
	/** Does not free the #SpaceLink itself. */
	void (*free)(struct SpaceLink *link);

	void (*init)(struct WindowManager *wm, struct ScrArea *area);
	void (*exit)(struct WindowManager *wm, struct ScrArea *area);
	/** Called when the mouse exits the area. */
	void (*deactivate)(struct ScrArea *area);

	/** Register operator types on startup. */
	void (*operatortypes)();

	/** Add default items to WM keymap. */
	void (*keymap)(struct wmKeyConfig *keyconf);

	/* region type definitions */
	ListBase regiontypes;
} SpaceType;

struct SpaceType *KER_spacetype_from_id(int spaceid);

/** \} */

/* -------------------------------------------------------------------- */
/** \name ARegionType
 * \{ */

typedef struct ARegionType {
	struct ARegionType *prev, *next;

	int regionid;

	void (*init)(struct WindowManager *wm, struct ARegion *region);
	void (*exit)(struct WindowManager *wm, struct ARegion *region);
	/** Does not free the #ARegion itself. */
	void (*free)(struct ARegion *region);

	void (*layout)(struct rContext *C, struct ARegion *region);
	void (*draw)(struct rContext *C, struct ARegion *region);

	/** Add default items to WM keymap. */
	void (*keymap)(struct wmKeyConfig *keyconf);

	int minsizex;
	int minsizey;
	int prefsizex;
	int prefsizey;

	ListBase paneltypes;
} ARegionType;

struct ARegionType *KER_regiontype_from_id(const struct SpaceType *type, int regionid);

typedef struct PanelType {
	struct PanelType *prev, *next;

	/** Unique name. */
	char idname[64];
	/** For panel header. */
	char label[64];
	/** For panel tooltip. */
	const char *description;

	int spacetype;
	int regiontype;

	int flag;

	/** Verify if the panel should draw or not. */
	bool (*poll)(const struct rContext *C, struct PanelType *pt);
	/** Draw header (optional) */
	void (*draw_header)(const struct rContext *C, struct Panel *panel);
	/** Draw entirely, view changes should be handled here. */
	void (*draw)(const struct rContext *C, Panel *panel);

	/** Sub panels. */
	struct PanelType *parent;
	ListBase children;
} PanelType;

/** #PanelType->flag */
enum {
	PANEL_TYPE_NO_HEADER = 1 << 0,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Screen Geometry
 * \{ */

struct ScrVert;
struct ScrEdge;

struct ScrEdge *KER_screen_find_edge(const struct Screen *screen, struct ScrVert *v1, struct ScrVert *v2);
void KER_screen_sort_scrvert(struct ScrVert **v1, struct ScrVert **v2);
void KER_screen_remove_double_scrverts(struct Screen *screen);
void KER_screen_remove_double_scredges(struct Screen *screen);
void KER_screen_remove_unused_scredges(struct Screen *screen);
void KER_screen_remove_unused_scrverts(struct Screen *screen);

/** \} */

/* -------------------------------------------------------------------- */
/** \name SpaceType
 * \{ */

struct ListBase *KER_spacetype_list(void);

void KER_spacetype_register(struct SpaceType *st);
bool KER_spacetype_exist(int spaceid);
void KER_spacetype_free(void);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Panel
 * \{ */

/**
 * Create and free panels.
 */
struct Panel *KER_panel_new(PanelType *panel_type);
void KER_panel_free(Panel *panel);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Data Free
 * \{ */

/**
 * Free (or release) any data used by this screen (does not free the screen itself).
 */
void KER_screen_free_data(struct Screen *screen);
void KER_area_region_free(struct SpaceType *st, struct ARegion *region);
void KER_screen_area_free(struct ScrArea *area);
void KER_screen_area_map_free(struct ScrAreaMap *area_map);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_SCREEN_H
