#ifndef LIB_OFFSET_SPAN_HH
#define LIB_OFFSET_SPAN_HH

#include "LIB_span.hh"

namespace rose {

template<typename T, typename BaseT> class OffsetSpan {
private:
	/** Value that is added to every element in #data_ when accessed. */
	T offset_ = 0;
	/** Original span where each element is offset by #offset_. */
	Span<BaseT> data_;

public:
	OffsetSpan() = default;
	OffsetSpan(const T offset, const Span<BaseT> data) : offset_(offset), data_(data) {
	}

	/** \return Underlying span containing the values that are not offset. */
	Span<BaseT> base_span() const {
		return data_;
	}

	T offset() const {
		return offset_;
	}

	bool is_empty() const {
		return data_.is_empty();
	}

	size_t size() const {
		return data_.size();
	}

	T last(const size_t n = 0) const {
		return offset_ + data_.last(n);
	}

	IndexRange index_range() const {
		return data_.index_range();
	}

	T operator[](const size_t i) const {
		return T(data_[i]) + offset_;
	}

	OffsetSpan slice(const IndexRange &range) const {
		return {offset_, data_.slice(range)};
	}

	OffsetSpan slice(const size_t start, const size_t size) const {
		return {offset_, data_.slice(start, size)};
	}

	class Iterator : public iterator::RandomAccessIteratorMixin<Iterator> {
	public:
		using value_type = T;
		using pointer = const T *;
		using reference = T;

	private:
		T offset_;
		const BaseT *data_;

	public:
		Iterator(const T offset, const BaseT *data) : offset_(offset), data_(data) {
		}

		T operator*() const {
			return T(*data_) + offset_;
		}

		const BaseT *const &iter_prop() const {
			return data_;
		}
	};

	Iterator begin() const {
		return {offset_, data_.begin()};
	}

	Iterator end() const {
		return {offset_, data_.end()};
	}
};

}  // namespace rose

#endif	// LIB_OFFSET_SPAN_HH
