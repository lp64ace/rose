#pragma once

#include <algorithm>
#include <cmath>
#include <iosfwd>

#include "LIB_assert.h"
#include "LIB_utildefines.h"

namespace rose {

template<typename T> class Span;

class IndexRange {
public:
	using size_type = size_t;
	using ssize_type = ptrdiff_t;

private:
	size_type start_ = 0;
	size_type size_ = 0;

public:
	constexpr IndexRange() = default;

	constexpr explicit IndexRange(size_type size) : start_(0), size_(size) {
	}

	constexpr IndexRange(size_type start, size_type size) : start_(start), size_(size) {
	}

	class Iterator {
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = size_t;
		using pointer = const size_t *;
		using reference = const size_t &;
		using difference_type = std::ptrdiff_t;

	private:
		size_type current_;

	public:
		constexpr explicit Iterator(size_type current) : current_(current) {
		}

		constexpr Iterator &operator++() {
			current_++;
			return *this;
		}

		constexpr Iterator operator++(int) {
			Iterator copied_iterator = *this;
			++(*this);
			return copied_iterator;
		}

		constexpr friend bool operator!=(const Iterator &a, const Iterator &b) {
			return a.current_ != b.current_;
		}

		constexpr friend bool operator==(const Iterator &a, const Iterator &b) {
			return a.current_ == b.current_;
		}

		constexpr friend int64_t operator-(const Iterator &a, const Iterator &b) {
			return a.current_ - b.current_;
		}

		constexpr int64_t operator*() const {
			return current_;
		}
	};

	constexpr Iterator begin() const {
		return Iterator(start_);
	}

	constexpr Iterator end() const {
		return Iterator(start_ + size_);
	}

	constexpr size_type operator[](size_type index) const {
		ROSE_assert(index < this->size());
		return start_ + index;
	}

	constexpr friend bool operator==(IndexRange a, IndexRange b) {
		return (a.size_ == b.size_) && (a.start_ == b.start_ || a.size_ == 0);
	}
	constexpr friend bool operator!=(IndexRange a, IndexRange b) {
		return !(a == b);
	}

	constexpr size_type size() const {
		return size_;
	}

	constexpr IndexRange index_range() const {
		return IndexRange(size_);
	}

	constexpr bool is_empty() const {
		return size_ == 0;
	}

	constexpr IndexRange after(size_type n) const {
		return IndexRange(start_ + size_, n);
	}

	constexpr IndexRange before(size_type n) const {
		return IndexRange(start_ - n, n);
	}

	constexpr size_type first() const {
		ROSE_assert(size_ > 0);
		return start_;
	}

	constexpr size_type last(const size_type n = 0) const {
		ROSE_assert(size_ > n);
		return start_ + size_ - 1 - n;
	}

	constexpr size_type one_before_start() const {
		ROSE_assert(start_ > 0);
		return start_ - 1;
	}

	constexpr size_type one_after_last() const {
		return start_ + size_;
	}

	constexpr size_type start() const {
		return start_;
	}

	constexpr bool contains(size_type value) const {
		return value >= start_ && value < start_ + size_;
	}

	constexpr IndexRange slice(size_type start, size_type size) const {
		return IndexRange(start_ + start, size);
	}

	constexpr IndexRange slice(IndexRange range) const {
		return this->slice(range.start(), range.size());
	}

	constexpr IndexRange intersect(IndexRange other) const {
		const size_type old_end = start_ + size_;
		const size_type new_start = std::min(old_end, std::max(start_, other.start_));
		const size_type new_end = std::max(new_start, std::min(old_end, other.start_ + other.size_));
		return IndexRange(new_start, new_end - new_start);
	}

	constexpr IndexRange drop_front(size_type n) const {
		const size_type new_size = static_cast<size_type>(std::max<ssize_type>(0, size_ - n));
		return IndexRange(start_ + n, new_size);
	}

	constexpr IndexRange drop_back(size_type n) const {
		const size_type new_size = static_cast<size_type>(std::max<ssize_type>(0, size_ - n));
		return IndexRange(start_, n);
	}

	constexpr IndexRange take_front(size_type n) const {
		const size_type new_size = std::min<size_type>(size_, n);
		return IndexRange(start_, new_size);
	}

	constexpr IndexRange take_back(size_type n) const {
		const size_type new_size = std::min<size_type>(size_, n);
		return IndexRange(start_ + size_ - new_size, new_size);
	}

	constexpr IndexRange shift(size_type n) const {
		return IndexRange(start_ + n, size_);
	}
};

}  // namespace rose
