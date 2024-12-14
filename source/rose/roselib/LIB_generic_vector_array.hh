#pragma once

/** \file
 *
 * A`GVectorArray` is a container for a fixed amount of dynamically growing vectors with a generic
 * data type. Its main use case is to store many small vectors with few separate allocations. Using
 * this structure is generally more efficient than allocating each vector separately.
 */

#include "LIB_array.hh"
#include "LIB_generic_virtual_vector_array.hh"
#include "LIB_linear_allocator.hh"

namespace rose {

/* An array of vectors containing elements of a generic type. */
class GVectorArray : NonCopyable, NonMovable {
private:
	struct Item {
		void *start = nullptr;
		size_t length = 0;
		size_t capacity = 0;
	};

	/* Use a linear allocator to pack many small vectors together. Currently, memory from
	 * reallocated vectors is not reused. This can be improved in the future. */
	LinearAllocator<> allocator_;
	/* The data type of individual elements. */
	const CPPType &type_;
	/* The size of an individual element. This is inlined from `type_.size()` for easier access. */
	const size_t element_size_;
	/* The individual vectors. */
	Array<Item> items_;

public:
	GVectorArray() = delete;

	GVectorArray(const CPPType &type, size_t array_size);

	~GVectorArray();

	size_t size() const {
		return items_.size();
	}

	bool is_empty() const {
		return items_.is_empty();
	}

	const CPPType &type() const {
		return type_;
	}

	void append(size_t index, const void *value);

	/* Add multiple elements to a single vector. */
	void extend(size_t index, const GVArray &values);
	void extend(size_t index, GSpan values);

	/* Add multiple elements to multiple vectors. */
	void extend(const IndexMask &mask, const GVVectorArray &values);
	void extend(const IndexMask &mask, const GVectorArray &values);

	void clear(const IndexMask &mask);

	GMutableSpan operator[](size_t index);
	GSpan operator[](size_t index) const;

private:
	void realloc_to_at_least(Item &item, size_t min_capacity);
};

/* A non-owning typed mutable reference to an `GVectorArray`. It simplifies access when the type of
 * the data is known at compile time. */
template<typename T> class GVectorArray_TypedMutableRef {
private:
	GVectorArray *vector_array_;

public:
	GVectorArray_TypedMutableRef(GVectorArray &vector_array) : vector_array_(&vector_array) {
		ROSE_assert(vector_array_->type().is<T>());
	}

	size_t size() const {
		return vector_array_->size();
	}

	bool is_empty() const {
		return vector_array_->is_empty();
	}

	void append(const size_t index, const T &value) {
		vector_array_->append(index, &value);
	}

	void extend(const size_t index, const Span<T> values) {
		vector_array_->extend(index, values);
	}

	void extend(const size_t index, const VArray<T> &values) {
		vector_array_->extend(index, values);
	}

	MutableSpan<T> operator[](const size_t index) {
		return (*vector_array_)[index].typed<T>();
	}
};

/* A generic virtual vector array implementation for a `GVectorArray`. */
class GVVectorArray_For_GVectorArray : public GVVectorArray {
private:
	const GVectorArray &vector_array_;

public:
	GVVectorArray_For_GVectorArray(const GVectorArray &vector_array) : GVVectorArray(vector_array.type(), vector_array.size()), vector_array_(vector_array) {
	}

protected:
	size_t get_vector_size_impl(const size_t index) const override {
		return vector_array_[index].size();
	}

	void get_vector_element_impl(const size_t index, const size_t index_in_vector, void *r_value) const override {
		type_->copy_assign(vector_array_[index][index_in_vector], r_value);
	}
};

}  // namespace rose
