#ifndef LIB_IMPLIT_SHARING_PTR_HH
#define LIB_IMPLIT_SHARING_PTR_HH

#include "LIB_implicit_sharing.hh"
#include "LIB_struct_equality_utils.hh"

namespace rose {

/**
 * #ImplicitSharingPtr is a smart ppointer that manages implicit sharing. It's designed to work with
 * types that derive from #ImplicitSharingMixin. It is fairly similar to #std::shared_ptr but
 * requires the reference count to be embedded in the data.
 */
template<typename T> class ImplicitSharingPtr {
private:
	const T *data_ = nullptr;

public:
	ImplicitSharingPtr() = default;

	explicit ImplicitSharingPtr(const T *data) : data_(data) {
	}

	/* Implicit conversion from nullptr */
	ImplicitSharingPtr(std::nullptr_t) : data_(nullptr) {
	}

	ImplicitSharingPtr(const ImplicitSharingPtr &other) : data_(other.data_) {
		this->add_user(data_);
	}

	ImplicitSharingPtr(ImplicitSharingPtr &&other) : data_(other.data_) {
		other.data_ = nullptr;
	}

	~ImplicitSharingPtr() {
		this->remove_user_and_delete_if_last(data_);
	}

	ImplicitSharingPtr &operator=(const ImplicitSharingPtr &other) {
		if (this == &other) {
			return *this;
		}

		this->remove_user_and_delete_if_last(data_);
		data_ = other.data_;
		this->add_user(data_);
		return *this;
	}

	ImplicitSharingPtr &operator=(ImplicitSharingPtr &&other) {
		if (this == &other) {
			return *this;
		}

		this->remove_user_and_delete_if_last(data_);
		data_ = other.data_;
		other.data_ = nullptr;
		return *this;
	}

	const T *operator->() const {
		ROSE_assert(data_ != nullptr);
		return data_;
	}

	const T &operator*() const {
		ROSE_assert(data_ != nullptr);
		return *data_;
	}

	operator bool() const {
		return data_ != nullptr;
	}

	const T *get() const {
		return data_;
	}

	const T *release() {
		T *data = data_;
		data_ = nullptr;
		return data;
	}

	void reset() {
		this->remove_user_and_delete_if_last(data_);
		data_ = nullptr;
	}

	bool has_value() const {
		return data_ != nullptr;
	}

	uint64_t hash() const {
		return get_default_hash(data_);
	}

	ROSE_STRUCT_EQUALITY_OPERATORS_1(ImplicitSharingPtr, data_)

private:
	static void add_user(const T *data) {
		if (data != nullptr) {
			data->add_user();
		}
	}

	static void remove_user_and_delete_if_last(const T *data) {
		if (data != nullptr) {
			data->remove_user_and_delete_if_last();
		}
	}
};

}  // namespace rose

#endif	// LIB_IMPLIT_SHARING_PTR_HH
