#ifndef DEG_EVAL_RUNTIME_BACKUP_OBJECT_HH
#define DEG_EVAL_RUNTIME_BACKUP_OBJECT_HH

#include "DNA_object_types.h"

#include "LIB_map.hh"
#include "LIB_session_uuid.h"

#include "intern/eval/deg_eval_runtime_backup_modifier.hh"

namespace rose::depsgraph {

struct Depsgraph;

class ObjectRuntimeBackup {
public:
	ObjectRuntimeBackup(const Depsgraph *depsgraph);

	/**
	 * Make a backup of object's evaluation runtime data, additionally 
	 * make object to be safe for free without invalidating backed up 
	 * pointers.
	 */
	void init_from_object(Object *object);
	void backup_modifier_runtime_data(Object *object);
	void backup_pose_channel_runtime_data(Object *object);

	/* Restore all fields to the given object. */
	void restore_to_object(Object *object);
	/* NOTE: Will free all runtime data which has not been restored. */
	void restore_modifier_runtime_data(Object *object);
	void restore_pose_channel_runtime_data(Object *object);

	Object_Runtime runtime;
	Map<SessionUUID, ModifierDataBackup> modifier_runtime_data;
	Map<SessionUUID, PoseChannel_Runtime> pose_channel_runtime_data;
};

}

#endif	// !DEG_EVAL_RUNTIME_BACKUP_OBJECT_HH
