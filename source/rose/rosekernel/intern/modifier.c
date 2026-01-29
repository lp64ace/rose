#include "MEM_guardedalloc.h"

#include "MOD_modifiertypes.h"

#include "LIB_listbase.h"
#include "LIB_string.h"

#include "KER_scene.h"
#include "KER_object.h"
#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_mesh.h"
#include "KER_modifier.h"

static ModifierTypeInfo *mod_types[NUM_MODIFIER_TYPES];

/* -------------------------------------------------------------------- */
/** \name Modifier Type Info Methods
 * \{ */

void KER_modifier_init(void) {
	MOD_modifier_types_init(mod_types);
}

const ModifierTypeInfo *KER_modifier_get_info(eModifierType type) {
	if (MODIFIER_TYPE_NONE <= type && type < NUM_MODIFIER_TYPES && mod_types[type]->name[0] != '\0') {
		return mod_types[type];
	}

	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Modifier Data Methods
 * \{ */

ROSE_STATIC ModifierData *modifier_allocate_and_init(eModifierType type) {
	const ModifierTypeInfo *mti = KER_modifier_get_info(type);
	if (!mti) {
		return NULL;
	}

	ModifierData *md = (ModifierData *)MEM_callocN(mti->size, mti->name);

	LIB_strcpy(md->name, ARRAY_SIZE(md->name), mti->name);

	md->type = type;
	
	if (mti->init_data) {
		mti->init_data(md);
	}

	return md;
}


ROSE_INLINE void modifier_copy_data_id_us_cb(void *user_data, struct Object *ob, struct ID **idpoin, const int cb_flag) {
	ID *id = *idpoin;
	if (id != NULL && (cb_flag & IDWALK_CB_USER) != 0) {
		id_us_add(id);
	}
}

void KER_modifier_copy_data_ex(ModifierData *target, const ModifierData *md, const int flag) {
	const ModifierTypeInfo *mti = KER_modifier_get_info(md->type);

	target->flag = md->flag;

	if (mti->copy_data) {
		mti->copy_data(md, target, flag);
	}

	if ((flag & LIB_ID_CREATE_NO_USER_REFCOUNT) == 0) {
    if (mti->foreach_ID_link) {
      mti->foreach_ID_link(target, NULL, modifier_copy_data_id_us_cb, NULL);
    }
  }
}

ModifierData *KER_modifier_new(eModifierType type) {
	return modifier_allocate_and_init(type);
}

ModifierData *KER_modifier_copy_ex(const ModifierData *md, int flag) {
	ModifierData *md_dst = modifier_allocate_and_init(md->type);

	LIB_strcpy(md_dst->name, ARRAY_SIZE(md_dst->name), md->name);
	KER_modifier_copy_data_ex(md_dst, md, flag);

	return md_dst;
}

ROSE_STATIC void modifier_free_data_id_us_cb(void *userdata, struct Object *object, struct ID **idpoint, const int flag) {
	ID *id = *idpoint;
	if (id != NULL && (flag & IDWALK_CB_USER) != 0) {
		id_us_rem(id);
	}
}

void KER_modifier_free_ex(ModifierData *md, int flag) {
	const ModifierTypeInfo *mti = KER_modifier_get_info((eModifierType)md->type);

	if ((flag & LIB_ID_FREE_NO_USER_REFCOUNT) == 0) {
		if (mti->foreach_ID_link) {
			mti->foreach_ID_link(md, NULL, modifier_free_data_id_us_cb, NULL);
		}
	}

	if (mti->free_data) {
		mti->free_data(md);
	}
	if (md->error) {
		MEM_freeN(md->error);
	}

	MEM_freeN(md);
}

void KER_modifier_free(ModifierData *md) {
	KER_modifier_free_ex(md, 0);
}

void KER_modifier_remove_from_list(Object *object, ModifierData *md) {
	ROSE_assert(LIB_haslink(&object->modifiers, md));

	LIB_remlink(&object->modifiers, md);
}

bool KER_modifier_depends_ontime(Scene *scene, ModifierData *md) {
	const ModifierTypeInfo *mti = KER_modifier_get_info((eModifierType)md->type);

	return mti->depends_on_time && mti->depends_on_time(scene, md);
}

bool KER_modifier_deform_verts(ModifierData *md, const ModifierEvalContext *ctx, struct Mesh *mesh, float (*positions)[3], size_t length) {
	const ModifierTypeInfo *mti = KER_modifier_get_info(md->type);

	if (mti->deform_verts) {
		if ((md->flag & MODIFIER_DEVICE_ONLY) == 0) {
			mti->deform_verts(md, ctx, mesh, positions, length);
			if (mesh) {
				KER_mesh_positions_changed(mesh);
			}
		}

		return true;
	}

	return false;
}

/** \} */


/* -------------------------------------------------------------------- */
/** \name ModifierData for each Query
 * \{ */

void KER_modifiers_foreach_ID_link(Object *object, IDWalkFunc walk, void *user_data) {
	LISTBASE_FOREACH(ModifierData *, md, &object->modifiers) {
		const ModifierTypeInfo *mti = KER_modifier_get_info(md->type);

		if (mti->foreach_ID_link) {
			mti->foreach_ID_link(md, object, walk, user_data);
		}
	}
}

/** \} */
