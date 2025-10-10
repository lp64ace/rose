#ifndef KER_ACTION_HH
#define KER_ACTION_HH

#include "KER_action.h"

#include "LIB_span.hh"

struct ActionSlot;
struct Main;

/* -------------------------------------------------------------------- */
/** \name Action Runtime
 * \{ */

rose::Span<ID *> KER_action_slot_runtime_users(ActionSlot *slot, Main *main);

/** \} */

#endif	// KER_ACTION_H