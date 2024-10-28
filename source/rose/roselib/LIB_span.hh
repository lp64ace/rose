#ifndef LIB_SPAN_HH
#define LIB_SPAN_HH

/**
 * An `rose::Span<T>` references an array that is owned by someone else. It is just a
 * pointer and a size. Since the memory is not owned, Span should not be used to transfer
 * ownership. The array cannot be modified through the Span. However, if T is a non-const
 * pointer, the pointed-to elements can be modified.
 *
 * There is also `rose::MutableSpan<T>`. It is mostly the same as Span, but allows the
 * array to be modified.
 *
 * A (Mutable)Span can refer to data owned by many different data structures including
 * rose::Vector, rose::Array, rose::VectorSet, std::vector, std::array, std::string,
 * std::initializer_list and c-style array.
 *
 * `rose::Span` is very similar to `std::span` (C++20). However, there are a few differences:
 * - `rose::Span` is const by default. This is to avoid making things mutable when they don't
 *   have to be. To get a non-const span, you need to use `rose::MutableSpan`. Below is a list
 *   of const-behavior-equivalent pairs of data structures:
 *   - std::span<int>                <==>  rose::MutableSpan<int>
 *   - std::span<const int>          <==>  rose::Span<int>
 *   - std::span<int *>              <==>  rose::MutableSpan<int *>
 *   - std::span<const int *>        <==>  rose::MutableSpan<const int *>
 *   - std::span<int * const>        <==>  rose::Span<int *>
 *   - std::span<const int * const>  <==>  rose::Span<const int *>
 * - `rose::Span` always has a dynamic extent, while `std::span` can have a size that is
 *   determined at compile time. I did not have a use case for that yet. If we need it, we can
 *   decide to add this functionality to `rose::Span` or introduce a new type like
 *   `rose::FixedSpan<T, N>`.
 *
 * `rose::Span<T>` should be your default choice when you have to pass a read-only array
 * into a function. It is better than passing a `const Vector &`, because then the function only
 * works for vectors and not for e.g. arrays. Using Span as function parameter makes it usable
 * in more contexts, better expresses the intent and does not sacrifice performance. It is also
 * better than passing a raw pointer and size separately, because it is more convenient and safe.
 *
 * `rose::MutableSpan<T>` can be used when a function is supposed to return an array, the
 * size of which is known before the function is called. One advantage of this approach is that the
 * caller is responsible for allocation and deallocation. Furthermore, the function can focus on
 * its task, without having to worry about memory allocation. Alternatively, a function could
 * return an Array or Vector.
 *
 * NOTE: When a function has a MutableSpan<T> output parameter and T is not a trivial type,
 * then the function has to specify whether the referenced array is expected to be initialized or
 * not.
 *
 * Since the arrays are only referenced, it is generally unsafe to store a Span. When you
 * store one, you should know who owns the memory.
 *
 * Instances of Span and MutableSpan are small and should be passed by value.
 */

#include <algorithm>
#include <array>
#include <vector>

#include "LIB_index_range.hh"
#include "LIB_memory_utils.hh"
#include "LIB_utildefines.h"

namespace rose {

template<typename T> uint64_t get_default_hash(const T &v);

template<typename T> class Span {
public:
	using value_type = T;
	using pointer = T *;
	using const_pointer = const T *;
	using const_reference = const T &;
	using iterator = const T *;
	using size_type = size_t;
	
	/* Similar to string::npos. */
	static constexpr size_t not_found = -1;
	
protected:
	const_pointer data_ = nullptr;
	size_type size_ = 0;
	
public:
	constexpr Span() = default;
	constexpr Span(const T *start, size_t size) : data_(start), size_(size) {
		ROSE_assert(size >= 0);
	}
	template<typename U, ROSE_ENABLE_IF((is_span_convertible_pointer_v<U, T>))>
	constexpr Span(const U *start, size_t size) : data_(static_cast<const T *>(start)), size_(size) {
		ROSE_assert(size >= 0);
	}
	/**
	 * Reference an initializer_list. Note that the data in the initializer_list is only valid until
	 * the expression containing it is fully computed.
	 *
	 * Do:
	 *  call_function_with_array({1, 2, 3, 4});
	 *
	 * Don't:
	 *  Span<int> span = {1, 2, 3, 4};
	 *  call_function_with_array(span);
	 */
	constexpr Span(const std::initializer_list<T> &list) : Span(list.begin(), list.size()) {
	}
	constexpr Span(const std::vector<T>& vector) : Span(vector.data(), vector.size()) {
	}
	template<std::size_t N> constexpr Span(const std::array<T, N> &array) : Span(array.data(), N) {
	}
	/**
	 * Support implicit conversions like the one below:
	 *   Span<T *> -> Span<const T *>
	 */
	template<typename U, ROSE_ENABLE_IF((is_span_convertible_pointer_v<U, T>))>
	constexpr Span(Span<U> span) : data_(static_cast<const T *>(span.data())), size_(span.size()) {
	}
	
	constexpr Span slice(size_t start, size_t size) const {
		ROSE_assert(0 <= start && start <= size_);
		ROSE_assert((0 <= start + size && start + size <= size_) || size == 0);
		return Span(data_ + start, size);
	}
	constexpr Span slice(IndexRange range) const {
		return this->slice(range.start(), range.size());
	}
	
	constexpr Span slice_safe(size_t start, size_t size) const {
		if(start + size <= size_ || size == 0) {
			return Span(data_ + start, size);
		}
		else if(start <= size_) {
			return Span(data_ + start, size_ - start);
		}
		return Span(data_ + start, 0);
	}
	constexpr Span slice_safe(IndexRange range) const {
		return this->slice_safe(range.start(), range.size());
	}
	
	/** Returns a new Span with n elements removed from the beginning. */
	constexpr Span drop_front(size_t n) const {
		return (n <= size_) ? Span(data_ + n, size_ - n) : Span(data_ + size_, 0);
	}
	/** Returns a new Span with n elements removed from the end. */
	constexpr Span drop_back(size_t n) const {
		return (n <= size_) ? Span(data_, size_ - n) : Span(data_, 0);
	}
	/** Returns a new Span that only contains the first n elements. */
	constexpr Span take_front(size_t n) const {
		return (n <= size_) ? Span(data_, n) : Span(data_, size_);
	}
	/** Returns a new Span that only contains the last n elements. */
	constexpr Span take_back(size_t n) const {
		return (n <= size_) ? Span(data_ + size_ - n, n) :  Span(data_, size_);
	}
	
	constexpr const T *data() const {
		return data_;
	}
	constexpr const T *begin() const {
		return data_;
	}
	constexpr const T *end() const {
		return data_ + size_;
	}
	
	constexpr std::reverse_iterator<const T *> rbegin() const {
		return std::reverse_iterator<const T *>(this->end());
	}
	constexpr std::reverse_iterator<const T *> rend() const {
		return std::reverse_iterator<const T *>(this->begin());
	}
	
	constexpr const T &operator[](size_t index) const {
		ROSE_assert(index <= size_);
		return data_[index];
	}
	
	constexpr size_t size() const {
		return size_;
	}
	constexpr bool is_empty() const {
		return size_ == 0;
	}
	constexpr size_t size_in_bytes() const {
		return sizeof(T) * size_;
	}
	
	constexpr bool contains(const T& value) const {
		for(const T &e : *this) {
			if(e == value) {
				return true;
			}
		}
		return false;
	}
	constexpr bool contains_ptr(const T *ptr) const {
		return this->begin() <= ptr && ptr < this->end();
	}
	
	constexpr size_t count(const T &value) const {
		size_t counter = 0;
		for(const T &e : *this) {
			if(e == value) {
				counter++;
			}
		}
		return counter;
	}
	
	constexpr const T &first() const {
		ROSE_assert(size_ > 0);
		return data_[0];
	}
	constexpr const T &last(const size_t n) const {
		ROSE_assert(n < size_);
		return data_[size_ - 1 - n];
	}
	
	/** Check if the array contains duplicates. Time complexity is O(n^2). */
	constexpr bool has_duplicates() const {
		ROSE_assert(size_ < 1000);
		
		for(size_t i = 0; i < size_; i++) {
			const T &value = data_[i];
			for(size_t j = i + 1; j < size_; j++) {
				if(value == data_[j]) {
					return true;
				}
			}
		}
		return false;
	}
	/** Returns true when this and the other array have an element in common. Time complexity is O(n^2). */
	constexpr bool intersects(Span other) const {
		ROSE_assert(size_ < 1000);
		
		for(size_t i = 0; i < size_; i++) {
			const T &value = data_[i];
			if(other.contains(value)) {
				return true;
			}
		}
		return false;
	}
	
	constexpr size_t first_index(const T &value) const {
		const size_t index = this->first_index_try(value);
		ROSE_assert(index != not_found);
		return index;
	}
	constexpr size_t first_index_try(const T &value) const {
		for(size_t index = 0; index < size_; index++) {
			if(data_[index] == value) {
				return index;
			}
		}
		return not_found;
	}
	
	constexpr IndexRange index_range() const {
		return IndexRange(size_);
	}
	
	constexpr uint64_t hash() const {
		uint64_t hash = 0;
		for(const T &value : *this) {
			hash = hash * 33 ^ get_default_hash(value);
		}
		return hash;
	}
	
	template<typename NewT> Span<NewT> constexpr cast() const {
		ROSE_assert((size_ * sizeof(T)) & sizeof(NewT) == 0);
		size_t new_size = size_ * sizeof(T) / sizeof(NewT);
		return Span<NewT>(reinterpret_cast<const NewT *>(data_), new_size);
	}
	
	friend bool operator ==(const Span<T> a, const Span<T> b) {
		if(a.size() != b.size()) {
			return false;
		}
		return std::equal(a.begin(), a.end(), b.begin());
	}
	friend bool operator !=(const Span<T> a, const Span<T> b) {
		return !(a == b);
	}
};

template<typename T> class MutableSpan {
public:
	using value_type = T;
	using pointer = T *;
	using const_pointer = const T *;
	using const_reference = const T &;
	using iterator = const T *;
	using size_type = size_t;
	
protected:
	pointer data_ = nullptr;
	size_type size_ = 0;
	
public:
	constexpr MutableSpan() = default;
	constexpr MutableSpan(T *start, size_t size) : data_(start), size_(size) {
		ROSE_assert(size >= 0);
	}
	template<typename U, ROSE_ENABLE_IF((is_span_convertible_pointer_v<U, T>))>
	constexpr MutableSpan(U *start, size_t size) : data_(static_cast<T *>(start)), size_(size) {
		ROSE_assert(size >= 0);
	}
	
	constexpr MutableSpan(std::vector<T>& vector) : MutableSpan(vector.data(), vector.size()) {
	}
	template<std::size_t N> constexpr MutableSpan(std::array<T, N> &array) : MutableSpan(array.data(), N) {
	}
	/**
	 * Support implicit conversions like the one below:
	 *   MutableSpan<T *> -> Span<const T *>
	 */
	template<typename U, ROSE_ENABLE_IF((is_span_convertible_pointer_v<U, T>))>
	constexpr MutableSpan(MutableSpan<U> span) : data_(static_cast<T *>(span.data())), size_(span.size()) {
	}
	
	constexpr operator Span<T>() const {
		return Span<T>(data_, size_);
	}
	template<typename U, ROSE_ENABLE_IF((is_span_convertible_pointer_v<T, U>))>
	constexpr operator Span<U>() const {
		return Span<U>(static_cast<const U *>(data_), size_);
	}
	
	constexpr size_t size() const {
		return size_;
	}
	constexpr bool is_empty() const {
		return size_ == 0;
	}
	constexpr size_t size_in_bytes() const {
		return sizeof(T) * size_;
	}
	
	constexpr void fill(const T &value) const {
		initialized_fill_n(data_, size_, value);
	}
	
	/**
	 * Replace a subset of all elements with the given value. This invokes undefined behavior when
	 * one of the indices is out of bounds.
	 */
	template<typename IndexT> constexpr void fill_indices(Span<IndexT> indices, const T &value) const {
		static_assert(std::is_integral_v<IndexT>);
		for (IndexT i : indices) {
			ROSE_assert(i < size_);
			data_[i] = value;
		}
	}
	
	constexpr T *data() const {
		return data_;
	}
	constexpr T *begin() const {
		return data_;
	}
	constexpr T *end() const {
		return data_ + size_;
	}
	
	constexpr std::reverse_iterator<T *> rbegin() const {
		return std::reverse_iterator<T *>(this->end());
	}
	constexpr std::reverse_iterator<T *> rend() const {
		return std::reverse_iterator<T *>(this->begin());
	}
	
	constexpr T &operator[](const size_t index) const {
		ROSE_assert(index < size_);
		return data_[index];
	}
	
	constexpr MutableSpan slice(size_t start, size_t size) const {
		ROSE_assert(0 <= start && start <= size_);
		ROSE_assert((0 <= start + size && start + size <= size_) || size == 0);
		return MutableSpan(data_ + start, size);
	}
	constexpr MutableSpan slice(IndexRange range) const {
		return this->slice(range.start(), range.size());
	}
	
	constexpr MutableSpan slice_safe(size_t start, size_t size) const {
		if(start + size <= size_ || size == 0) {
			return MutableSpan(data_ + start, size);
		}
		else if(start <= size_) {
			return MutableSpan(data_ + start, size_ - start);
		}
		return MutableSpan(data_ + start, 0);
	}
	constexpr MutableSpan slice_safe(IndexRange range) const {
		return this->slice_safe(range.start(), range.size());
	}
	
	/** Returns a new MutableSpan with n elements removed from the beginning. */
	constexpr MutableSpan drop_front(size_t n) const {
		return (n <= size_) ? MutableSpan(data_ + n, size_ - n) : MutableSpan(data_ + size_, 0);
	}
	/** Returns a new MutableSpan with n elements removed from the end. */
	constexpr MutableSpan drop_back(size_t n) const {
		return (n <= size_) ? MutableSpan(data_, size_ - n) : MutableSpan(data_, 0);
	}
	/** Returns a new MutableSpan that only contains the first n elements. */
	constexpr MutableSpan take_front(size_t n) const {
		return (n <= size_) ? MutableSpan(data_, n) : MutableSpan(data_, size_);
	}
	/** Returns a new MutableSpan that only contains the last n elements. */
	constexpr MutableSpan take_back(size_t n) const {
		return (n <= size_) ? MutableSpan(data_ + size_ - n, n) :  MutableSpan(data_, size_);
	}
	
	constexpr void reverse() const {
		for(const size_t i : IndexRange(size_ / 2)) {
			std::swap(data_[size_ - 1 - i], data_[i]);
		}
	}
	
	/**
	 * Returns an (immutable) Span that references the same array. This is usually not needed,
	 * due to implicit conversions. However, sometimes automatic type deduction needs some help.
	 */
	constexpr Span<T> as_span() const {
		return Span<T>(data_, size_);
	}
	/**
	 * Utility to make it more convenient to iterate over all indices that can be used with this
	 * array.
	 */
	constexpr IndexRange index_range() const {
		return IndexRange(size_);
	}
	
	constexpr T &first() const {
		ROSE_assert(size_ > 0);
		return data_[0];
	}
	constexpr T &last(const size_t n) const {
		ROSE_assert(n < size_);
		return data_[size_ - 1 - n];
	}
	
	constexpr size_t count(const T &value) const {
		size_t counter = 0;
		for(const T &e : *this) {
			if(e == value) {
				counter++;
			}
		}
		return counter;
	}
	
	/**
	 * Does a constant time check to see if the pointer points to a value in the referenced array.
	 * Return true if it is, otherwise false.
	 */
	constexpr bool contains_ptr(const T *ptr) const {
		return (this->begin() <= ptr) && (ptr < this->end());
	}
	/**
	 * Copy all values from another span into this span. This invokes undefined behavior when the
	 * destination contains uninitialized data and T is not trivially copy constructible.
	 * The size of both spans is expected to be the same.
	 */
	constexpr void copy_from(Span<T> values) const {
		ROSE_assert(size_ == values.size());
		initialized_copy_n(values.data(), size_, data_);
	}
	
	/**
	 * Returns a new span to the same underlying memory buffer. No conversions are done.
	 * The caller is responsible for making sure that the type cast is valid.
	 */
	template<typename NewT> constexpr MutableSpan<NewT> cast() const {
		ROSE_assert((size_ * sizeof(T)) % sizeof(NewT) == 0);
		int64_t new_size = size_ * sizeof(T) / sizeof(NewT);
		return MutableSpan<NewT>(reinterpret_cast<NewT *>(data_), new_size);
	}
};

} // namespace rose

#endif // LIB_SPAN_HH
