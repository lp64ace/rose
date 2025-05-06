#ifndef LIB_BIT_GROUP_VECTOR_HH
#define LIB_BIT_GROUP_VECTOR_HH

#include "LIB_bit_vector.hh"

namespace rose::bits {

/**
 * A #BitGroupVector is a compact data structure that allows storing an arbitrary but fixed number
 * of bits per element. For example, it could be used to compactly store 5 bits per vertex in a
 * mesh. The data structure stores the bits in a way so that the #BitSpan for every element is
 * bounded according to #is_bounded_span. The makes sure that operations on entire groups can be
 * implemented efficiently. For example, one can easy `or` one group into another.
 */
template<size_t InlineBufferCapacity = 64, typename Allocator = GuardedAllocator> class BitGroupVector {
private:
	/**
	 * Number of bits per group.
	 */
	size_t group_size_ = 0;
	/**
	 * Actually stored number of bits per group so that individual groups are bounded according to
	 * #is_bounded_span.
	 */
	size_t aligned_group_size_ = 0;
	BitVector<InlineBufferCapacity, Allocator> data_;

	static size_t align_group_size(const size_t group_size) {
		if (group_size < 64) {
			/* Align to next power of two so that a single group never spans across two ints. */
			return (size_t)1 << (size_t)ceil(log2(group_size));
		}
		/* Align to multiple of BitsPerInt. */
		return (group_size + BitsPerInt - 1) & ~(BitsPerInt - 1);
	}

public:
	BitGroupVector(Allocator allocator = {}) noexcept : data_(allocator) {
	}

	BitGroupVector(NoExceptConstructor, Allocator allocator = {}) noexcept : BitGroupVector(allocator) {
	}

	BitGroupVector(const size_t size_in_groups, const size_t group_size, const bool value = false, Allocator allocator = {}) : group_size_(group_size), aligned_group_size_(align_group_size(group_size)), data_(size_in_groups * aligned_group_size_, value, allocator) {
		ROSE_assert(group_size >= 0);
		ROSE_assert(size_in_groups >= 0);
	}

	BitGroupVector(const BitGroupVector &other) : group_size_(other.group_size_), aligned_group_size_(other.aligned_group_size_), data_(other.data_) {
	}

	BitGroupVector(BitGroupVector &&other) : group_size_(other.group_size_), aligned_group_size_(other.aligned_group_size_), data_(std::move(other.data_)) {
	}

	BitGroupVector &operator=(const BitGroupVector &other) {
		return copy_assign_container(*this, other);
	}

	BitGroupVector &operator=(BitGroupVector &&other) {
		return move_assign_container(*this, std::move(other));
	}

	/** Get all the bits at an index. */
	BoundedBitSpan operator[](const size_t i) const {
		const size_t offset = aligned_group_size_ * i;
		return {data_.data() + (offset >> BitToIntIndexShift), IndexRange(offset & BitIndexMask, group_size_)};
	}

	/** Get all the bits at an index. */
	MutableBoundedBitSpan operator[](const size_t i) {
		const size_t offset = aligned_group_size_ * i;
		return {data_.data() + (offset >> BitToIntIndexShift), IndexRange(offset & BitIndexMask, group_size_)};
	}

	/** Number of groups. */
	size_t size() const {
		return aligned_group_size_ == 0 ? 0 : data_.size() / aligned_group_size_;
	}

	bool is_empty() const {
		return this->size() == 0;
	}

	/** Number of bits per group. */
	size_t group_size() const {
		return group_size_;
	}

	IndexRange index_range() const {
		return IndexRange{this->size()};
	}

	/**
	 * Get all stored bits. Note that this may also contain padding bits. This can be used to e.g.
	 * mix multiple #BitGroupVector.
	 */
	BoundedBitSpan all_bits() const {
		return data_;
	}

	MutableBoundedBitSpan all_bits() {
		return data_;
	}
};

}  // namespace rose::bits

namespace rose {
using bits::BitGroupVector;
}  // namespace rose

#endif	// LIB_BIT_GROUP_VECTOR_HH
