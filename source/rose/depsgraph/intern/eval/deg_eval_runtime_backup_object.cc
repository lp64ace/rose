#include "LIB_listbase.h"

#include "KER_action.h"
#include "KER_object.h"
#include "KER_modifier.h"

#include "deg_eval_runtime_backup_object.hh"

namespace rose::depsgraph {

ObjectRuntimeBackup::ObjectRuntimeBackup(const Depsgraph *depsgraph) {
	memset(&runtime, 0, sizeof(runtime));
}

void ObjectRuntimeBackup::init_from_object(Object *object) {
	/* Store evaluated mesh and curve_cache, and make sure we don't free it. */
	runtime = object->runtime;
	KER_object_runtime_reset(object);

	/* Keep bbox (for now at least). */
	object->runtime.bb = runtime.bb;
	/**
	 * Object update will override actual object->data to an evaluated version.
	 * Need to make sure we don't have data set to evaluated one before free
	 * anything.
	 */
	object->data = runtime.data_orig;
	/* Backup runtime data of all modifiers. */
	backup_modifier_runtime_data(object);
	/* Backup runtime data of all pose channels. */
	backup_pose_channel_runtime_data(object);
}

void ObjectRuntimeBackup::backup_modifier_runtime_data(Object *object) {
	LISTBASE_FOREACH(ModifierData *, md, &object->modifiers) {
		if (md->runtime == nullptr) {
			continue;
		}

		const SessionUUID &session_uuid = md->uuid;
		ROSE_assert(LIB_session_uuid_is_generated(&session_uuid));

		modifier_runtime_data.add(session_uuid, ModifierDataBackup(md));
		md->runtime = nullptr;
	}
}

void ObjectRuntimeBackup::backup_pose_channel_runtime_data(Object *object) {
	if (object->pose != nullptr) {
		LISTBASE_FOREACH(PoseChannel *, pchannel, &object->pose->channelbase) {
			const SessionUUID &session_uuid = pchannel->runtime.uuid;
			ROSE_assert(LIB_session_uuid_is_generated(&session_uuid));

			pose_channel_runtime_data.add(session_uuid, pchannel->runtime);
			KER_pose_channel_runtime_reset(&pchannel->runtime);
		}
	}
}

void ObjectRuntimeBackup::restore_to_object(Object *object) {
	ID *data_orig = static_cast<ID *>(object->runtime.data_orig);
	ID *data_eval = static_cast<ID *>(runtime.data_eval);
	BoundBox *bb = object->runtime.bb;
	object->runtime = runtime;
	object->runtime.data_orig = data_orig;
	object->runtime.bb = bb;
	if (ELEM(object->type, OB_MESH) && data_eval != nullptr) {
		if (object->id.recalc & ID_RECALC_GEOMETRY) {
			/* If geometry is tagged for update it means, that part of
			 * evaluated mesh are not valid anymore. In this case we can not
			 * have any "persistent" pointers to point to an invalid data.
			 *
			 * We restore object's data datablock to an original copy of
			 * that datablock. */
			object->data = data_orig;

			/* After that, immediately free the invalidated caches. */
			KER_object_free_derived_caches(object);
		}
		else {
			/* Do same thing as object update: override actual object data pointer with evaluated
			 * datablock, but only if the evaluated data has the same type as the original data. */
			if (GS(((ID *)object->data)->name) == GS(data_eval->name)) {
				object->data = data_eval;
			}
		}
	}

	/**
	 * Restore modifier's runtime data.
	 * NOTE: Data of unused modifiers will be freed there.
	 */
	restore_modifier_runtime_data(object);
	restore_pose_channel_runtime_data(object);
}

void ObjectRuntimeBackup::restore_modifier_runtime_data(Object *object) {
	LISTBASE_FOREACH(ModifierData *, modifier_data, &object->modifiers) {
		const SessionUUID &session_uuid = modifier_data->uuid;
		ROSE_assert(LIB_session_uuid_is_generated(&session_uuid));

		std::optional<ModifierDataBackup> backup = modifier_runtime_data.pop_try(session_uuid);
		if (backup.has_value()) {
			modifier_data->runtime = backup->runtime;
		}
	}

	for (ModifierDataBackup &backup : modifier_runtime_data.values()) {
		const ModifierTypeInfo *modifier_type_info = KER_modifier_get_info(static_cast<eModifierType>(backup.type));
		ROSE_assert(modifier_type_info != nullptr);
		modifier_type_info->free_runtime_data(backup.runtime);
	}
}

void ObjectRuntimeBackup::restore_pose_channel_runtime_data(Object *object) {
	if (object->pose != nullptr) {
		LISTBASE_FOREACH(PoseChannel *, pchan, &object->pose->channelbase) {
			const SessionUUID &session_uuid = pchan->runtime.uuid;
			std::optional<PoseChannel_Runtime> runtime = pose_channel_runtime_data.pop_try(session_uuid);
			if (runtime.has_value()) {
				pchan->runtime = *runtime;
			}
		}
	}
	for (PoseChannel_Runtime &runtime : pose_channel_runtime_data.values()) {
		KER_pose_channel_runtime_free(&runtime);
	}
}

}  // namespace rose::depsgraph
