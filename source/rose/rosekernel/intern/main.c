#include "MEM_alloc.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_spinlock.h"

#include "KER_lib_id.h"
#include "KER_main.h"

struct Main *KER_main_new(void) {
	struct Main *main = MEM_callocN(sizeof(Main), "Main");

	main->lock = MEM_mallocN(sizeof(SpinLock), "MainLock");
	LIB_spin_init((SpinLock *)main->lock);

	return main;
}

void KER_main_free(struct Main *main) {
	ListBase *lbarray[INDEX_ID_MAX];
	int a;

	/** Since we are removing whole main, no need to bother 'properly' (and slowly) removing each ID from it. */
	const int free_flag = (LIB_ID_FREE_NO_MAIN | LIB_ID_FREE_NO_USER_REFCOUNT);

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

	MEM_freeN(main->lock);
	MEM_freeN(main);
}

void KER_main_lock(struct Main *main) {
	LIB_spin_lock((SpinLock *)main->lock);
}

void KER_main_unlock(struct Main *main) {
	LIB_spin_unlock((SpinLock *)main->lock);
}

struct ListBase *which_libbase(struct Main *main, short type) {
	switch (type) {
		case ID_LI:
			return &(main->libraries);
		case ID_WM:
			return &(main->wm);
	}
	return NULL;
}

int set_listbasepointers(struct Main *main, struct ListBase *lb[]) {
	lb[INDEX_ID_LI] = &(main->libraries);
	lb[INDEX_ID_WM] = &(main->wm);

	lb[INDEX_ID_NULL] = NULL;

	return (INDEX_ID_MAX - 1);
}
