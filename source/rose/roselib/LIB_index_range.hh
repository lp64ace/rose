#ifndef LIB_INDEX_RANGE_HH
#define LIB_INDEX_RANGE_HH

#include "LIB_assert.h"
#include "LIB_random_access_iterator_mixin.hh"

#include <algorithm>

namespace rose {

template<typename T> class Span;

class IndexRange {
private:
	size_t start_ = 0;
	size_t size_ = 0;

public:
	constexpr IndexRange() = default;
	constexpr IndexRange(size_t size) : start_(0), size_(size) {
	}
	constexpr IndexRange(size_t start, size_t size) : start_(start), size_(size) {
	}

	constexpr static IndexRange from_begin_size(const size_t begin, const size_t size) {
		return IndexRange(begin, size);
	}
	constexpr static IndexRange from_begin_end(const size_t begin, const size_t end) {
		ROSE_assert(begin <= end);
		return IndexRange(begin, end - begin);
	}
	constexpr static IndexRange from_begin_end_inclusive(const size_t first, const size_t last) {
		ROSE_assert(first <= last);
		return IndexRange(first, last - first + 1);
	}
	constexpr static IndexRange from_end_size(const size_t end, const size_t size) {
		ROSE_assert(size <= end);
		return IndexRange(end - size, size);
	}
	constexpr static IndexRange from_single(const size_t index) {
		return IndexRange(index, 1);
	}

	class Iterator : public iterator::RandomAccessIteratorMixin<Iterator> {
	public:
		using value_type = size_t;
		using pointer = const size_t *;
		using reference = size_t;

	private:
		size_t current_;

	public:
		constexpr explicit Iterator(size_t current) : current_(current) {
		}

		constexpr size_t operator*() const {
			return current_;
		}

		const size_t &iter_prop() const {
			return current_;
		}
	};

	constexpr Iterator begin() const {
		return Iterator(start_);
	}
	constexpr Iterator end() const {
		return Iterator(start_ + size_);
	}

	constexpr size_t operator[](const size_t index) const {
		ROSE_assert(index <= this->size());
		return start_ + index;
	}

	constexpr friend bool operator==(IndexRange a, IndexRange b) {
		return (a.size_ == b.size_) && (a.start_ == b.start_ || a.size_ == 0);
	}
	constexpr friend bool operator!=(IndexRange a, IndexRange b) {
		return !(a == b);
	}

	constexpr size_t start() const {
		return start_;
	}
	constexpr size_t size() const {
		return size_;
	}

	constexpr IndexRange index_range() const {
		return IndexRange(size_);
	}

	constexpr bool is_empty() const {
		return size_ == 0;
	}

	constexpr size_t first() const {
		ROSE_assert(size_ > 0);
		return start_;
	}
	constexpr size_t last(const size_t n = 0) const {
		ROSE_assert(size_ > n);
		return start_ + size_ - 1 - n;
	}

	constexpr bool contains(size_t value) const {
		return start_ <= value && value < start_ + size_;
	}

	constexpr IndexRange slice(size_t start, size_t size) const {
		ROSE_assert(start >= 0);
		ROSE_assert(size >= 0);

		size_t new_start = start_ + start;
		ROSE_assert(new_start + size <= start_ + size_ || size == 0);
		return IndexRange(new_start, size);
	}
	constexpr IndexRange slice(IndexRange range) const {
		return this->slice(range.start(), range.size());
	}

	constexpr IndexRange intersect(IndexRange other) const {
		const size_t old_end = start_ + size_;
		const size_t new_start = std::min(old_end, std::max(start_, other.start_));
		const size_t new_end = std::max(new_start, std::min(old_end, other.start_ + other.size_));
		return IndexRange(new_start, new_end - new_start);
	}

	constexpr IndexRange drop_front(size_t n) const {
		return (n <= size_) ? IndexRange(start_ + n, size_ - n) : IndexRange(start_ + n, 0);
	}
	constexpr IndexRange drop_back(size_t n) const {
		return (n <= size_) ? IndexRange(start_, size_ - n) : IndexRange(start_, 0);
	}

	constexpr IndexRange take_front(size_t n) const {
		return (n <= size_) ? IndexRange(start_, n) : IndexRange(start_, size_);
	}
	constexpr IndexRange take_back(size_t n) const {
		return (n <= size_) ? IndexRange(start_ + size_ - n, n) : IndexRange(start_, size_);
	}

	constexpr IndexRange shift(size_t n) const {
		return IndexRange(start_ + n, size_);
	}
};

}  // namespace rose

#endif	// LIB_INDEX_RANGE_HH
