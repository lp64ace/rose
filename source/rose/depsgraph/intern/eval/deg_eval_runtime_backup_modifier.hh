#ifndef DEG_EVAL_RUNTIME_MODIFIER_HH
#define DEG_EVAL_RUNTIME_MODIFIER_HH

#include "KER_modifier.h"

namespace rose::depsgraph {

class ModifierDataBackup {
public:
	explicit ModifierDataBackup(ModifierData *modifier_data);

	int type;
	void *runtime;
};

}  // namespace rose::depsgraph

#endif	// !DEG_EVAL_RUNTIME_MODIFIER_HH
