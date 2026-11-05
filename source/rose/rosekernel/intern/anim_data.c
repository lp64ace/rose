#include "MEM_guardedalloc.h"

#include "KER_action.h"
#include "KER_anim_data.h"
#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_main.h"
#include "KER_nla.h"
#include "KER_fcurve.h"

#include "LIB_utildefines.h"

#include "RLO_read_write.h"

/* -------------------------------------------------------------------- */
/** \name Animation Data Creation/Deletion
 * \{ */

bool id_type_can_have_animdata(const short id_type) {
	const IDTypeInfo *typeinfo = KER_idtype_get_info_from_idcode(id_type);
	if (typeinfo != NULL) {
		return (typeinfo->flag & IDTYPE_FLAGS_NO_ANIMDATA) == 0;
	}
	return false;
};

bool id_can_have_animdata(const ID *id) {
	if (id == NULL) {
		return false;
	}

	return id_type_can_have_animdata(GS(id->name));
}

AnimData *KER_animdata_from_id(const ID *id) {
	if (id_can_have_animdata(id)) {
		const IdAdtTemplate *iat = (const IdAdtTemplate *)id;
		return iat->adt;
	}
	return NULL;
}

AnimData *KER_animdata_ensure_id(ID *id) {
	if (id_can_have_animdata(id)) {
		IdAdtTemplate *iat = (IdAdtTemplate *)id;
		if (iat->adt == NULL) {
			iat->adt = (AnimData *)MEM_callocN(sizeof(AnimData), "AnimData");
		}
		return iat->adt;
	}
	return NULL;
}

AnimData *KER_animdata_copy_ex(Main *main, AnimData *adt, int flag) {
	const bool do_action = (flag & LIB_ID_COPY_ACTIONS) == 0 && (flag & LIB_ID_CREATE_NO_MAIN) == 0;
	const bool do_user = (flag & LIB_ID_CREATE_NO_USER_REFCOUNT) == 0;

	if (adt == NULL) {
		return NULL;
	}

	AnimData *new_adt = (AnimData *)MEM_dupallocN(adt);

	if (do_action) {
		const int id_copy_flag = (flag & LIB_ID_CREATE_NO_MAIN) == 0 ? (flag & ~LIB_ID_CREATE_NO_USER_REFCOUNT) : flag;

		new_adt->action = (Action *)KER_id_copy_ex(main, (ID *)adt->action, NULL, id_copy_flag);
	}
	else if (do_user) {
		id_us_add((ID *)new_adt->action);
	}

	KER_nla_tracks_copy_from_adt(main, new_adt, adt, flag);

	const bool is_main = (flag & LIB_ID_CREATE_NO_MAIN) == 0;
	if (is_main) {
	/**
	 * Action references were changed, so the Slot-to-user map is incomplete now. Only necessary
	 * when this happens in the main database though, as the user cache only tracks original IDs,
	 * not evaluated copies.
	 *
	 * This function does not have access to the animated ID, so it cannot just add that ID to the
	 * slot's users, hence the invalidation of the users map.
	 */
		if (main) {
			KER_action_slot_runtime_users_invalidate(main);
		}
	}

	return new_adt;
}

void KER_animdata_free(ID *id, const bool do_id_user) {
	if (!id_can_have_animdata(id)) {
		return;
	}

	IdAdtTemplate *iat = (IdAdtTemplate *)id;
	AnimData *adt = iat->adt;
	if (!adt) {
		return;
	}

	if (do_id_user) {
		if (adt->action) {
			const bool ok = KER_action_assign(NULL, id);
			ROSE_assert_msg(ok, "[Kernel] Expecting action un-assignment to always work");
			UNUSED_VARS_NDEBUG(ok);
		}
	}

	KER_nla_tracks_free(&adt->nla_tracks, do_id_user);

	MEM_freeN(adt);
	iat->adt = NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Rose Read/Write
 * \{ */

void KER_animdata_rose_write(RoseWriter *writer, AnimData *adt) {
	RLO_write_struct(writer, AnimData, adt);

	KER_nla_rose_write(writer, &adt->nla_tracks);
}

void KER_animdata_rose_read_data(RoseDataReader *reader, ID *id) {
	IdAdtTemplate *iat = id_can_have_animdata(id) ? (IdAdtTemplate *)id : NULL;
	if (!iat || !iat->adt) {
		return;
	}

	AnimData *adt = RLO_read_struct(reader, AnimData, &iat->adt);
	if (adt == NULL) {
		return;
	}

	RLO_read_struct_list(reader, NlaTrack, &adt->nla_tracks);
	KER_nla_rose_read_data(reader, id, &adt->nla_tracks);
}

void KER_animdata_rose_read_lib(RoseLibReader *reader, ID *id, AnimData *adt) {
	if (adt == NULL) {
		return;
	}

	RLO_read_id_address(reader, id->lib, &adt->action);

	LISTBASE_FOREACH (NlaTrack *, track, &adt->nla_tracks) {
		KER_nla_rose_read_lib(reader, id, &track->strips);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Iteration
 * \{ */

void KER_animdata_foreach_id(AnimData *adt, struct LibraryForeachIDData *data) {
	KER_LIB_FOREACHID_PROCESS_IDSUPER(data, adt->action, IDWALK_CB_USER);
	
	LISTBASE_FOREACH (NlaTrack *, nla_track, &adt->nla_tracks) {
		LISTBASE_FOREACH (NlaStrip *, nla_strip, &nla_track->strips) {
			KER_nla_strip_foreach_id(nla_strip, data);
		}
	}
}

/** \} */
