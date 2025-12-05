#include "MEM_guardedalloc.h"

#include "KER_lib_id.h"
#include "KER_main.h"

#include "DNA_ID.h"
#include "DNA_ID_enums.h"

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

Main *KER_main_new(void) {
	Main *main = MEM_callocN(sizeof(Main), "Main");
	KER_main_init(main);
	return main;
}

void KER_main_init(Main *main) {
	main->lock = MEM_mallocN(sizeof(SpinLock), "Main::SpinLock");
	LIB_spin_init(main->lock);
	main->is_global_main = false;
}

void KER_main_clear(Main *main) {
	/* Also call when reading a file, erase all, etc */
	ListBase *lbarray[INDEX_ID_MAX];
	int a;

	const int free_flag = LIB_ID_FREE_NO_MAIN | LIB_ID_FREE_NO_USER_REFCOUNT;

	a = set_listbasepointers(main, lbarray);
	while (a--) {
		ListBase *lb = lbarray[a];
		ID *id, *id_next;

		for (id = (ID *)(lb->first); id != NULL; id = id_next) {
			id_next = (ID *)(id->next);

			KER_id_free_ex(main, id, free_flag, false);
		}
		LIB_listbase_clear(lb);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Methods
 * \{ */

void KER_main_destroy(Main *main) {
	KER_main_clear(main);

	LIB_spin_end(main->lock);
	MEM_freeN((void *)main->lock);
	main->lock = NULL;
}

void KER_main_free(Main *main) {
	if (main->next) {
		KER_main_free(main->next);
	}

	KER_main_destroy(main);
	MEM_freeN(main);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Lookup Methods
 * \{ */

ID *KER_main_id_lookup(Main *main, short type, const char *name) {
	ListBase *lb = which_libbase(main, type);

	LISTBASE_FOREACH(ID *, id, lb) {
		if (STREQ(id->name + 2, name)) {
			return id;
		}
	}

	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool KER_main_is_empty(struct Main *main) {
	ListBase *lbarray[INDEX_ID_MAX];
	int a;

	a = set_listbasepointers(main, lbarray);
	while (a--) {
		if (!LIB_listbase_is_empty(lbarray[a])) {
			return false;
		}
	}

	return true;
}

int set_listbasepointers(Main *main, ListBase *lb[]) {
	/* Libraries may be accessed from pretty much any other ID. */
	lb[INDEX_ID_LI] = &(main->libraries);
	lb[INDEX_ID_AC] = &(main->actions);
	lb[INDEX_ID_AR] = &(main->armatures);
	lb[INDEX_ID_CA] = &(main->cameras);
	lb[INDEX_ID_ME] = &(main->meshes);
	lb[INDEX_ID_OB] = &(main->objects);
	lb[INDEX_ID_GR] = &(main->collections);
	lb[INDEX_ID_SCE] = &(main->scenes);
	lb[INDEX_ID_SCR] = &(main->screens);
	lb[INDEX_ID_WM] = &(main->wm);

	lb[INDEX_ID_NULL] = NULL;

	return (INDEX_ID_MAX - 1);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Data-Block Access Methods
 * \{ */

ListBase *which_libbase(Main *main, short type) {
	switch (type) {
		case ID_LI:
			return &(main->libraries);
		case ID_AC:
			return &(main->actions);
		case ID_AR:
			return &(main->armatures);
		case ID_CA:
			return &(main->cameras);
		case ID_ME:
			return &(main->meshes);
		case ID_OB:
			return &(main->objects);
		case ID_GR:
			return &(main->collections);
		case ID_SCE:
			return &(main->scenes);
		case ID_SCR:
			return &(main->screens);
		case ID_WM:
			return &(main->wm);
	}
	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Lock/Unlock Methods
 * \{ */

void KER_main_lock(Main *main) {
	LIB_spin_lock(main->lock);
}
void KER_main_unlock(Main *main) {
	LIB_spin_unlock(main->lock);
}

/** \} */
