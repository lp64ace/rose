#ifndef LIB_BIT_VECTOR_HH
#define LIB_BIT_VECTOR_HH

#include "LIB_allocator.hh"
#include "LIB_bit_bool_conversion.hh"
#include "LIB_bit_span.hh"
#include "LIB_span.hh"

namespace rose::bits {

template<
	/**
	 * The number of bits that can be stored in the vector without doing an allocation.
	 */
	size_t InlineBufferCapacity = 64,
	/**
	 * The allocator used by this vector. Should rarely be changed, except when you don't want that
	 * MEM_* is used internally.
	 */
	typename Allocator = GuardedAllocator>
class BitVector {
private:
	static constexpr size_t required_ints_for_bits(const size_t number_of_bits) {
		return (number_of_bits + BitsPerInt - 1) / BitsPerInt;
	}

	static constexpr size_t IntsInInlineBuffer = required_ints_for_bits(InlineBufferCapacity);
	static constexpr size_t BitsInInlineBuffer = IntsInInlineBuffer * BitsPerInt;
	static constexpr size_t AllocationAlignment = alignof(BitInt);

	/** Points to the first integer used by the vector. It might point to the memory in the inline buffer. */
	BitInt *data_;

	size_t size_in_bits_;
	size_t capacity_in_bits_;

	ROSE_NO_UNIQUE_ADDRESS Allocator allocator_;
	ROSE_NO_UNIQUE_ADDRESS TypedBuffer<BitInt, IntsInInlineBuffer> inline_buffer_;

public:
	BitVector(Allocator allocator = {}) noexcept : allocator_(allocator) {
		data_ = inline_buffer_;
		size_in_bits_ = 0;
		capacity_in_bits_ = BitsInInlineBuffer;
		uninitialized_fill_n(data_, IntsInInlineBuffer, BitInt(0));
	}

	BitVector(NoExceptConstructor, Allocator allocator = {}) noexcept : BitVector(allocator) {
	}

	BitVector(const BoundedBitSpan span) : BitVector(NoExceptConstructor()) {
		const size_t ints_to_copy = required_ints_for_bits(span.size());
		if (span.size() <= BitsInInlineBuffer) {
			data_ = inline_buffer_;
			capacity_in_bits_ = BitsInInlineBuffer;
		}
		else {
			data_ = static_cast<BitInt *>(allocator_.allocate(ints_to_copy * sizeof(BitInt), AllocationAlignment, "BitVector::data"));
			capacity_in_bits_ = ints_to_copy * BitsPerInt;
		}
		size_in_bits_ = span.size();
		uninitialized_copy_n(span.data(), ints_to_copy, data_);
	}

	BitVector(const BitVector &other) : BitVector(BoundedBitSpan(other)) {
		allocator_ = other.allocator_;
	}

	BitVector(BitVector &&other) noexcept : BitVector(NoExceptConstructor(), other.allocator_) {
		if (other.is_inline()) {
			/* Copy the data into the inline buffer. */
			/* For small inline buffers, always copy all the bits because checking how many bits to copy
			 * would add additional overhead. */
			size_t ints_to_copy = IntsInInlineBuffer;
			if constexpr (IntsInInlineBuffer > 8) {
				/* Avoid copying too much unnecessary data in case the inline buffer is large. */
				ints_to_copy = other.used_ints_amount();
			}
			data_ = inline_buffer_;
			uninitialized_copy_n(other.data_, ints_to_copy, data_);
		}
		else {
			/* Steal the pointer. */
			data_ = other.data_;
		}
		size_in_bits_ = other.size_in_bits_;
		capacity_in_bits_ = other.capacity_in_bits_;

		/* Clear the other vector because it has been moved from. */
		other.data_ = other.inline_buffer_;
		other.size_in_bits_ = 0;
		other.capacity_in_bits_ = BitsInInlineBuffer;
	}

	/**
	 * Create a new vector with the given size and fill it with #value.
	 */
	BitVector(const size_t size_in_bits, const bool value = false, Allocator allocator = {}) : BitVector(NoExceptConstructor(), allocator) {
		this->resize(size_in_bits, value);
	}

	/**
	 * Create a bit vector based on an array of bools. Each byte of the input array maps to one bit.
	 */
	explicit BitVector(const Span<bool> values, Allocator allocator = {}) : BitVector(NoExceptConstructor(), allocator) {
		this->resize(values.size(), false);
		or_bools_into_bits(values, *this);
	}

	~BitVector() {
		if (!this->is_inline()) {
			allocator_.deallocate(data_);
		}
	}

	BitVector &operator=(const BitVector &other) {
		return copy_assign_container(*this, other);
	}

	BitVector &operator=(BitVector &&other) {
		return move_assign_container(*this, std::move(other));
	}

	operator BoundedBitSpan() const {
		return {data_, IndexRange(size_in_bits_)};
	}

	operator MutableBoundedBitSpan() {
		return {data_, IndexRange(size_in_bits_)};
	}

	/**
	 * Number of bits in the bit vector.
	 */
	size_t size() const {
		return size_in_bits_;
	}

	/**
	 * Number of bits that can be stored before the BitVector has to grow.
	 */
	size_t capacity() const {
		return capacity_in_bits_;
	}

	bool is_empty() const {
		return size_in_bits_ == 0;
	}

	BitInt *data() {
		return data_;
	}

	const BitInt *data() const {
		return data_;
	}

	/**
	 * Get a read-only reference to a specific bit.
	 */
	[[nodiscard]] BitRef operator[](const size_t index) const {
		ROSE_assert(index < size_in_bits_);
		return {data_, index};
	}

	/**
	 * Get a mutable reference to a specific bit.
	 */
	[[nodiscard]] MutableBitRef operator[](const size_t index) {
		ROSE_assert(index < size_in_bits_);
		return {data_, index};
	}

	IndexRange index_range() const {
		return IndexRange(size_in_bits_);
	}

	/**
	 * Add a new bit to the end of the vector.
	 */
	void append(const bool value) {
		this->ensure_space_for_one();
		MutableBitRef bit{data_, size_in_bits_};
		bit.set(value);
		size_in_bits_++;
	}

	BitIterator begin() const {
		return {data_, 0};
	}

	BitIterator end() const {
		return {data_, size_in_bits_};
	}

	MutableBitIterator begin() {
		return {data_, 0};
	}

	MutableBitIterator end() {
		return {data_, size_in_bits_};
	}

	/**
	 * Change the size of the vector. If the new vector is larger than the old one, the new elements
	 * are filled with #value.
	 */
	void resize(const size_t new_size_in_bits, const bool value = false) {
		const size_t old_size_in_bits = size_in_bits_;
		if (new_size_in_bits > old_size_in_bits) {
			this->reserve(new_size_in_bits);
		}
		size_in_bits_ = new_size_in_bits;
		if (old_size_in_bits < new_size_in_bits) {
			MutableBitSpan(data_, IndexRange::from_begin_end(old_size_in_bits, new_size_in_bits)).set_all(value);
		}
	}

	/**
	 * Set #value on every element.
	 */
	void fill(const bool value) {
		MutableBitSpan(data_, size_in_bits_).set_all(value);
	}

	/**
	 * Make sure that the capacity of the vector is large enough to hold the given amount of bits.
	 * The actual size is not changed.
	 */
	void reserve(const int new_capacity_in_bits) {
		this->realloc_to_at_least(new_capacity_in_bits);
	}

	/**
	 * Reset the size of the vector to zero elements, but keep the same memory capacity to be
	 * refilled again.
	 */
	void clear() {
		size_in_bits_ = 0;
	}

	/**
	 * Free memory and reset the vector to zero elements.
	 */
	void clear_and_shrink() {
		size_in_bits_ = 0;
		capacity_in_bits_ = 0;
		if (!this->is_inline()) {
			allocator_.deallocate(data_);
		}
		data_ = inline_buffer_;
	}

private:
	void ensure_space_for_one() {
		if (size_in_bits_ >= capacity_in_bits_) {
			this->realloc_to_at_least(size_in_bits_ + 1);
		}
	}

	ROSE_NOINLINE void realloc_to_at_least(const size_t min_capacity_in_bits, const BitInt initial_value_for_new_ints = 0) {
		if (capacity_in_bits_ >= min_capacity_in_bits) {
			return;
		}

		const size_t min_capacity_in_ints = this->required_ints_for_bits(min_capacity_in_bits);

		/* At least double the size of the previous allocation. */
		const size_t min_new_capacity_in_ints = 2 * this->required_ints_for_bits(capacity_in_bits_);

		const size_t new_capacity_in_ints = std::max(min_capacity_in_ints, min_new_capacity_in_ints);
		const size_t ints_to_copy = this->used_ints_amount();

		BitInt *new_data = static_cast<BitInt *>(allocator_.allocate(new_capacity_in_ints * sizeof(BitInt), AllocationAlignment, __func__));
		uninitialized_copy_n(data_, ints_to_copy, new_data);
		/* Always initialize new capacity even if it isn't used yet. That's necessary to avoid warnings
		 * caused by using uninitialized memory. This happens when e.g. setting a clearing a bit in an
		 * uninitialized int. */
		uninitialized_fill_n(new_data + ints_to_copy, new_capacity_in_ints - ints_to_copy, initial_value_for_new_ints);

		if (!this->is_inline()) {
			allocator_.deallocate(data_);
		}

		data_ = new_data;
		capacity_in_bits_ = new_capacity_in_ints * BitsPerInt;
	}

	bool is_inline() const {
		return data_ == inline_buffer_;
	}

	size_t used_ints_amount() const {
		return this->required_ints_for_bits(size_in_bits_);
	}
};

template<size_t InlineBufferCapacity, typename Allocator> inline BoundedBitSpan to_best_bit_span(const BitVector<InlineBufferCapacity, Allocator> &data) {
	return data;
}

template<size_t InlineBufferCapacity, typename Allocator> inline MutableBoundedBitSpan to_best_bit_span(BitVector<InlineBufferCapacity, Allocator> &data) {
	return data;
}

}  // namespace rose::bits

namespace rose {
using bits::BitVector;
}  // namespace rose

#endif	// LIB_BIT_VECTOR_HH
