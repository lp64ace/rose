#pragma once

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "LIB_allocator.hh"
#include "LIB_index_range.hh"
#include "LIB_memory_utils.hh"
#include "LIB_span.hh"
#include "LIB_utildefines.h"

#include "MEM_alloc.h"

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
	using iterator = T *;
	using const_iterator = const T *;
	using size_type = size_t;
	using ssize_type = ptrdiff_t;

private:
	T *begin_;
	T *end_;
	T *capacity_end_;

	/** Used for allocations when the inline buffer is too small. */
	ROSE_NO_UNIQUE_ADDRESS Allocator allocator_;

	/** A placeholder buffer that will remain unintialized until it is used. */
	ROSE_NO_UNIQUE_ADDRESS TypedBuffer<T, InlineBufferCapacity> inline_buffer_;

	/**
	 * Be a friend with other vector instantiations. This is necessary to implement some memory
	 * management logic.
	 */
	template<typename OtherT, size_t OtherInlineBufferCapacity, typename OtherAllocator> friend class Vector;

	static constexpr bool is_nothrow_move_constructible() {
		if constexpr (InlineBufferCapacity == 0) {
			return true;
		}
		else {
			return std::is_nothrow_move_constructible_v<T>;
		}
	}

public:
	Vector(Allocator allocator = {}) noexcept : allocator_(allocator) {
		end_ = begin_ = inline_buffer_;
		capacity_end_ = begin_ + InlineBufferCapacity;
	}

	Vector(NoExceptConstructor, Allocator allocator = {}) noexcept : Vector(allocator) {
	}

	explicit Vector(size_type size, Allocator allocator = {}) : Vector(NoExceptConstructor(), allocator) {
		this->resize(size);
	}

	Vector(size_type size, const T &value, Allocator allocator = {}) : Vector(NoExceptConstructor(), allocator) {
		this->resize(size, value);
	}

	template<typename U, ROSE_ENABLE_IF((std::is_convertible_v<U, T>))> Vector(Span<U> values, Allocator allocator = {}) : Vector(NoExceptConstructor(), allocator) {
		const size_type size = values.size();
		this->reserve(size);
		uninitialized_convert_n<U, T>(values.data(), size, begin_);
		this->increase_size_by_unchecked(size);
	}

	template<typename U, ROSE_ENABLE_IF((std::is_convertible_v<U, T>))> explicit Vector(MutableSpan<U> values, Allocator allocator = {}) : Vector(values.as_span(), allocator) {
	}

	template<typename U, ROSE_ENABLE_IF((std::is_convertible_v<U, T>))> Vector(const std::initializer_list<U> &values) : Vector(Span<U>(values)) {
	}

	Vector(const std::initializer_list<T> &values) : Vector(Span<T>(values)) {
	}

	template<typename U, size_t N, ROSE_ENABLE_IF((std::is_convertible_v<U, T>))> Vector(const std::array<U, N> &values) : Vector(Span(values)) {
	}
	/* This constructor should not be called with e.g. Vector(3, 10), because that is
	 * expected to produce the vector (10, 10, 10). */
	template<typename InputIt, ROSE_ENABLE_IF((!std::is_convertible_v<InputIt, int>))> Vector(InputIt first, InputIt last, Allocator allocator = {}) : Vector(NoExceptConstructor(), allocator) {
		for (InputIt current = first; current != last; ++current) {
			this->append(*current);
		}
	}

	Vector(const Vector &other) : Vector(other.as_span(), other.allocator_) {
	}

	template<size_type OtherInlineBufferCapacity> Vector(const Vector<T, OtherInlineBufferCapacity, Allocator> &other) : Vector(other.as_span(), other.allocator_) {
	}

	template<int64_t OtherInlineBufferCapacity> Vector(Vector<T, OtherInlineBufferCapacity, Allocator> &&other) noexcept(is_nothrow_move_constructible()) : Vector(NoExceptConstructor(), other.allocator_) {
		const int64_t size = other.size();

		if (other.is_inline()) {
			if (size <= InlineBufferCapacity) {
				/* Copy between inline buffers. */
				uninitialized_relocate_n(other.begin_, size, begin_);
				end_ = begin_ + size;
			}
			else {
				/* Copy from inline buffer to newly allocated buffer. */
				const int64_t capacity = size;
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

	const T &operator[](size_type index) const {
		ROSE_assert(index < this->size());
		return begin_[index];
	}

	T &operator[](size_type index) {
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

	void reserve(const size_type min_capacity) {
		if (min_capacity > this->capacity()) {
			this->realloc_to_at_least(min_capacity);
		}
	}

	void resize(const size_type new_size) {
		const size_type old_size = this->size();
		if (new_size > old_size) {
			this->reserve(new_size);
			default_construct_n(begin_ + old_size, new_size - old_size);
		}
		else {
			destruct_n(begin_ + new_size, old_size - new_size);
		}
		end_ = begin_ + new_size;
	}

	void resize(const size_type new_size, const T &value) {
		const size_type old_size = this->size();
		if (new_size > old_size) {
			this->reserve(new_size);
			uninitialized_fill_n(begin_ + old_size, new_size - old_size, value);
		}
		else {
			destruct_n(begin_ + new_size, old_size - new_size);
		}
		end_ = begin_ + new_size;
	}

	void reinitialize(const size_type new_size) {
		this->clear();
		this->reisze(new_size);
	}

	void clear() {
		destruct_n(begin_, this->size());
		end_ = begin_;
	}

	void clear_and_shrink() {
		destruct_n(begin_, this->size());
		if (!this->is_inline()) {
			allocator_.deallocate(begin_);
		}

		begin_ = inline_buffer_;
		end_ = begin_;
		capacity_end_ = begin_ + InlineBufferCapacity;
	}

	void append(const T &value) {
		this->append_as(value);
	}
	void append(T &&value) {
		this->append_as(std::move(value));
	}
	template<typename... ForwardValue> void append_as(ForwardValue &&...value) {
		this->ensure_space_for_one();
		this->append_unchecked_as(std::forward<ForwardValue>(value)...);
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

	size_type append_and_get_index(const T &value) {
		return this->append_and_get_index_as(value);
	}
	size_type append_and_get_index(T &&value) {
		return this->append_and_get_index_as(std::move(value));
	}
	template<typename... ForwardValue> size_type append_and_get_index_as(ForwardValue &&...value) {
		const size_type index = this->size();
		this->append_as(std::forward<ForwardValue>(value)...);
		return index;
	}

	void append_non_duplicates(const T &value) {
		if (!this->contains(value)) {
			this->append(value);
		}
	}

	void append_n_times(const T &value, const size_type n) {
		this->reserve(this->size() + n);
		uninitialized_fill_n(end_, n, value);
		this->increase_size_by_unchecked(n);
	}

	void increase_size_by_unchecked(const size_type n) noexcept {
		ROSE_assert(end_ + n <= capacity_end_);
		end_ += n;
	}

	void extend(Span<T> array) {
		this->extend(array.data(), array.size());
	}
	void extend(const T *start, size_type amount) {
		this->reserve(this->size() + amount);
		this->extend_unchecked(start, amount);
	}

	void extend_non_duplicates(Span<T> array) {
		for (const T &value : array) {
			this->append_non_duplicates(value);
		}
	}

	void extend_unchecked(Span<T> array) {
		this->extend_unchecked(array.data(), array.size());
	}
	void extend_unchecked(const T *start, size_type amount) {
		ROSE_assert(begin_ + amount <= capacity_end_);
		uninitialized_copy_n(start, amount, end_);
		end_ += amount;
	}
	template<typename InputIt> void extend(InputIt first, InputIt last) {
		this->insert(this->end(), first, last);
	}

	void insert(const size_type insert_index, const T &value) {
		this->insert(insert_index, Span<T>(&value, 1));
	}
	void insert(const size_type insert_index, T &&value) {
		this->insert(insert_index, std::make_move_iterator(&value), std::make_move_iterator(&value + 1));
	}
	void insert(const size_type insert_index, Span<T> array) {
		this->insert(begin_ + insert_index, array.begin(), array.end());
	}
	template<typename InputIt> void insert(const T *insert_position, InputIt first, InputIt last) {
		const size_type insert_index = insert_position - begin_;
		this->insert(insert_index, first, last);
	}
	template<typename InputIt> void insert(const size_type insert_index, InputIt first, InputIt last) {
		ROSE_assert(insert_index >= 0);
		ROSE_assert(insert_index <= this->size());

		const size_type insert_amount = std::distance(first, last);
		const size_type old_size = this->size();
		const size_type new_size = old_size + insert_amount;
		const size_type move_amount = old_size - insert_index;

		this->reserve(new_size);
		for (size_type i = 0; i < move_amount; i++) {
			const size_type src_index = insert_index + move_amount - i - 1;
			const size_type dst_index = new_size - i - 1;
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

	const T &last(const size_type n = 0) const {
		ROSE_assert(n < this->size());
		return *(end_ - 1 - n);
	}
	T &last(const size_type n = 0) {
		ROSE_assert(n < this->size());
		return *(end_ - 1 - n);
	}

	const T &first() const {
		ROSE_assert(this->size() > 0);
		return *begin_;
	}
	T &first() {
		ROSE_assert(this->size() > 0);
		return *begin_;
	}

	size_type size() const {
		const size_type current_size = static_cast<size_type>(end_ - begin_);
		return current_size;
	}

	bool is_empty() const {
		return begin_ == end_;
	}

	void remove_last() {
		ROSE_assert(!this->is_empty());
		end_--;
		end_->~T();
	}

	T pop_last() {
		ROSE_assert(!this->is_empty());
		T value = std::move(*(end_ - 1));
		end_--;
		end_->~T();
		return value;
	}

	void remove_and_reorder(const size_type index) {
		T *element_to_remove = begin_ + index;
		T *last_element = end_ - 1;
		if (element_to_remove < last_element) {
			*element_to_remove = std::move(*last_element);
		}
		end_ = last_element;
		last_element->~T();
	}

	void remove_first_occurrence_and_reorder(const T &value) {
		const ssize_type index = this->first_index_of(value);
		if (0 <= index && index < this->size()) {
			this->remove_and_reorder(static_cast<size_type>(index));
		}
	}

	void remove(const size_type index) {
		const size_type last_index = this->size() - 1;
		for (size_type i = index; i < last_index; i++) {
			begin_[i] = std::move(begin_[i + 1]);
		}
		begin_[last_index].~T();
		end_--;
	}

	void remove(const size_type index, const size_type length) {
		ROSE_assert(index + length <= this->size());
		const size_type old_size = this->size();
		for (size_type i = 0; i < move_amount; i++) {
			begin_[start_index + i] = std::move(begin_[start_index + amount + i]);
		}
		destruct_n(end_ - amount, amount);
		end_ -= amount;
	}

	template<typename Predicate> size_type remove_if(Predicate &&predicate) {
		const T *prev_end = this->end();
		end_ = std::remove_if(this->begin(), this->end(), predicate);
		destruct_n(end_, prev_end - end_);
		return static_cast<size_type>(prev_end - end_);
	}

	ssize_type first_index_of_try(const T &value) const {
		for (const T *current = begin_; current != end_; current++) {
			if (*current == value) {
				return static_cast<ssize_type>(current - begin_);
			}
		}
		return -1;
	}

	size_type first_index_of(const T &value) const {
		const ssize_type index = this->first_index_of_try(value);
		ROSE_assert(index >= 0);
		return static_cast<size_type>(index);
	}

	bool contains(const T &value) const {
		return this->first_index_of_try(value) != -1;
	}

	void fill(const T &value) {
		initialized_fill_n(begin_, this->size(), value);
	}

	T *data() {
		return begin_;
	}

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
	size_type capacity() const {
		return static_cast<size_type>(capacity_end_ - begin_);
	}

	bool is_at_capacity() const {
		return end_ == capacity_end_;
	}

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

	void realloc_to_at_least(const size_type min_capacity) {
		if (this->capacity() >= min_capacity) {
			return;
		}

		/* At least double the size of the previous allocation. Otherwise consecutive calls to grow can
		 * cause a reallocation every time even though min_capacity only increments. */
		const size_type min_new_capacity = this->capacity() * 2;

		const size_type new_capacity = std::max(min_capacity, min_new_capacity);
		const size_type size = this->size();

		T *new_array = static_cast<T *>(allocator_.allocate(size_t(new_capacity) * sizeof(T), alignof(T), "VectorBuffer"));
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

}  // namespace rose
