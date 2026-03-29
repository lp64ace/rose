#include "WM_api.h"
#include "WM_handler.h"

#include "io_ops.h"
#include "io_utils.hh"

void ED_operatortypes_io() {
	WM_operatortype_append(WM_OT_fbx_import);
}
