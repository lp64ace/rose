#include "MEM_guardedalloc.h"

#include "KER_idtype.h"
#include "KER_lib_query.h"
#include "KER_screen.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name ID Type Implementation
 * \{ */

ROSE_STATIC void screen_free_data(ID *id) {
	Screen *screen = (Screen *)id;

	KER_screen_area_map_free(AREAMAP_FROM_SCREEN(screen));
	LIB_freelistN(&screen->regionbase);
}

ROSE_STATIC void screen_foreach_id(ID *id, struct LibraryForeachIDData *data) {
	Screen *screen = (Screen *)id;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Screen Geometry
 * \{ */

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
		SWAP(ScrVert *, *v1, *v2);
	}
}

void KER_screen_remove_double_scrverts(Screen *screen) {
	LISTBASE_FOREACH(ScrVert *, verg, &screen->vertbase) {
		if (verg->newv == NULL) {
			ScrVert *v1 = verg->next;
			while (v1) {
				if (v1->newv == NULL) {
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
			fprintf(stderr, "[Kernel] Area %d, edge 1 doesn't exist\n", a);
		}
		else {
			se->flag = 1;
		}
		se = KER_screen_find_edge(screen, area->v2, area->v3);
		if (se == NULL) {
			fprintf(stderr, "[Kernel] Area %d, edge 2 doesn't exist\n", a);
		}
		else {
			se->flag = 1;
		}
		se = KER_screen_find_edge(screen, area->v3, area->v4);
		if (se == NULL) {
			fprintf(stderr, "[Kernel] Area %d, edge 3 doesn't exist\n", a);
		}
		else {
			se->flag = 1;
		}
		se = KER_screen_find_edge(screen, area->v4, area->v1);
		if (se == NULL) {
			fprintf(stderr, "[Kernel] Area %d, edge 4 doesn't exist\n", a);
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

/** \} */

/* -------------------------------------------------------------------- */
/** \name SpaceType
 * \{ */

ROSE_STATIC ListBase space_types;

SpaceType *KER_spacetype_from_id(int spaceid) {
	return LIB_findbytes(&space_types, &spaceid, sizeof(int), offsetof(SpaceType, spaceid));
}
ARegionType *KER_regiontype_from_id(const SpaceType *type, int regionid) {
	return LIB_findbytes(&type->regiontypes, &regionid, sizeof(int), offsetof(ARegionType, regionid));
}

void KER_spacetype_register(SpaceType *st) {
	ROSE_assert(!KER_spacetype_exist(st->spaceid));
	ROSE_assert(st->spaceid != SPACE_EMPTY);
	LIB_addtail(&space_types, st);
}
bool KER_spacetype_exist(int spaceid) {
	return KER_spacetype_from_id(spaceid) != NULL;
}
void KER_spacetype_free(void) {
	LISTBASE_FOREACH(SpaceType *, st, &space_types) {
		LIB_freelistN(&st->regiontypes);
	}
	LIB_freelistN(&space_types);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Data Free
 * \{ */

void KER_screen_free_data(Screen *screen) {
	screen_free_data((ID *)screen);
}
void KER_spacedata_freelist(ListBase *lb) {
	LISTBASE_FOREACH(SpaceLink *, sl, lb) {
		SpaceType *st = KER_spacetype_from_id(sl->spacetype);

		LISTBASE_FOREACH(ARegion *, region, &sl->regionbase) {
			KER_area_region_free(st, region);
		}
		LIB_freelistN(&sl->regionbase);

		if (st && st->free) {
			st->free(sl);
		}
	}
	LIB_freelistN(lb);
}
void KER_area_region_free(SpaceType *st, ARegion *region) {
	if (st) {
		ARegionType *art = KER_regiontype_from_id(st, region->regiontype);

		if (art && art->free) {
			art->free(region);
		}
	}
	else if (region->type && region->type->free) {
		region->type->free(region);
	}
}
void KER_screen_area_free(ScrArea *area) {
	SpaceType *st = KER_spacetype_from_id(area->spacetype);

	LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
		KER_area_region_free(st, region);
	}
	LIB_freelistN(&area->regionbase);

	MEM_SAFE_FREE(area->global);

	KER_spacedata_freelist(&area->spacedata);
}
void KER_screen_area_map_free(ScrAreaMap *area_map) {
	LISTBASE_FOREACH_MUTABLE(ScrArea *, area, &area_map->areabase) {
		KER_screen_area_free(area);
	}
	LIB_freelistN(&area_map->vertbase);
	LIB_freelistN(&area_map->edgebase);
	LIB_freelistN(&area_map->areabase);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Screen Data-block Definition
 * \{ */

IDTypeInfo IDType_ID_SCR = {
	.idcode = ID_SCR,

	.filter = FILTER_ID_SCR,
	.depends = 0,
	.index = INDEX_ID_SCR,
	.size = sizeof(Screen),

	.name = "Screen",
	.name_plural = "Screens",

	.flag = IDTYPE_FLAGS_NO_COPY,

	.init_data = NULL,
	.copy_data = NULL,
	.free_data = screen_free_data,

	.foreach_id = screen_foreach_id,

	.write = NULL,
	.read_data = NULL,
};

/** \} */
