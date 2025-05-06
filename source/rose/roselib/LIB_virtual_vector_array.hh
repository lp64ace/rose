#pragma once

#include "LIB_virtual_array.hh"

namespace rose {

/** A read-only virtual array of vectors. */
template<typename T> class VVectorArray {
protected:
	size_t size_;

public:
	VVectorArray(const size_t size) : size_(size) {
	}

	virtual ~VVectorArray() = default;

	/* Returns the number of vectors in the vector array. */
	size_t size() const {
		return size_;
	}

	/* Returns true when there is no vector in the vector array. */
	bool is_empty() const {
		return size_ == 0;
	}

	/* Returns the size of the vector at the given index. */
	size_t get_vector_size(const size_t index) const {
		ROSE_assert(index < size_);
		return this->get_vector_size_impl(index);
	}

	/* Returns an element from one of the vectors. */
	T get_vector_element(const size_t index, const size_t index_in_vector) const {
		ROSE_assert(index < size_);
		ROSE_assert(index_in_vector < this->get_vector_size(index));
		return this->get_vector_element_impl(index, index_in_vector);
	}

	/* Returns true when the same vector is used at every index. */
	bool is_single_vector() const {
		if (size_ == 1) {
			return true;
		}
		return this->is_single_vector_impl();
	}

protected:
	virtual size_t get_vector_size_impl(size_t index) const = 0;

	virtual T get_vector_element_impl(size_t index, size_t index_in_vetor) const = 0;

	virtual bool is_single_vector_impl() const {
		return false;
	}
};

}  // namespace rose
