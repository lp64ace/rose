#ifndef LIB_CPP_TYPES_MAKES_HH
#define LIB_CPP_TYPES_MAKES_HH

#include "LIB_cpp_type_make.hh"
#include "LIB_cpp_types.hh"

namespace rose {

template<typename ValueType> inline VectorCPPType::VectorCPPType(TypeTag<ValueType> /*value_type*/) : self(CPPType::get<Vector<ValueType>>()), value(CPPType::get<ValueType>()) {
	this->register_self();
}

}  // namespace rose

/** Create a new #VectorCPPType that can be accessed through `VectorCPPType::get<T>()`. */
#define ROSE_VECTOR_CPP_TYPE_MAKE(VALUE_TYPE)                                           \
	ROSE_CPP_TYPE_MAKE(rose::Vector<VALUE_TYPE>, CPPTypeFlags::None)                    \
	template<> const rose::VectorCPPType &rose::VectorCPPType::get_impl<VALUE_TYPE>() { \
		static rose::VectorCPPType type{rose::TypeTag<VALUE_TYPE>{}};                   \
		return type;                                                                    \
	}

/** Register a #VectorCPPType created with #ROSE_VECTOR_CPP_TYPE_MAKE. */
#define ROSE_VECTOR_CPP_TYPE_REGISTER(VALUE_TYPE) rose::VectorCPPType::get<VALUE_TYPE>()

#endif	// LIB_CPP_TYPES_MAKES_HH
