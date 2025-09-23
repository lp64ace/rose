#include "LIB_color.hh"
#include "LIB_cpp_type_make.hh"
#include "LIB_cpp_types_make.hh"
#include "LIB_math_quaternion_types.hh"
#include "LIB_math_vector_types.hh"

#include <ostream>

namespace rose {

static auto &get_vector_from_self_map() {
	static Map<const CPPType *, const VectorCPPType *> map;
	return map;
}

static auto &get_vector_from_value_map() {
	static Map<const CPPType *, const VectorCPPType *> map;
	return map;
}

void VectorCPPType::register_self() {
	get_vector_from_self_map().add_new(&this->self, this);
	get_vector_from_value_map().add_new(&this->value, this);
}

const VectorCPPType *VectorCPPType::get_from_self(const CPPType &self) {
	const VectorCPPType *type = get_vector_from_self_map().lookup_default(&self, nullptr);
	ROSE_assert(type == nullptr || type->self == self);
	return type;
}

const VectorCPPType *VectorCPPType::get_from_value(const CPPType &value) {
	const VectorCPPType *type = get_vector_from_value_map().lookup_default(&value, nullptr);
	ROSE_assert(type == nullptr || type->value == value);
	return type;
}

}  // namespace rose

ROSE_CPP_TYPE_MAKE(bool, CPPTypeFlags::BasicType)

ROSE_CPP_TYPE_MAKE(float, CPPTypeFlags::BasicType)
ROSE_CPP_TYPE_MAKE(float2, CPPTypeFlags::BasicType)
ROSE_CPP_TYPE_MAKE(float3, CPPTypeFlags::BasicType)
ROSE_CPP_TYPE_MAKE(uint3, CPPTypeFlags::BasicType)
ROSE_CPP_TYPE_MAKE(float4x4, CPPTypeFlags::BasicType)

ROSE_CPP_TYPE_MAKE(int8_t, CPPTypeFlags::BasicType)
ROSE_CPP_TYPE_MAKE(int16_t, CPPTypeFlags::BasicType)
ROSE_CPP_TYPE_MAKE(int32_t, CPPTypeFlags::BasicType)
ROSE_CPP_TYPE_MAKE(int2, CPPTypeFlags::BasicType)
ROSE_CPP_TYPE_MAKE(int64_t, CPPTypeFlags::BasicType)

ROSE_CPP_TYPE_MAKE(uint8_t, CPPTypeFlags::BasicType)
ROSE_CPP_TYPE_MAKE(uint16_t, CPPTypeFlags::BasicType)
ROSE_CPP_TYPE_MAKE(uint32_t, CPPTypeFlags::BasicType)
ROSE_CPP_TYPE_MAKE(uint64_t, CPPTypeFlags::BasicType)

ROSE_CPP_TYPE_MAKE(rose::ColorGeometry4f, CPPTypeFlags::BasicType)
ROSE_CPP_TYPE_MAKE(rose::ColorGeometry4b, CPPTypeFlags::BasicType)

ROSE_CPP_TYPE_MAKE(rose::math::Quaternion, CPPTypeFlags::BasicType | CPPTypeFlags::IdentityDefaultValue)

ROSE_CPP_TYPE_MAKE(std::string, CPPTypeFlags::BasicType)

ROSE_VECTOR_CPP_TYPE_MAKE(std::string)

namespace rose {

void register_cpp_types() {
	ROSE_CPP_TYPE_REGISTER(bool);

	ROSE_CPP_TYPE_REGISTER(float);
	ROSE_CPP_TYPE_REGISTER(float2);
	ROSE_CPP_TYPE_REGISTER(float3);
	ROSE_CPP_TYPE_REGISTER(uint3);

	ROSE_CPP_TYPE_REGISTER(int8_t);
	ROSE_CPP_TYPE_REGISTER(int16_t);
	ROSE_CPP_TYPE_REGISTER(int32_t);
	ROSE_CPP_TYPE_REGISTER(int2);
	ROSE_CPP_TYPE_REGISTER(int64_t);

	ROSE_CPP_TYPE_REGISTER(uint8_t);
	ROSE_CPP_TYPE_REGISTER(uint16_t);
	ROSE_CPP_TYPE_REGISTER(uint32_t);
	ROSE_CPP_TYPE_REGISTER(uint64_t);

	ROSE_CPP_TYPE_REGISTER(rose::ColorGeometry4f);
	ROSE_CPP_TYPE_REGISTER(rose::ColorGeometry4b);

	ROSE_CPP_TYPE_REGISTER(std::string);

	ROSE_VECTOR_CPP_TYPE_REGISTER(std::string);
}

}  // namespace rose
