#ifndef LIB_GENERIC_POINTER_HH
#define LIB_GENERIC_POINTER_HH

#include "LIB_cpp_type.hh"

namespace rose {

class GMutablePointer {
private:
	const CPPType *type_ = nullptr;
	void *data_ = nullptr;

public:
	GMutablePointer() = default;

	GMutablePointer(const CPPType *type, void *data = nullptr) : type_(type), data_(data) {
		ROSE_assert(data_ == nullptr || type_ != nullptr);
	}

	GMutablePointer(const CPPType &type, void *data = nullptr) : GMutablePointer(&type, data) {
	}

	template<typename T, ROSE_ENABLE_IF(!std::is_void_v<T>)> GMutablePointer(T *data) : GMutablePointer(&CPPType::get<T>(), data) {
	}

	void *get() const {
		return data_;
	}

	const CPPType *type() const {
		return type_;
	}

	template<typename T> T *get() const {
		ROSE_assert(this->is_type<T>());
		return static_cast<T *>(data_);
	}

	template<typename T> bool is_type() const {
		return type_ != nullptr && type_->is<T>();
	}

	template<typename T> T relocate_out() {
		ROSE_assert(this->is_type<T>());
		T value;
		type_->relocate_assign(data_, &value);
		data_ = nullptr;
		type_ = nullptr;
		return value;
	}

	void destruct() {
		ROSE_assert(data_ != nullptr);
		type_->destruct(data_);
	}
};

class GPointer {
public:
	const CPPType *type_ = nullptr;
	const void *data_ = nullptr;

public:
	GPointer() = default;

	GPointer(GMutablePointer ptr) : type_(ptr.type()), data_(ptr.get()) {
	}

	GPointer(const CPPType *type, const void *data = nullptr) : type_(type), data_(data) {
		ROSE_assert(data_ == nullptr || type_ != nullptr);
	}

	GPointer(const CPPType &type, const void *data = nullptr) : GPointer(&type, data) {
	}

	template<typename T> GPointer(T *data) : GPointer(&CPPType::get<T>(), data) {
	}

	const void *get() const {
		return data_;
	}

	const CPPType *type() const {
		return type_;
	}

	template<typename T> const T *get() const {
		ROSE_assert(this->is_type<T>());
		return static_cast<const T *>(data_);
	}

	template<typename T> bool is_type() const {
		return type_ != nullptr && type_->is<T>();
	}
};

}  // namespace rose

#endif	// LIB_GENERIC_POINTER_HH
