#include <math.h>
#include <stdio.h>
#include <string.h>

#include "MEM_alloc.h"

#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_screen.h"

#include "LIB_listbase.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

/* -------------------------------------------------------------------- */
/** \name ID Type Implementation
 * \{ */

static void screen_free_data(ID *id) {
	Screen *screen = (Screen *)id;

	LISTBASE_FOREACH(ARegion *, region, &screen->regionbase) {
		KER_area_region_free(NULL, region);
	}

	LIB_freelistN(&screen->regionbase);

	KER_screen_area_map_free(AREAMAP_FROM_SCREEN(screen));
}

void KER_screen_foreach_id_screen_area(LibraryForeachIDData *data, ScrArea *area) {
	LISTBASE_FOREACH(SpaceLink *, link, &area->spacedata) {
		SpaceType *spacetype = KER_spacetype_from_id(link->spacetype);

		if (spacetype && spacetype->foreach_id) {
			spacetype->foreach_id(link, data);
		}
	}
}

static void screen_foreach_id(ID *id, LibraryForeachIDData *data) {
	Screen *screen = (Screen *)id;
	const int flag = KER_lib_query_foreachid_process_flags_get(data);

	if (flag & IDWALK_INCLUDE_UI) {
		LISTBASE_FOREACH(ScrArea *, area, &screen->areabase) {
			KER_LIB_FOREACHID_PROCESS_FUNCTION_CALL(data, KER_screen_foreach_id_screen_area(data, area));
		}
	}
}

IDTypeInfo IDType_ID_SCR = {
	.id_code = ID_SCR,
	.id_filter = FILTER_ID_SCR,
	.main_listbase_index = INDEX_ID_SCR,
	.struct_size = sizeof(Screen),

	.name = "Screen",
	.name_plural = "Screens",

	.init_data = NULL,
	.copy_data = NULL,
	.free_data = screen_free_data,

	.foreach_id = screen_foreach_id,
};

/* \} */

/* -------------------------------------------------------------------- */
/** \name Space handling
 * \{ */

void KER_spacedata_freelist(ListBase *lb) {
	LISTBASE_FOREACH(SpaceLink *, link, lb) {
		SpaceType *st = KER_spacetype_from_id(link->spacetype);

		LISTBASE_FOREACH(ARegion *, region, &link->regionbase) {
			KER_area_region_free(st, region);
		}

		LIB_freelistN(&link->regionbase);

		if (st && st->free) {
			st->free(link);
		}
	}

	LIB_freelistN(lb);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Space Types
 * \{ */

/** Keep global; this has to be accessible outside of window-manager. */
static ListBase spacetypes = {NULL, NULL};

static void spacetype_free(SpaceType *type) {
	LISTBASE_FOREACH(ARegionType *, regiontype, &type->regiontypes) {
		LIB_freelistN(&regiontype->drawcalls);
	}

	LIB_freelistN(&type->regiontypes);
}

SpaceType *KER_spacetype_from_id(int spacetype) {
	LISTBASE_FOREACH(SpaceType *, st, &spacetypes) {
		if (st->spaceid == spacetype) {
			return st;
		}
	}
	return NULL;
}

void KER_spacetype_register(SpaceType *type) {
	SpaceType *stype = KER_spacetype_from_id(type->spaceid);
	if (stype) {
		printf("error: redefinition of spacetype %s\n", stype->name);
		spacetype_free(stype);
		MEM_freeN(stype);
	}

	LIB_addtail(&spacetypes, type);
}

bool KER_spacetype_exists(int spacetype) {
	return KER_spacetype_from_id(spacetype) != NULL;
}

void KER_spacetypes_free() {
	LISTBASE_FOREACH(SpaceType *, st, &spacetypes) {
		spacetype_free(st);
	}

	LIB_freelistN(&spacetypes);
}

const ListBase *KER_spacetypes_list() {
	return &spacetypes;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Region Types
 * \{ */

ARegionType *KER_regiontype_from_id(const SpaceType *spacetype, int regionid) {
	LISTBASE_FOREACH(ARegionType *, art, &spacetype->regiontypes) {
		if (art->regionid == regionid) {
			return art;
		}
	}
	return NULL;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Screen Areas
 * \{ */

void KER_screen_area_free(ScrArea *area) {
	SpaceType *st = KER_spacetype_from_id(area->spacetype);

	LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
		KER_area_region_free(st, region);
	}

	LIB_freelistN(&area->regionbase);

	KER_spacedata_freelist(&area->spacedata);

	if (area->global) {
		MEM_freeN(area->global);
	}
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Screen Regions
 * \{ */

void KER_area_region_free(SpaceType *spacetype, ARegion *region) {
	if (spacetype) {
		ARegionType *art = KER_regiontype_from_id(spacetype, region->regiontype);

		if (art && art->free) {
			art->free(region);
		}
	}
	else if (region->type && region->type->free) {
		region->type->free(region);
	}
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Screen AreaMap
 * \{ */

void KER_screen_area_map_free(ScrAreaMap *areamap) {
	LISTBASE_FOREACH_MUTABLE(ScrArea *, area, &areamap->areabase) {
		KER_screen_area_free(area);
	}

	LIB_freelistN(&areamap->vertbase);
	LIB_freelistN(&areamap->edgebase);
	LIB_freelistN(&areamap->areabase);
}

ScrEdge *KER_screen_find_edge(const Screen *screen, ScrVert *v1, ScrVert *v2) {
	KER_screen_sort_scrvert(&v1, &v2);
	LISTBASE_FOREACH(ScrEdge *, se, &screen->edgebase) {
		if (se->v1 == v1 && se->v2 == v2) {
			return se;
		}
	}

	return NULL;
}

void KER_screen_sort_scrvert(ScrVert **v1, ScrVert **v2) {
	if (*v1 > *v2) {
		ScrVert *tmp = *v1;
		*v1 = *v2;
		*v2 = tmp;
	}
}

void KER_screen_remove_double_scrverts(Screen *screen) {
	LISTBASE_FOREACH(ScrVert *, verg, &screen->vertbase) {
		if (verg->newv == NULL) { /* !!! */
			ScrVert *v1 = verg->next;
			while (v1) {
				if (v1->newv == NULL) { /* !?! */
					if (v1->vec.x == verg->vec.x && v1->vec.y == verg->vec.y) {
						v1->newv = verg;
					}
				}
				v1 = v1->next;
			}
		}
	}

	/* replace pointers in edges and faces */
	LISTBASE_FOREACH(ScrEdge *, se, &screen->edgebase) {
		if (se->v1->newv) {
			se->v1 = se->v1->newv;
		}
		if (se->v2->newv) {
			se->v2 = se->v2->newv;
		}
		/* edges changed: so.... */
		KER_screen_sort_scrvert(&(se->v1), &(se->v2));
	}
	LISTBASE_FOREACH(ScrArea *, area, &screen->areabase) {
		if (area->v1->newv) {
			area->v1 = area->v1->newv;
		}
		if (area->v2->newv) {
			area->v2 = area->v2->newv;
		}
		if (area->v3->newv) {
			area->v3 = area->v3->newv;
		}
		if (area->v4->newv) {
			area->v4 = area->v4->newv;
		}
	}

	/* remove */
	LISTBASE_FOREACH_MUTABLE(ScrVert *, verg, &screen->vertbase) {
		if (verg->newv) {
			LIB_remlink(&screen->vertbase, verg);
			MEM_freeN(verg);
		}
	}
}

void KER_screen_remove_double_scredges(Screen *screen) {
	/* compare */
	LISTBASE_FOREACH(ScrEdge *, verg, &screen->edgebase) {
		ScrEdge *se = verg->next;
		while (se) {
			ScrEdge *sn = se->next;
			if (verg->v1 == se->v1 && verg->v2 == se->v2) {
				LIB_remlink(&screen->edgebase, se);
				MEM_freeN(se);
			}
			se = sn;
		}
	}
}

void KER_screen_remove_unused_scredges(Screen *screen) {
	/* sets flags when edge is used in area */
	int a = 0;
	LISTBASE_FOREACH_INDEX(ScrArea *, area, &screen->areabase, a) {
		ScrEdge *se = KER_screen_find_edge(screen, area->v1, area->v2);
		if (se == NULL) {
			printf("error: area %d edge 1 doesn't exist\n", a);
		}
		else {
			se->flag = 1;
		}
		se = KER_screen_find_edge(screen, area->v2, area->v3);
		if (se == NULL) {
			printf("error: area %d edge 2 doesn't exist\n", a);
		}
		else {
			se->flag = 1;
		}
		se = KER_screen_find_edge(screen, area->v3, area->v4);
		if (se == NULL) {
			printf("error: area %d edge 3 doesn't exist\n", a);
		}
		else {
			se->flag = 1;
		}
		se = KER_screen_find_edge(screen, area->v4, area->v1);
		if (se == NULL) {
			printf("error: area %d edge 4 doesn't exist\n", a);
		}
		else {
			se->flag = 1;
		}
	}

	LISTBASE_FOREACH_MUTABLE(ScrEdge *, se, &screen->edgebase) {
		if (se->flag == 0) {
			LIB_remlink(&screen->edgebase, se);
			MEM_freeN(se);
		}
		else {
			se->flag = 0;
		}
	}
}

void KER_screen_remove_unused_scrverts(Screen *screen) {
	/* we assume edges are ok */
	LISTBASE_FOREACH(ScrEdge *, se, &screen->edgebase) {
		se->v1->flag = 1;
		se->v2->flag = 1;
	}

	LISTBASE_FOREACH_MUTABLE(ScrVert *, sv, &screen->vertbase) {
		if (sv->flag == 0) {
			LIB_remlink(&screen->vertbase, sv);
			MEM_freeN(sv);
		}
		else {
			sv->flag = 0;
		}
	}
}

ScrArea *KER_screen_area_map_find_area_xy(const ScrAreaMap *areamap, int spacetype, const int xy[2]) {
	LISTBASE_FOREACH(ScrArea *, area, &areamap->areabase) {
		/* Test area's outer screen verts, not inner `area->totrct`. */
		if (xy[0] >= area->v1->vec.x && xy[0] <= area->v4->vec.x && xy[1] >= area->v1->vec.y && xy[1] <= area->v2->vec.y) {
			if (ELEM(spacetype, SPACE_TYPE_ANY, area->spacetype)) {
				return area;
			}
			break;
		}
	}
	return NULL;
}

ARegion *KER_area_find_region_xy(const ScrArea *area, const int regiontype, const int xy[2]) {
	if (area == NULL) {
		return NULL;
	}

	LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
		if (ELEM(regiontype, RGN_TYPE_ANY, region->regiontype)) {
			if (LIB_rcti_isect_pt_v(&region->winrct, xy)) {
				return region;
			}
		}
	}
	return NULL;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Screen Utils
 * \{ */

ScrArea *KER_screen_find_area_xy(const Screen *screen, int spacetype, const int xy[2]) {
	return KER_screen_area_map_find_area_xy(AREAMAP_FROM_SCREEN(screen), spacetype, xy);
}

ARegion *KER_screen_find_region_xy(const Screen *screen, int regiontype, const int xy[2]) {
	LISTBASE_FOREACH(ARegion *, region, &screen->regionbase) {
		if (ELEM(regiontype, RGN_TYPE_ANY, region->regiontype)) {
			if (LIB_rcti_isect_pt_v(&region->winrct, xy)) {
				return region;
			}
		}
	}
	return NULL;
}

ARegion *KER_screen_find_main_region_at_xy(const Screen *screen, int spacetype, const int xy[2]) {
	ScrArea *area = KER_screen_find_area_xy(screen, spacetype, xy);
	if (!area) {
		return NULL;
	}
	return KER_area_find_region_xy(area, RGN_TYPE_WINDOW, xy);
}

void KER_screen_free_data(Screen *screen) {
	screen_free_data(&screen->id);
}

/* \} */
