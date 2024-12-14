#include "LIB_cpp_type_make.hh"
#include "LIB_cpp_types_make.hh"

#include "KER_cpp_types.h"

#include "DNA_meshdata_types.h"

ROSE_CPP_TYPE_MAKE(MStringProperty, CPPTypeFlags::None);

void KER_cpp_types_init() {
	rose::register_cpp_types();
}
