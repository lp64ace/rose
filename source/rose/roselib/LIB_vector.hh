#ifndef LIB_VECTOR_HH
#define LIB_VECTOR_HH

#include <algorithm>

#include "LIB_allocator.hh"
#include "LIB_index_range.hh"
#include "LIB_memory_utils.hh"
#include "LIB_span.hh"
#include "LIB_utildefines.h"

namespace rose {

template<
	/**
	 * Type of the values stored in this vector. It has to be movable.
	 */
	typename T,
	/**
	 * The number of values that can be stored in this vector, without doing a heap allocation.
	 * Sometimes it makes sense to increase this value a lot. The memory in the inline buffer is
	 * not initialized when it is not needed.
	 *
	 * When T is large, the small buffer optimization is disabled by default to avoid large
	 * unexpected allocations on the stack. It can still be enabled explicitly though.
	 */
	size_t InlineBufferCapacity = default_inline_buffer_capacity(sizeof(T)),
	/**
	 * The allocator used by this vector. Should rarely be changed, except when you don't want that
	 * MEM_* is used internally.
	 */
	typename Allocator = GuardedAllocator>
class Vector {
public:
	using value_type = T;
	using pointer = T *;
	using const_pointer = const T *;
	using reference = T &;
	using const_reference = const T &;
	using iterator = T *;
	using const_iterator = const T *;
	using size_type = size_t;

	/* Similar to string::npos. */
	static constexpr size_t not_found = -1;

private:
	/**
	 * Use pointers instead of storing the size explicitly. This reduces the number of instructions
	 * in `append`.
	 *
	 * The pointers might point to the memory in the inline buffer.
	 */
	T *begin_;
	T *end_;
	T *capacity_end_;

	/** Used for allocations when the inline buffer is too small. */
	ROSE_NO_UNIQUE_ADDRESS Allocator allocator_;

	/** A placeholder buffer that will remain uninitialized until it is used. */
	ROSE_NO_UNIQUE_ADDRESS TypedBuffer<T, InlineBufferCapacity> inline_buffer_;

	/**
	 * Be a friend with other vector instantiations. This is necessary to implement some memory
	 * management logic.
	 */
	template<typename OtherT, size_t OtherInlineBufferCapacity, typename OtherAllocator> friend class Vector;

	/** Required in case `T` is an incomplete type. */
	static constexpr bool is_nothrow_move_constructible() {
		if constexpr (InlineBufferCapacity == 0) {
			return true;
		}
		else {
			return std::is_nothrow_move_constructible_v<T>;
		}
	}

public:
	/**
	 * Create an empty vector.
	 * This does not do any memory allocation.
	 */
	Vector(Allocator allocator = {}) noexcept : allocator_(allocator) {
		begin_ = inline_buffer_;
		end_ = begin_;
		capacity_end_ = begin_ + InlineBufferCapacity;
	}

	Vector(NoExceptConstructor, Allocator allocator = {}) noexcept : Vector(allocator) {
	}

	/**
	 * Create a vector with a specific size.
	 * The elements will be default constructed.
	 * If T is trivially constructible, the elements in the vector are not touched.
	 */
	explicit Vector(size_t size, Allocator allocator = {}) : Vector(NoExceptConstructor(), allocator) {
		this->resize(size);
	}

	/**
	 * Create a vector filled with a specific value.
	 */
	Vector(size_t size, const T &value, Allocator allocator = {}) : Vector(NoExceptConstructor(), allocator) {
		this->resize(size, value);
	}

	/**
	 * Create a vector from a span. The values in the vector are copy constructed.
	 */
	template<typename U, ROSE_ENABLE_IF((std::is_convertible_v<U, T>))> Vector(Span<U> values, Allocator allocator = {}) : Vector(NoExceptConstructor(), allocator) {
		const size_t size = values.size();
		this->reserve(size);
		uninitialized_convert_n<U, T>(values.data(), size, begin_);
		this->increase_size_by_unchecked(size);
	}

	template<typename U, ROSE_ENABLE_IF((std::is_convertible_v<U, T>))> explicit Vector(MutableSpan<U> values, Allocator allocator = {}) : Vector(values.as_span(), allocator) {
	}

	/**
	 * Create a vector that contains copies of the values in the initialized list.
	 *
	 * This allows you to write code like:
	 * Vector<int> vec = {3, 4, 5};
	 */
	template<typename U, ROSE_ENABLE_IF((std::is_convertible_v<U, T>))> Vector(const std::initializer_list<U> &values) : Vector(Span<U>(values)) {
	}

	Vector(const std::initializer_list<T> &values) : Vector(Span<T>(values)) {
	}

	template<typename U, size_t N, ROSE_ENABLE_IF((std::is_convertible_v<U, T>))> Vector(const std::array<U, N> &values) : Vector(Span(values)) {
	}

	template<typename InputIt, ROSE_ENABLE_IF((!std::is_convertible_v<InputIt, int>))> Vector(InputIt first, InputIt last, Allocator allocator = {}) : Vector(NoExceptConstructor(), allocator) {
		for (InputIt current = first; current != last; ++current) {
			this->append(*current);
		}
	}

	/**
	 * Create a copy of another vector. The other vector will not be changed. If the other vector has
	 * less than InlineBufferCapacity elements, no allocation will be made.
	 */
	Vector(const Vector &other) : Vector(other.as_span(), other.allocator_) {
	}

	/**
	 * Create a copy of a vector with a different InlineBufferCapacity. This needs to be handled
	 * separately, so that the other one is a valid copy constructor.
	 */
	template<size_t OtherInlineBufferCapacity> Vector(const Vector<T, OtherInlineBufferCapacity, Allocator> &other) : Vector(other.as_span(), other.allocator_) {
	}

	/**
	 * Steal the elements from another vector. This does not do an allocation. The other vector will
	 * have zero elements afterwards.
	 */
	template<size_t OtherInlineBufferCapacity> Vector(Vector<T, OtherInlineBufferCapacity, Allocator> &&other) noexcept(is_nothrow_move_constructible()) : Vector(NoExceptConstructor(), other.allocator_) {
		const size_t size = other.size();

		if (other.is_inline()) {
			if (size <= InlineBufferCapacity) {
				/* Copy between inline buffers. */
				uninitialized_relocate_n(other.begin_, size, begin_);
				end_ = begin_ + size;
			}
			else {
				/* Copy from inline buffer to newly allocated buffer. */
				const size_t capacity = size;
				begin_ = static_cast<T *>(allocator_.allocate(sizeof(T) * size_t(capacity), alignof(T), AT));
				capacity_end_ = begin_ + capacity;
				uninitialized_relocate_n(other.begin_, size, begin_);
				end_ = begin_ + size;
			}
		}
		else {
			/* Steal the pointer. */
			begin_ = other.begin_;
			end_ = other.end_;
			capacity_end_ = other.capacity_end_;
		}

		other.begin_ = other.inline_buffer_;
		other.end_ = other.begin_;
		other.capacity_end_ = other.begin_ + OtherInlineBufferCapacity;
	}

	~Vector() {
		destruct_n(begin_, this->size());
		if (!this->is_inline()) {
			allocator_.deallocate(begin_);
		}
	}

	Vector &operator=(const Vector &other) {
		return copy_assign_container(*this, other);
	}

	Vector &operator=(Vector &&other) {
		return move_assign_container(*this, std::move(other));
	}

	/**
	 * Get the value at the given index. This invokes undefined behavior when the index is out of
	 * bounds.
	 */
	const T &operator[](size_t index) const {
		ROSE_assert(index < this->size());
		return begin_[index];
	}

	T &operator[](size_t index) {
		ROSE_assert(index < this->size());
		return begin_[index];
	}

	operator Span<T>() const {
		return Span<T>(begin_, this->size());
	}

	operator MutableSpan<T>() {
		return MutableSpan<T>(begin_, this->size());
	}

	template<typename U, ROSE_ENABLE_IF((is_span_convertible_pointer_v<T, U>))> operator Span<U>() const {
		return Span<U>(begin_, this->size());
	}

	template<typename U, ROSE_ENABLE_IF((is_span_convertible_pointer_v<T, U>))> operator MutableSpan<U>() {
		return MutableSpan<U>(begin_, this->size());
	}

	Span<T> as_span() const {
		return *this;
	}

	MutableSpan<T> as_mutable_span() {
		return *this;
	}

	/**
	 * Make sure that enough memory is allocated to hold min_capacity elements.
	 * This won't necessarily make an allocation when min_capacity is small.
	 * The actual size of the vector does not change.
	 */
	void reserve(const size_t min_capacity) {
		if (min_capacity > this->capacity()) {
			this->realloc_to_at_least(min_capacity);
		}
	}

	/**
	 * Change the size of the vector so that it contains new_size elements.
	 * If new_size is smaller than the old size, the elements at the end of the vector are
	 * destructed. If new_size is larger than the old size, the new elements at the end are default
	 * constructed. If T is trivially constructible, the memory is not touched by this function.
	 */
	void resize(const size_t new_size) {
		const size_t old_size = this->size();
		if (new_size > old_size) {
			this->reserve(new_size);
			default_construct_n(begin_ + old_size, new_size - old_size);
		}
		else {
			destruct_n(begin_ + new_size, old_size - new_size);
		}
		end_ = begin_ + new_size;
	}

	/**
	 * Change the size of the vector so that it contains new_size elements.
	 * If new_size is smaller than the old size, the elements at the end of the vector are
	 * destructed. If new_size is larger than the old size, the new elements will be copy constructed
	 * from the given value.
	 */
	void resize(const size_t new_size, const T &value) {
		const size_t old_size = this->size();
		if (new_size > old_size) {
			this->reserve(new_size);
			uninitialized_fill_n(begin_ + old_size, new_size - old_size, value);
		}
		else {
			destruct_n(begin_ + new_size, old_size - new_size);
		}
		end_ = begin_ + new_size;
	}

	/**
	 * Reset the size of the vector so that it contains new_size elements.
	 * All existing elements are destructed, and not copied if the data must be reallocated.
	 */
	void reinitialize(const size_t new_size) {
		this->clear();
		this->resize(new_size);
	}

	/**
	 * Afterwards the vector has 0 elements, but will still have
	 * memory to be refilled again.
	 */
	void clear() {
		destruct_n(begin_, this->size());
		end_ = begin_;
	}

	/**
	 * Afterwards the vector has 0 elements and any allocated memory
	 * will be freed.
	 */
	void clear_and_shrink() {
		destruct_n(begin_, this->size());
		if (!this->is_inline()) {
			allocator_.deallocate(begin_);
		}

		begin_ = inline_buffer_;
		end_ = begin_;
		capacity_end_ = begin_ + InlineBufferCapacity;
	}

	/**
	 * Insert a new element at the end of the vector.
	 * This might cause a reallocation with the capacity is exceeded.
	 *
	 * This is similar to std::vector::push_back.
	 */
	void append(const T &value) {
		this->append_as(value);
	}
	void append(T &&value) {
		this->append_as(std::move(value));
	}
	/* This is similar to `std::vector::emplace_back`. */
	template<typename... ForwardValue> void append_as(ForwardValue &&...value) {
		this->ensure_space_for_one();
		this->append_unchecked_as(std::forward<ForwardValue>(value)...);
	}

	/**
	 * Append the value to the vector and return the index that can be used to access the newly
	 * added value.
	 */
	size_t append_and_get_index(const T &value) {
		return this->append_and_get_index_as(value);
	}
	size_t append_and_get_index(T &&value) {
		return this->append_and_get_index_as(std::move(value));
	}
	template<typename... ForwardValue> size_t append_and_get_index_as(ForwardValue &&...value) {
		const size_t index = this->size();
		this->append_as(std::forward<ForwardValue>(value)...);
		return index;
	}

	/**
	 * Append the value if it is not yet in the vector. This has to do a linear search to check if
	 * the value is in the vector. Therefore, this should only be called when it is known that the
	 * vector is small.
	 */
	void append_non_duplicates(const T &value) {
		if (!this->contains(value)) {
			this->append(value);
		}
	}

	/**
	 * Append the value and assume that vector has enough memory reserved. This invokes undefined
	 * behavior when not enough capacity has been reserved beforehand. Only use this in performance
	 * critical code.
	 */
	void append_unchecked(const T &value) {
		this->append_unchecked_as(value);
	}
	void append_unchecked(T &&value) {
		this->append_unchecked_as(std::move(value));
	}
	template<typename... ForwardT> void append_unchecked_as(ForwardT &&...value) {
		ROSE_assert(end_ < capacity_end_);
		new (end_) T(std::forward<ForwardT>(value)...);
		end_++;
	}

	/**
	 * Insert the same element n times at the end of the vector.
	 * This might result in a reallocation internally.
	 */
	void append_n_times(const T &value, const size_t n) {
		this->reserve(this->size() + n);
		uninitialized_fill_n(end_, n, value);
		this->increase_size_by_unchecked(n);
	}

	/**
	 * Enlarges the size of the internal buffer that is considered to be initialized.
	 * This invokes undefined behavior when the new size is larger than the capacity.
	 * The method can be useful when you want to call constructors in the vector yourself.
	 * This should only be done in very rare cases and has to be justified every time.
	 */
	void increase_size_by_unchecked(const size_t n) noexcept {
		ROSE_assert(end_ + n <= capacity_end_);
		end_ += n;
	}

	/**
	 * Copy the elements of another array to the end of this vector.
	 *
	 * This can be used to emulate parts of std::vector::insert.
	 */
	void extend(Span<T> array) {
		this->extend(array.data(), array.size());
	}
	void extend(const T *start, size_t amount) {
		this->reserve(this->size() + amount);
		this->extend_unchecked(start, amount);
	}

	/**
	 * Adds all elements from the array that are not already in the vector. This is an expensive
	 * operation when the vector is large, but can be very cheap when it is known that the vector is
	 * small.
	 */
	void extend_non_duplicates(Span<T> array) {
		for (const T &value : array) {
			this->append_non_duplicates(value);
		}
	}

	/**
	 * Extend the vector without bounds checking. It is assumed that enough memory has been reserved
	 * beforehand. Only use this in performance critical code.
	 */
	void extend_unchecked(Span<T> array) {
		this->extend_unchecked(array.data(), array.size());
	}
	void extend_unchecked(const T *start, size_t amount) {
		ROSE_assert(begin_ + amount <= capacity_end_);
		uninitialized_copy_n(start, amount, end_);
		end_ += amount;
	}

	template<typename InputIt> void extend(InputIt first, InputIt last) {
		this->insert(this->end(), first, last);
	}

	/**
	 * Insert elements into the vector at the specified position. This has a running time of O(n)
	 * where n is the number of values that have to be moved. Undefined behavior is invoked when the
	 * insert position is out of bounds.
	 */
	void insert(const size_t insert_index, const T &value) {
		this->insert(insert_index, Span<T>(&value, 1));
	}
	void insert(const size_t insert_index, T &&value) {
		this->insert(insert_index, std::make_move_iterator(&value), std::make_move_iterator(&value + 1));
	}
	void insert(const size_t insert_index, Span<T> array) {
		this->insert(begin_ + insert_index, array.begin(), array.end());
	}
	template<typename InputIt> void insert(const T *insert_position, InputIt first, InputIt last) {
		const size_t insert_index = insert_position - begin_;
		this->insert(insert_index, first, last);
	}
	template<typename InputIt> void insert(const size_t insert_index, InputIt first, InputIt last) {
		ROSE_assert(insert_index <= this->size());

		const size_t insert_amount = std::distance(first, last);
		const size_t old_size = this->size();
		const size_t new_size = old_size + insert_amount;
		const size_t move_amount = old_size - insert_index;

		this->reserve(new_size);
		for (size_t i = 0; i < move_amount; i++) {
			const size_t src_index = insert_index + move_amount - i - 1;
			const size_t dst_index = new_size - i - 1;
			try {
				new (static_cast<void *>(begin_ + dst_index)) T(std::move(begin_[src_index]));
			}
			catch (...) {
				/* Destruct all values that have been moved already. */
				destruct_n(begin_ + dst_index + 1, i);
				end_ = begin_ + src_index + 1;
				throw;
			}
			begin_[src_index].~T();
		}

		try {
			std::uninitialized_copy_n(first, insert_amount, begin_ + insert_index);
		}
		catch (...) {
			/* Destruct all values that have been moved. */
			destruct_n(begin_ + new_size - move_amount, move_amount);
			end_ = begin_ + insert_index;
			throw;
		}
		end_ = begin_ + new_size;
	}

	/**
	 * Insert values at the beginning of the vector. The has to move all the other elements, so it
	 * has a linear running time.
	 */
	void prepend(const T &value) {
		this->insert(0, value);
	}
	void prepend(T &&value) {
		this->insert(0, std::move(value));
	}
	void prepend(Span<T> values) {
		this->insert(0, values);
	}
	template<typename InputIt> void prepend(InputIt first, InputIt last) {
		this->insert(0, first, last);
	}

	/**
	 * Return a reference to the nth last element.
	 * This invokes undefined behavior when the vector is too short.
	 */
	const T &last(const size_t n = 0) const {
		ROSE_assert(n < this->size());
		return *(end_ - 1 - n);
	}
	T &last(const size_t n = 0) {
		ROSE_assert(n < this->size());
		return *(end_ - 1 - n);
	}

	/**
	 * Return a reference to the first element in the vector.
	 * This invokes undefined behavior when the vector is empty.
	 */
	const T &first() const {
		ROSE_assert(this->size() > 0);
		return *begin_;
	}
	T &first() {
		ROSE_assert(this->size() > 0);
		return *begin_;
	}

	/**
	 * Return how many values are currently stored in the vector.
	 */
	size_t size() const {
		return end_ - begin_;
	}

	/**
	 * Returns true when the vector contains no elements, otherwise false.
	 *
	 * This is the same as std::vector::empty.
	 */
	bool is_empty() const {
		return begin_ == end_;
	}

	/**
	 * Destructs the last element and decreases the size by one. This invokes undefined behavior when
	 * the vector is empty.
	 */
	void remove_last() {
		ROSE_assert(!this->is_empty());
		end_--;
		end_->~T();
	}

	/**
	 * Remove the last element from the vector and return it. This invokes undefined behavior when
	 * the vector is empty.
	 *
	 * This is similar to std::vector::pop_back.
	 */
	T pop_last() {
		ROSE_assert(!this->is_empty());
		T value = std::move(*(end_ - 1));
		end_--;
		end_->~T();
		return value;
	}

	/**
	 * Delete any element in the vector. The empty space will be filled by the previously last
	 * element. This takes O(1) time.
	 */
	void remove_and_reorder(const size_t index) {
		ROSE_assert(index < this->size());
		T *element_to_remove = begin_ + index;
		T *last_element = end_ - 1;
		if (element_to_remove < last_element) {
			*element_to_remove = std::move(*last_element);
		}
		end_ = last_element;
		last_element->~T();
	}

	/**
	 * Finds the first occurrence of the value, removes it and copies the last element to the hole in
	 * the vector. This takes O(n) time.
	 */
	void remove_first_occurrence_and_reorder(const T &value) {
		const size_t index = this->first_index_of(value);
		this->remove_and_reorder(index);
	}

	/**
	 * Remove the element at the given index and move all values coming after it one towards the
	 * front. This takes O(n) time. If the order is not important, remove_and_reorder should be used
	 * instead.
	 *
	 * This is similar to std::vector::erase.
	 */
	void remove(const size_t index) {
		ROSE_assert(index < this->size());
		const size_t last_index = this->size() - 1;
		for (size_t i = index; i < last_index; i++) {
			begin_[i] = std::move(begin_[i + 1]);
		}
		begin_[last_index].~T();
		end_--;
	}

	/**
	 * Remove a contiguous chunk of elements and move all values coming after it towards the front.
	 * This takes O(n) time.
	 *
	 * This is similar to std::vector::erase.
	 */
	void remove(const size_t start_index, const size_t amount) {
		const size_t old_size = this->size();
		ROSE_assert(start_index + amount <= old_size);
		const size_t move_amount = old_size - start_index - amount;
		for (size_t i = 0; i < move_amount; i++) {
			begin_[start_index + i] = std::move(begin_[start_index + amount + i]);
		}
		destruct_n(end_ - amount, amount);
		end_ -= amount;
	}

	/**
	 * Remove all values for which the given predicate is true and return the number of values
	 * removed.
	 *
	 * This is similar to std::erase_if.
	 */
	template<typename Predicate> size_t remove_if(Predicate &&predicate) {
		const T *prev_end = this->end();
		end_ = std::remove_if(this->begin(), this->end(), predicate);
		destruct_n(end_, prev_end - end_);
		return prev_end - end_;
	}

	/**
	 * Do a linear search to find the value in the vector.
	 * When found, return the first index, otherwise return -1.
	 */
	size_t first_index_of_try(const T &value) const {
		for (const T *current = begin_; current != end_; current++) {
			if (*current == value) {
				return size_t(current - begin_);
			}
		}
		return not_found;
	}

	/**
	 * Do a linear search to find the value in the vector and return the found index. This invokes
	 * undefined behavior when the value is not in the vector.
	 */
	size_t first_index_of(const T &value) const {
		const size_t index = this->first_index_of_try(value);
		return index;
	}

	/**
	 * Do a linear search to see of the value is in the vector.
	 * Return true when it exists, otherwise false.
	 */
	bool contains(const T &value) const {
		return this->first_index_of_try(value) != -1;
	}

	/**
	 * Copies the given value to every element in the vector.
	 */
	void fill(const T &value) const {
		initialized_fill_n(begin_, this->size(), value);
	}

	/**
	 * Get access to the underlying array.
	 */
	T *data() {
		return begin_;
	}

	/**
	 * Get access to the underlying array.
	 */
	const T *data() const {
		return begin_;
	}

	T *begin() {
		return begin_;
	}
	T *end() {
		return end_;
	}

	const T *begin() const {
		return begin_;
	}
	const T *end() const {
		return end_;
	}

	std::reverse_iterator<T *> rbegin() {
		return std::reverse_iterator<T *>(this->end());
	}
	std::reverse_iterator<T *> rend() {
		return std::reverse_iterator<T *>(this->begin());
	}

	std::reverse_iterator<const T *> rbegin() const {
		return std::reverse_iterator<const T *>(this->end());
	}
	std::reverse_iterator<const T *> rend() const {
		return std::reverse_iterator<const T *>(this->begin());
	}

	/**
	 * Get the current capacity of the vector, i.e. the maximum number of elements the vector can
	 * hold, before it has to reallocate.
	 */
	size_t capacity() const {
		return capacity_end_ - begin_;
	}

	bool is_at_capacity() const {
		return end_ == capacity_end_;
	}

	/**
	 * Get an index range that makes looping over all indices more convenient and less error prone.
	 * Obviously, this should only be used when you actually need the index in the loop.
	 *
	 * Example:
	 *  for (size_t i : myvector.index_range()) {
	 *    do_something(i, my_vector[i]);
	 *  }
	 */
	IndexRange index_range() const {
		return IndexRange(this->size());
	}

	uint64_t hash() const {
		return this->as_span().hash();
	}

	static uint64_t hash_as(const Span<T> values) {
		return values.hash();
	}

	friend bool operator==(const Vector &a, const Vector &b) {
		return a.as_span() == b.as_span();
	}

	friend bool operator!=(const Vector &a, const Vector &b) {
		return !(a == b);
	}

private:
	bool is_inline() const {
		return begin_ == inline_buffer_;
	}

	void ensure_space_for_one() {
		if (end_ >= capacity_end_) {
			this->realloc_to_at_least(this->size() + 1);
		}
	}

	inline void realloc_to_at_least(const size_t min_capacity) {
		if (this->capacity() >= min_capacity) {
			return;
		}

		/* At least double the size of the previous allocation. Otherwise consecutive calls to grow can
		 * cause a reallocation every time even though min_capacity only increments. */
		const size_t min_new_capacity = this->capacity() * 2;

		const size_t new_capacity = std::max(min_capacity, min_new_capacity);
		const size_t size = this->size();

		T *new_array = static_cast<T *>(allocator_.allocate(size_t(new_capacity) * sizeof(T), alignof(T), AT));
		try {
			uninitialized_relocate_n(begin_, size, new_array);
		}
		catch (...) {
			allocator_.deallocate(new_array);
			throw;
		}

		if (!this->is_inline()) {
			allocator_.deallocate(begin_);
		}

		begin_ = new_array;
		end_ = begin_ + size;
		capacity_end_ = begin_ + new_capacity;
	}
};

/**
 * Same as a normal Vector, but does not use Rose's guarded allocator. This is useful when
 * allocating memory with static storage duration.
 */
template<typename T, int64_t InlineBufferCapacity = default_inline_buffer_capacity(sizeof(T))> using RawVector = Vector<T, InlineBufferCapacity, RawAllocator>;

}  // namespace rose

#endif	// LIB_VECTOR_HH
