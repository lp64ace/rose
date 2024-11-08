#ifndef LIB_ARRAY_HH
#define LIB_ARRAY_HH

#include "LIB_allocator.hh"
#include "LIB_index_range.hh"
#include "LIB_memory_utils.hh"
#include "LIB_span.hh"
#include "LIB_utildefines.h"

namespace rose {

template<
	/**
	 * The type of the values stored in the array.
	 */
	typename T,
	/**
	 * The number of values that can be stored in the array, without doing a heap allocation.
	 */
	size_t InlineBufferCapacity = default_inline_buffer_capacity(sizeof(T)),
	/**
	 * The allocator used by this array. Should rarely be changed, except when you don't want that
	 * MEM_* functions are used internally.
	 */
	typename Allocator = GuardedAllocator>
class Array {
public:
	using value_type = T;
	using pointer = T *;
	using const_pointer = const T *;
	using reference = T &;
	using const_reference = const T &;
	using iterator = T *;
	using const_iterator = const T *;
	using size_type = size_t;

public:
	/** The beginning of the array. It might point into the inline buffer. */
	T *data_;

	/** Number of elements in the array. */
	size_type size_;

	/** Used for allocations when the inline buffer is too small. */
	ROSE_NO_UNIQUE_ADDRESS Allocator allocator_;

	/** A placeholder buffer that will remain uninitialized until it is used. */
	ROSE_NO_UNIQUE_ADDRESS TypedBuffer<T, InlineBufferCapacity> inline_buffer_;

public:
	Array(Allocator allocator = {}) noexcept : allocator_(allocator) {
		data_ = inline_buffer_;
		size_ = 0;
	}

	Array(NoExceptConstructor, Allocator allocator = {}) noexcept : Array(allocator) {
	}

	/**
	 * Create a new array that contains copies of all values.
	 */
	template<typename U, ROSE_ENABLE_IF((std::is_convertible_v<U, T>))>
	Array(Span<U> values, Allocator allocator = {}) : Array(NoExceptConstructor(), allocator) {
		const size_type size = values.size();
		data_ = this->get_buffer_for_size(size);
		uninitialized_convert_n<U, T>(values.data(), size, data_);
		size_ = size;
	}

	/**
	 * Create a new array with the given size. All values will be default constructed. For trivial
	 * types like int, default construction does nothing.
	 *
	 * We might want another version of this in the future, that does not do default construction
	 * even for non-trivial types. This should not be the default though, because one can easily mess
	 * up when dealing with uninitialized memory.
	 */
	explicit Array(size_type size, Allocator allocator = {}) : Array(NoExceptConstructor(), allocator) {
		data_ = this->get_buffer_for_size(size);
		default_construct_n(data_, size);
		size_ = size;
	}

	/**
	 * Create a new array with the given size. All values will be initialized by copying the given
	 * default.
	 */
	Array(size_type size, const T &value, Allocator allocator = {}) : Array(NoExceptConstructor(), allocator) {
		ROSE_assert(size >= 0);
		data_ = this->get_buffer_for_size(size);
		uninitialized_fill_n(data_, size, value);
		size_ = size;
	}

	/**
	 * Create a new array with uninitialized elements. The caller is responsible for constructing the
	 * elements. Moving, copying or destructing an Array with uninitialized elements invokes
	 * undefined behavior.
	 *
	 * This should be used very rarely. Note, that the normal size-constructor also does not
	 * initialize the elements when T is trivially constructible. Therefore, it only makes sense to
	 * use this with non trivially constructible types.
	 *
	 * Usage:
	 *  Array<std::string> my_strings(10, NoInitialization());
	 */
	Array(size_type size, NoInitialization, Allocator allocator = {}) : Array(NoExceptConstructor(), allocator) {
		ROSE_assert(size >= 0);
		data_ = this->get_buffer_for_size(size);
		size_ = size;
	}

	Array(const Array &other) : Array(other.as_span(), other.allocator_) {
	}

	Array(Array &&other) noexcept(std::is_nothrow_move_constructible_v<T>) : Array(NoExceptConstructor(), other.allocator_) {
		if (other.data_ == other.inline_buffer_) {
			uninitialized_relocate_n(other.data_, other.size_, data_);
		}
		else {
			data_ = other.data_;
		}
		size_ = other.size_;

		other.data_ = other.inline_buffer_;
		other.size_ = 0;
	}

	~Array() {
		destruct_n(data_, size_);
		this->deallocate_if_not_inline(data_);
	}

	Array &operator=(const Array &other) {
		return copy_assign_container(*this, other);
	}

	Array &operator=(Array &&other) noexcept(std::is_nothrow_move_constructible_v<T>) {
		return move_assign_container(*this, std::move(other));
	}

	T &operator[](size_type index) {
		ROSE_assert(index >= 0);
		ROSE_assert(index < size_);
		return data_[index];
	}

	const T &operator[](size_type index) const {
		ROSE_assert(index >= 0);
		ROSE_assert(index < size_);
		return data_[index];
	}

	operator Span<T>() const {
		return Span<T>(data_, size_);
	}

	operator MutableSpan<T>() {
		return MutableSpan<T>(data_, size_);
	}

	template<typename U, ROSE_ENABLE_IF((is_span_convertible_pointer_v<T, U>))> operator Span<U>() const {
		return Span<U>(data_, size_);
	}

	template<typename U, ROSE_ENABLE_IF((is_span_convertible_pointer_v<T, U>))> operator MutableSpan<U>() {
		return MutableSpan<U>(data_, size_);
	}

	Span<T> as_span() const {
		return *this;
	}

	MutableSpan<T> as_mutable_span() {
		return *this;
	}

	/**
	 * Returns the number of elements in the array.
	 */
	size_type size() const {
		return size_;
	}

	/**
	 * Returns true when the number of elements in the array is zero.
	 */
	bool is_empty() const {
		return size_ == 0;
	}

	/**
	 * Copies the given value to every element in the array.
	 */
	void fill(const T &value) const {
		initialized_fill_n(data_, size_, value);
	}

	/**
	 * Return a reference to the first element in the array.
	 * This invokes undefined behavior when the array is empty.
	 */
	const T &first() const {
		ROSE_assert(size_ > 0);
		return *data_;
	}
	T &first() {
		ROSE_assert(size_ > 0);
		return *data_;
	}

	/**
	 * Return a reference to the nth last element.
	 * This invokes undefined behavior when the array is too short.
	 */
	const T &last(const size_type n = 0) const {
		ROSE_assert(n >= 0);
		ROSE_assert(n < size_);
		return *(data_ + size_ - 1 - n);
	}
	T &last(const size_type n = 0) {
		ROSE_assert(n >= 0);
		ROSE_assert(n < size_);
		return *(data_ + size_ - 1 - n);
	}

	/**
	 * Get a pointer to the beginning of the array.
	 */
	const T *data() const {
		return data_;
	}
	T *data() {
		return data_;
	}

	const T *begin() const {
		return data_;
	}
	const T *end() const {
		return data_ + size_;
	}

	T *begin() {
		return data_;
	}
	T *end() {
		return data_ + size_;
	}

	std::reverse_iterator<T *> rbegin() {
		return std::reverse_iterator<T *>(this->end());
	}
	std::reverse_iterator<T *> rend() {
		return std::reverse_iterator<T *>(this->begin());
	}

	std::reverse_iterator<const T *> rbegin() const {
		return std::reverse_iterator<T *>(this->end());
	}
	std::reverse_iterator<const T *> rend() const {
		return std::reverse_iterator<T *>(this->begin());
	}

	/**
	 * Get an index range containing all valid indices for this array.
	 */
	IndexRange index_range() const {
		return IndexRange(size_);
	}

	/**
	 * Sets the size to zero. This should only be used when you have manually destructed all elements
	 * in the array beforehand. Use with care.
	 */
	void clear_without_destruct() {
		size_ = 0;
	}

	/**
	 * Access the allocator used by this array.
	 */
	Allocator &allocator() {
		return allocator_;
	}
	const Allocator &allocator() const {
		return allocator_;
	}

	/**
	 * Get the value of the InlineBufferCapacity template argument. This is the number of elements
	 * that can be stored without doing an allocation.
	 */
	static size_type inline_buffer_capacity() {
		return InlineBufferCapacity;
	}

	/**
	 * Destruct values and create a new array of the given size. The values in the new array are
	 * default constructed.
	 */
	void reinitialize(const size_type new_size) {
		ROSE_assert(new_size >= 0);
		size_type old_size = size_;

		destruct_n(data_, size_);
		size_ = 0;

		if (new_size <= old_size) {
			default_construct_n(data_, new_size);
		}
		else {
			T *new_data = this->get_buffer_for_size(new_size);
			try {
				default_construct_n(new_data, new_size);
			}
			catch (...) {
				this->deallocate_if_not_inline(new_data);
				throw;
			}
			this->deallocate_if_not_inline(data_);
			data_ = new_data;
		}

		size_ = new_size;
	}

private:
	T *get_buffer_for_size(size_type size) {
		if (size <= InlineBufferCapacity) {
			return inline_buffer_;
		}
		else {
			return this->allocate(size);
		}
	}

	T *allocate(size_type size) {
		return static_cast<T *>(allocator_.allocate(size_t(size) * sizeof(T), alignof(T), AT));
	}

	void deallocate_if_not_inline(T *ptr) {
		if (ptr != inline_buffer_) {
			allocator_.deallocate(ptr);
		}
	}
};

}  // namespace rose

#endif	// LIB_ARRAY_HH
