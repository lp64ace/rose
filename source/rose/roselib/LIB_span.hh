#pragma once

#include <algorithm>
#include <array>
#include <string>
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
	using reference = T &;
	using const_reference = const T &;
	using iterator = const T *;
	using size_type = size_t;
	using ssize_type = ptrdiff_t;

protected:
	const T *data_ = nullptr;
	size_type size_ = 0;

public:
	constexpr Span() = default;

	constexpr Span(const T *start, size_type size) : data_(start), size_(size) {
	}

	template<typename U, ROSE_ENABLE_IF((is_span_convertible_pointer_v<U, T>))>
	constexpr Span(const U *start, size_type size) : data_(static_cast<const T *>(start)), size_(size) {
		ROSE_assert(size >= 0);
	}

	constexpr Span(const std::initializer_list<T> &list) : Span(list.begin(), static_cast<size_type>(list.size())) {
	}

	constexpr Span(const std::vector<T> &vector) : Span(vector.data(), static_cast<size_type>(vector.size())) {
	}

	template<size_type N> constexpr Span(const std::array<T, N> &array) : Span(array.data(), N) {
	}

	template<typename U, ROSE_ENABLE_IF((is_span_convertible_pointer_v<U, T>))>
	constexpr Span(Span<U> span) : data_(static_cast<const T *>(span.data())), size_(span.size()) {
	}

	constexpr Span slice(size_type start, size_type size) const {
		ROSE_assert(start + size <= size_ || size == 0);
		return Span(data_ + start, size);
	}

	constexpr Span slice(IndexRange range) const {
		return this->slice(range.start(), range.size());
	}

	constexpr Span slice_safe(const size_type start, const size_type size) const {
		const size_type new_size = size;
		if (start + size > size_) {
			new_size = static_cast<size_type>(std::max<ssize_type>(0, size_ - start));
		}
		return Span(data_ + start, new_size);
	}

	constexpr Span slice_safe(IndexRange range) const {
		return slice_safe(range.start(), range.size());
	}

	constexpr Span drop_front(size_type n) const {
		const size_type new_size = static_cast<size_type>(std::max<ssize_type>(0, size_ - n));
		return Span(data_ + n, new_size);
	}

	constexpr Span drop_back(size_type n) const {
		const size_type new_size = static_cast<size_type>(std::max<ssize_type>(0, size_ - n));
		return Span(data_, new_size);
	}

	constexpr Span take_front(size_type n) const {
		const size_type new_size = std::min<size_type>(size_, n);
		return Span(data_, new_size);
	}

	constexpr Span take_back(size_type n) const {
		const size_type new_size = std::min<size_type>(size_, n);
		return Span(data_ + size_ - new_size, new_size);
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

	constexpr const T &operator[](size_type index) const {
		ROSE_assert(index < size_);
		return data_[index];
	}

	constexpr size_type size() const {
		return size_;
	}

	constexpr bool is_empty() const {
		return size_ == 0;
	}

	constexpr size_type size_in_bytes() const {
		return sizeof(T) * size_;
	}

	const bool contains(const T &value) const {
		for (const T &element : *this) {
			if (element == value) {
				return true;
			}
		}
		return false;
	}

	const bool contains_ptr(const T *ptr) const {
		return (this->begin() <= ptr) && (ptr < this->end());
	}

	const size_type count(const T &value) const {
		size_type counter = 0;
		for (const T &element : *this) {
			if (element == value) {
				counter++;
			}
		}
		return counter;
	}

	constexpr const T &first() const {
		ROSE_assert(size_ > 0);
		return data_[0];
	}

	constexpr const T &last(const size_type n = 0) const {
		ROSE_assert(n < size_);
		return data_[size_ - 1 - n];
	}

	constexpr T get(size_type index, const T &fallback) const {
		if (index < size_) {
			return data_[index];
		}
		return fallback;
	}

	constexpr bool has_duplicates__linear_search() const {
		ROSE_assert(size_ < 1000);

		for (size_type i = 0; i < size_; i++) {
			for (size_type j = i + 1; j < size_; j++) {
				if (data_[i] == data_[j]) {
					return true;
				}
			}
		}
		return false;
	}

	constexpr bool intersects__linear_search(Span other) const {
		ROSE_assert(size_ < 1000);

		for (size_type i = 0; i < size_; i++) {
			if (other.contains(data_[i])) {
				return true;
			}
		}
		return false;
	}

	constexpr size_type first_index(const T &search_value) const {
		const ssize_type index = this->first_index_try(search_value);
		ROSE_assert(index >= 0);
		return static_cast<size_type>(index);
	}

	constexpr ssize_type first_index_try(const T &search_value) const {
		for (size_type i = 0; i < size_; i++) {
			if (data_[i] == search_value) {
				return static_cast<ssize_type>(i);
			}
		}
		return static_cast<ssize_type>(-1);
	}

	constexpr IndexRange index_range() const {
		return IndexRange(size_);
	}

	constexpr uint64_t hash() const {
		uint64_t hash = 0;
		for (const T &value : *this) {
			hash = hash * 33 ^ get_default_hash(value);
		}
		return hash;
	}

	template<typename NewT> Span<NewT> constexpr cast() const {
		ROSE_assert((size_ * sizeof(T)) % sizeof(NewT) == 0);
		size_type new_size = size_ * sizeof(T) / sizeof(NewT);
		return Span<NewT>(reinterpret_cast<const NewT *>(data_), new_size);
	}

	friend bool operator==(const Span<T> a, const Span<T> b) {
		if (a.size() != b.size()) {
			return false;
		}
		return std::equal(a.begin(), a.end(), b.begin());
	}

	friend bool operator!=(const Span<T> a, const Span<T> b) {
		return !(a == b);
	}
};

template<typename T> class MutableSpan {
public:
	using value_type = T;
	using pointer = T *;
	using const_pointer = const T *;
	using reference = T &;
	using const_reference = const T &;
	using iterator = const T *;
	using size_type = size_t;
	using ssize_type = ptrdiff_t;

protected:
	T *data_ = nullptr;
	size_type size_ = 0;

public:
	constexpr MutableSpan() = default;

	constexpr MutableSpan(T *start, const size_type size) : data_(start), size_(size) {
	}

	constexpr MutableSpan(std::vector<T> &vector) : MutableSpan(vector.data(), vector.size()) {
	}

	template<std::size_t N> constexpr MutableSpan(std::array<T, N> &array) : MutableSpan(array.data(), N) {
	}

	template<typename U, ROSE_ENABLE_IF((is_span_convertible_pointer_v<U, T>))>
	constexpr MutableSpan(MutableSpan<U> span) : data_(static_cast<T *>(span.data())), size_(span.size()) {
	}

	constexpr operator Span<T>() const {
		return Span<T>(data_, size_);
	}

	template<typename U, ROSE_ENABLE_IF((is_span_convertible_pointer_v<U, T>))> constexpr operator Span<U>() const {
		return Span<U>(static_cast<const U *>(data_), size_);
	}

	constexpr size_type size() const {
		return size_;
	}

	constexpr size_type size_in_bytes() const {
		return sizeof(T) * size_;
	}

	constexpr bool is_empty() const {
		return size_ == 0;
	}

	constexpr void fill(const T &value) const {
		initialized_fill_n(data_, size_, value);
	}

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

	constexpr T &operator[](const size_type index) const {
		ROSE_assert(index < size_);
		return data_[index];
	}

	constexpr MutableSpan slice(size_type start, size_type size) const {
		ROSE_assert(start + size <= size_ || size == 0);
		return MutableSpan(data_ + start, size);
	}

	constexpr MutableSpan slice(IndexRange range) const {
		return this->slice(range.start(), range.size());
	}

	constexpr MutableSpan slice_safe(const size_type start, const size_type size) const {
		const size_type new_size = size;
		if (start + size > size_) {
			new_size = static_cast<size_type>(std::max<ssize_type>(0, size_ - start));
		}
		return MutableSpan(data_ + start, new_size);
	}

	constexpr MutableSpan slice_safe(IndexRange range) const {
		return slice_safe(range.start(), range.size());
	}

	constexpr MutableSpan drop_front(size_type n) const {
		const size_type new_size = static_cast<size_type>(std::max<ssize_type>(0, size_ - n));
		return MutableSpan(data_ + n, new_size);
	}

	constexpr MutableSpan drop_back(size_type n) const {
		const size_type new_size = static_cast<size_type>(std::max<ssize_type>(0, size_ - n));
		return MutableSpan(data_, new_size);
	}

	constexpr MutableSpan take_front(size_type n) const {
		const size_type new_size = std::min<size_type>(size_, n);
		return MutableSpan(data_, new_size);
	}

	constexpr MutableSpan take_back(size_type n) const {
		const size_type new_size = std::min<size_type>(size_, n);
		return MutableSpan(data_ + size_ - new_size, new_size);
	}

	constexpr void reverse() const {
		for (const size_type i : IndexRange(size_ / 2)) {
			std::swap(data_[size_ - 1 - i], data_[i]);
		}
	}

	constexpr Span<T> as_span() const {
		return Span<T>(data_, size_);
	}

	constexpr IndexRange index_range() const {
		return IndexRange(size_);
	}

	constexpr T &first() const {
		return data_[0];
	}

	constexpr T &last(const size_type n = 0) const {
		ROSE_assert(n < size_);
		return data_[size_ - 1 - n];
	}

	const bool contains(const T &value) const {
		for (const T &element : *this) {
			if (element == value) {
				return true;
			}
		}
		return false;
	}

	const bool contains_ptr(const T *ptr) const {
		return (this->begin() <= ptr) && (ptr < this->end());
	}

	const size_type count(const T &value) const {
		size_type counter = 0;
		for (const T &element : *this) {
			if (element == value) {
				counter++;
			}
		}
		return counter;
	}

	constexpr void copy_from(Span<T> values) const {
		ROSE_assert(size_ == values.size());
		initialized_copy_n(values.data(), size_, data_);
	}

	template<typename NewT> constexpr MutableSpan<NewT> cast() const {
		ROSE_assert((size_ * sizeof(T)) % sizeof(NewT) == 0);
		size_type new_size = size_ * sizeof(T) / sizeof(NewT);
		return MutableSpan<NewT>(reinterpret_cast<NewT *>(data_), new_size);
	}
};

}  // namespace rose
