#include "MEM_guardedalloc.h"

#include "KER_action.h"
#include "KER_anim_data.h"
#include "KER_idtype.h"
#include "KER_fcurve.h"

#include "LIB_utildefines.h"

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

	MEM_freeN(adt);
	iat->adt = NULL;
}
