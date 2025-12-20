#include "MEM_guardedalloc.h"

#include "LIB_string.h"

#include "KER_context.h"
#include "KER_idprop.h"

#include "RNA_access.h"
#include "RNA_define.h"
#include "RNA_prototypes.h"

#include "WM_api.h"

/* -------------------------------------------------------------------- */
/** \name Event Handling
 *
 * Handlers have pointers to the keymap in the default configuration.
 * During event handling this function is called to get the keymap from the final configuration.
 * \{ */

wmKeyMap *WM_keymap_list_find(ListBase *lb, const char *idname, int spaceid, int regionid) {
	LISTBASE_FOREACH(wmKeyMap *, km, lb) {
		if (km->spaceid == spaceid && km->regionid == regionid) {
			if (STREQLEN(idname, km->idname, ARRAY_SIZE(km->idname))) {
				return km;
			}
		}
	}

	return NULL;
}

ROSE_INLINE wmKeyMap *wm_keymap_new(const char *idname, int spaceid, int regionid) {
	wmKeyMap *km = MEM_callocN(sizeof(wmKeyMap), "wmKeyMap");

	LIB_strcpy(km->idname, ARRAY_SIZE(km->idname), idname);
	km->spaceid = spaceid;
	km->regionid = regionid;

	return km;
}

wmKeyMap *WM_keymap_ensure(wmKeyConfig *keyconf, const char *idname, int spaceid, int regionid) {
	wmKeyMap *km = WM_keymap_list_find(&keyconf->keymaps, idname, spaceid, regionid);

	if (km == NULL) {
		km = wm_keymap_new(idname, spaceid, regionid);
		LIB_addtail(&keyconf->keymaps, km);
	}

	return km;
}

wmKeyMap *WM_keymap_active(WindowManager *wm, wmKeyMap *keymap) {
	if (!keymap) {
		return NULL;
	}

	/* First user defined keymaps. */
	wmKeyMap *km = WM_keymap_list_find(&wm->runtime.keymaps, keymap->idname, keymap->spaceid, keymap->regionid);

	if (km) {
		return km;
	}

	return keymap;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name KeyMap
 * \{ */

ROSE_INLINE void keymap_event_set(wmKeyMapItem *kmi, const KeyMapItem_Params *params) {
	kmi->type = params->type;
	kmi->val = params->value;

	if (params->modifier == KM_ANY) {
		kmi->shift = kmi->ctrl = kmi->alt = kmi->oskey = KM_ANY;
	}
	else {
		const int mod = params->modifier & 0xff;
		const int mod_any = KMI_PARAMS_MOD_FROM_ANY(params->modifier);

		/* Only one of the flags should be set. */
		ROSE_assert((mod & mod_any) == 0);

		kmi->shift = ((mod & KM_SHIFT) ? KM_MOD_HELD : ((mod_any & KM_SHIFT) ? KM_ANY : KM_NOTHING));
		kmi->ctrl = ((mod & KM_CTRL) ? KM_MOD_HELD : ((mod_any & KM_CTRL) ? KM_ANY : KM_NOTHING));
		kmi->alt = ((mod & KM_ALT) ? KM_MOD_HELD : ((mod_any & KM_ALT) ? KM_ANY : KM_NOTHING));
		kmi->oskey = ((mod & KM_OSKEY) ? KM_MOD_HELD : ((mod_any & KM_OSKEY) ? KM_ANY : KM_NOTHING));
	}
}

ROSE_STATIC void keymap_item_properties_set(wmKeyMapItem *kmi) {
	WM_operator_properties_alloc(&(kmi->ptr), &(kmi->properties), kmi->idname);
}

wmKeyMapItem *WM_keymap_add_item(wmKeyMap *keymap, const char *idname, const KeyMapItem_Params *params) {
	wmKeyMapItem *kmi = MEM_callocN(sizeof(wmKeyMapItem), "wmKeyMapItem");
	LIB_strcpy(kmi->idname, ARRAY_SIZE(kmi->idname), idname);
	
	keymap_event_set(kmi, params);
	keymap_item_properties_set(kmi);

	LIB_addtail(&keymap->items, kmi);
	return kmi;
}

void wm_keymap_item_free_data(wmKeyMapItem *kmi) {
	if (kmi->ptr) {
		WM_operator_properties_free(kmi->ptr);
		MEM_freeN(kmi->ptr);
		kmi->ptr = NULL;
		kmi->properties = NULL;
	}
	else if (kmi->properties) {
		IDP_FreeProperty(kmi->properties);
		kmi->properties = NULL;
	}
}

void WM_keymap_clear(wmKeyMap *keymap) {
	LISTBASE_FOREACH_MUTABLE(wmKeyMapItem *, kmi, &keymap->items) {
		wm_keymap_item_free_data(kmi);
		MEM_freeN(kmi);
	}

	LIB_listbase_clear(&keymap->items);
}

bool WM_keymap_poll(rContext *C, wmKeyMap *keymap) {
	if (keymap->poll != NULL) {
		return keymap->poll(C);
	}

	return true;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name KeyConfig
 * \{ */

wmKeyConfig *WM_keyconfig_new(WindowManager *wm, const char *idname) {
	ROSE_assert(!LIB_findstr(&wm->runtime.keyconfigs, idname, offsetof(wmKeyConfig, idname)));
	/* Create new configuration. */
	wmKeyConfig *keyconf = MEM_callocN(sizeof(wmKeyConfig), "wmKeyConfig");
	LIB_strcpy(keyconf->idname, ARRAY_SIZE(keyconf->idname), idname);
	LIB_addtail(&wm->runtime.keyconfigs, keyconf);

	return keyconf;
}

void WM_keyconfig_clear(wmKeyConfig *keyconf) {
	LISTBASE_FOREACH_MUTABLE(wmKeyMap *, km, &keyconf->keymaps) {
		WM_keymap_clear(km);
		MEM_freeN(km);
	}

	LIB_listbase_clear(&keyconf->keymaps);
}

void WM_keyconfig_free(wmKeyConfig *keyconf) {
	WM_keyconfig_clear(keyconf);
	MEM_freeN(keyconf);
}

/** \} */
