#ifndef EDITORS_IO_UTILS_HH
#define EDITORS_IO_UTILS_HH

#include "LIB_vector.hh"

#include "WM_api.h"
#include "WM_handler.h"

#include <string>

struct PointerRNA;

struct rContext;
struct wmEvent;
struct wmOperator;
struct wmOperatorType;

namespace rose::editors::io {

/**
 * Shows a import dialog if the operator was invoked with filepath properties set,
 * otherwise invokes the file-select window.
 */
wmOperatorStatus filesel_drop_import_invoke(struct rContext *C, struct wmOperator *op, const struct wmEvent *event);

rose::Vector<std::string> paths_from_operator_properties(struct PointerRNA *ptr);

}  // namespace rose::editors::io

#ifdef __cplusplus
extern "C" {
#endif

void WM_OT_fbx_import(wmOperatorType *ot);

#ifdef __cplusplus
}
#endif

#endif	// !EDITORS_IO_UTILS_HH
