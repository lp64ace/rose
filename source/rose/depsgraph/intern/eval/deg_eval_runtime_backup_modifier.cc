#include "intern/eval/deg_eval_runtime_backup_modifier.hh"

#include "DNA_modifier_types.h"

namespace rose::depsgraph {

ModifierDataBackup::ModifierDataBackup(ModifierData *modifier_data) : type(modifier_data->type), runtime(modifier_data->runtime) {
}

}  // namespace rose::depsgraph
