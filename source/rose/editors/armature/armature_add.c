#include "MEM_guardedalloc.h"

#include "LIB_listbase.h"
#include "LIB_string.h"

#include "KER_armature.h"

#include "ED_armature.h"

EditBone *ED_armature_ebone_add(Armature *armature, const char *name) {
	EditBone *bone = (EditBone *)MEM_callocN(sizeof(EditBone), "editors::EditBone");

	LIB_strcpy(bone->name, ARRAY_SIZE(bone->name), name);

	bone->weight = 1.0f;

	LIB_addhead(armature->ebonebase, bone);

	return bone;
}
