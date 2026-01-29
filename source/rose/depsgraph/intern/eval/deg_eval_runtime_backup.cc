#include "intern/eval/deg_eval_runtime_backup.hh"
#include "intern/eval/deg_eval_copy_on_write.hh"

#include "KER_lib_id.h"

#include "LIB_utildefines.h"

namespace rose::depsgraph {

RuntimeBackup::RuntimeBackup(const Depsgraph *depsgraph) : have_backup(false), object_backup(depsgraph) {
}

void RuntimeBackup::init_from_id(ID *id) {
	if (!deg_copy_on_write_is_expanded(id)) {
		return;
	}
	have_backup = true;

	const ID_Type id_type = GS(id->name);
	switch (id_type) {
		case ID_OB:
			object_backup.init_from_object(reinterpret_cast<Object *>(id));
			break;
		case ID_SCE:
			// scene_backup.init_from_scene(reinterpret_cast<Scene *>(id));
			break;
		default:
			break;
	}
}

void RuntimeBackup::restore_to_id(ID *id) {
	if (!have_backup) {
		return;
	}

	const ID_Type id_type = GS(id->name);
	switch (id_type) {
		case ID_OB:
			object_backup.restore_to_object(reinterpret_cast<Object *>(id));
			break;
		case ID_SCE:
			// scene_backup.restore_to_scene(reinterpret_cast<Scene *>(id));
			break;
		default:
			break;
	}
}

}  // namespace rose::depsgraph