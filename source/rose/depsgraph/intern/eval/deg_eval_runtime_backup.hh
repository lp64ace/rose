#ifndef DEG_EVAL_RUNTIME_BACKUP_HH
#define DEG_EVAL_RUNTIME_BACKUP_HH

#include "DNA_ID.h"

#include "intern/eval/deg_eval_runtime_backup_object.hh"

namespace rose::depsgraph {
struct Depsgraph;

class RuntimeBackup {
public:
	explicit RuntimeBackup(const Depsgraph *depsgraph);

	/* NOTE: Will reset all runtime fields which has been backed up to nullptr. */
	void init_from_id(ID *id);

	/* Restore fields to the given ID. */
	void restore_to_id(ID *id);

	/* Denotes whether init_from_id did put anything into the backup storage.
	 * This will not be the case when init_from_id() is called for an ID which has never been
	 * copied-on-write. In this case there is no need to backup or restore anything.
	 *
	 * It also allows to have restore() logic to be symmetrical to init() without need to worry
	 * that init() might not have happened.
	 *
	 * In practice this is used by audio system to lock audio while scene is going through
	 * copy-on-write mechanism. */
	bool have_backup;

	ObjectRuntimeBackup object_backup;
};

}  // namespace rose::depsgraph

#endif	// !DEG_EVAL_RUNTIME_BACKUP_HH
