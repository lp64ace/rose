#include "LIB_virtual_array.hh"

#include <iostream>

void rose::internal::print_mutable_varray_span_warning() {
	std::cout << "Warning: Call `save()` to make sure that changes persist in all cases.\n";
}
