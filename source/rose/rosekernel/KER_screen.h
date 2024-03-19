#pragma once

#include "DNA_defines.h"
#include "DNA_screen_types.h"
#include "DNA_space_types.h"

#include "LIB_listbase.h"
#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SpaceType {
	struct SpaceType *prev, *next;
	
	char name[MAX_NAME];
	int spaceid;
	
	/** Create the editor specific data for this #SpaceType. */
	SpaceLink *(*create)(const ScrArea *area);
	/** The deconstructor for the editor specific data, does not free the link itself. */
	void (*free)(SpaceLink *link);
	
	/** Init is to cope with screen changes and check handlers. */
	void (*init)(struct wmWindowManager *wm, ScrArea *area);
	/** Exit is called when the area is hidden or removed. */
	void (*exit)(struct wmWindowManager *wm, ScrArea *area);
	
	/** Called when the mouse moves out of the area. */
	void (*deactivate)(ScrArea *area);
	/** Refresh context related data. */
	void (*refresh)(const struct Context *C, ScrArea *area);
	
	/** After a spacedata copy, an init should result in the exact same situation. */
	SpaceLink *(*duplicate)(SpaceLink *link);
	
	/**
	 * We want to be able to reach everry referenced ID from any other ID, screen reference #SpaceType(s) and some 
	 * #SpaceType(s) reference other ID(s), for instance #Scene is an ID, a 3D rendering #SpaceType would reference that.
	 * 
	 * This is very similar to the #foreach_id function from #IDType.
	 */
	void (*foreach_id)(SpaceLink *space_link, struct LibraryForeachIDData *data);
	
	/** Each editor has some regions, the base functions for these regions are described using a #ARegionType. */
	ListBase regiontypes;
} SpaceType;

typedef struct ARegionType {
	struct ARegionType *prev, *next;
	
	int regionid;
	
	/** Add handlers, stuff you only do once or on area/region type/size changes */
	void (*init)(struct wmWindowManager *wm, ARegion *region);
	/** Exit is called when the region is hidden or removed */
	void (*exit)(struct wmWindowManager *wm, ARegion *region);
	
	/** Draw entirely, view changes should be handled here. */
	void (*draw)(const struct Context *C, ARegion *region);
	
	void (*free)(ARegion *);
	
	/** Split region, copy data optionally. */
	void *(*duplicate)(void *ptr);
	
	ListBase drawcalls;
	
	int minsizex;
	int minsizey;
	int prefsizex;
	int prefsizey;
} ARegionType;

/* -------------------------------------------------------------------- */
/** \name Space Types
 * \{ */

/** If #KER_spacetype_exists returns true this is guaranteed to return non-NULL. */
SpaceType *KER_spacetype_from_id(int spacetype);

/** #SpaceType(s) are registered in the editors module, when it is initialized. */
 
void KER_spacetype_register(SpaceType *type);
bool KER_spacetype_exists(int spacetype);
void KER_spacetypes_free();

const ListBase *KER_spacetypes_list();
 
/* \} */

/* -------------------------------------------------------------------- */
/** \name Region Types
 * \{ */

ARegionType *KER_regiontype_from_id(const SpaceType *space, int regionid);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Screen Areas
 * \{ */

void KER_screen_area_free(ScrArea *area);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Screen Regions
 * \{ */

void KER_area_region_free(SpaceType *spacetype, ARegion *region);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Screen AreaMap
 * \{ */

void KER_screen_area_map_free(ScrAreaMap *areamap);

ScrEdge *KER_screen_find_edge(const Screen *screen, ScrVert *v1, ScrVert *v2);
void KER_screen_sort_scrvert(ScrVert **v1, ScrVert **v2);
void KER_screen_remove_double_scrverts(Screen *screen);
void KER_screen_remove_double_scredges(Screen *screen);
void KER_screen_remove_unused_scredges(Screen *screen);
void KER_screen_remove_unused_scrverts(Screen *screen);

ScrArea *KER_screen_area_map_find_area_xy(const ScrAreaMap *areamap, int spacetype, const int xy[2]);

ARegion *KER_area_find_region_xy(const ScrArea *area, const int regiontype, const int xy[2]);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Screen Utils
 * \{ */

ScrArea *KER_screen_find_area_xy(const Screen *screen, int spacetype, const int xy[2]);

ARegion *KER_screen_find_region_xy(const Screen *screen, int spacetype, const int xy[2]);
ARegion *KER_screen_find_main_region_at_xy(const Screen *screen, int spacetype, const int xy[2]);

/**
 * Hey! It seems something is missing here...!
 * Oh yeah, right! How are we supposed to create a #Screen, you let us delete a #Screen but there is no function to create one.
 * 
 * Well... this is no mistake, this module is the KERNEL module, can you image how bad it would be to have UI layouts here?
 * Operations in this module have to remain strictly about handling basic operations for the data, not create or draw UI.
 * These operations are done through the #editors module whose purpose is exactly that, to create, draw and manage UI.
 */

void KER_screen_free_data(Screen *screen);

/* \} */

#ifdef __cplusplus
}
#endif
