#ifndef LIB_BIT_REF_HH
#define LIB_BIT_REF_HH

#include "LIB_index_range.hh"
#include "LIB_utildefines.h"

#include <iosfwd>

namespace rose::bits {

/** Using a large integer type is better because then it's easier to process many bits at once. */
using BitInt = uint64_t;
/** Number of bits that fit into #BitInt. */
static constexpr size_t BitsPerInt = size_t(sizeof(BitInt) * 8);
/** Shift amount to get from a bit index to an int index. Equivalent to `log(BitsPerInt, 2)`. */
static constexpr size_t BitToIntIndexShift = 3 + (sizeof(BitInt) >= 2) + (sizeof(BitInt) >= 4) + (sizeof(BitInt) >= 8);
/** Bit mask containing a 1 for the last few bits that index a bit inside of an #BitInt. */
static constexpr BitInt BitIndexMask = (BitInt(1) << BitToIntIndexShift) - 1;

inline BitInt mask_first_n_bits(const size_t n) {
	ROSE_assert(n >= 0);
	ROSE_assert(n <= BitsPerInt);
	if (n == BitsPerInt) {
		return BitInt(-1);
	}
	return (BitInt(1) << n) - 1;
}

inline BitInt mask_last_n_bits(const size_t n) {
	return ~mask_first_n_bits(BitsPerInt - n);
}

inline BitInt mask_range_bits(const size_t start, const size_t size) {
	ROSE_assert(start >= 0);
	ROSE_assert(size >= 0);
	const size_t end = start + size;
	ROSE_assert(end <= BitsPerInt);
	if (end == BitsPerInt) {
		return mask_last_n_bits(size);
	}
	return ((BitInt(1) << end) - 1) & ~((BitInt(1) << start) - 1);
}

inline BitInt mask_single_bit(const size_t bit_index) {
	ROSE_assert(bit_index >= 0);
	ROSE_assert(bit_index < BitsPerInt);
	return BitInt(1) << bit_index;
}

inline BitInt *int_containing_bit(BitInt *data, const size_t bit_index) {
	return data + (bit_index >> BitToIntIndexShift);
}

inline const BitInt *int_containing_bit(const BitInt *data, const size_t bit_index) {
	return data + (bit_index >> BitToIntIndexShift);
}

/**
 * This is a read-only pointer to a specific bit. The value of the bit can be retrieved, but
 * not changed.
 */
class BitRef {
private:
	/** Points to the exact integer that the bit is in. */
	const BitInt *int_;
	/** All zeros except for a single one at the bit that is referenced. */
	BitInt mask_;

	friend class MutableBitRef;

public:
	BitRef() = default;

	/**
	 * Reference a specific bit in an array. Note that #data does *not* have to point to the
	 * exact integer the bit is in.
	 */
	BitRef(const BitInt *data, const size_t bit_index) {
		int_ = int_containing_bit(data, bit_index);
		mask_ = mask_single_bit(bit_index & BitIndexMask);
	}

	/**
	 * Return true when the bit is currently 1 and false otherwise.
	 */
	bool test() const {
		const BitInt value = *int_;
		const BitInt masked_value = value & mask_;
		return masked_value != 0;
	}

	operator bool() const {
		return this->test();
	}
};

/**
 * Similar to #BitRef, but also allows changing the referenced bit.
 */
class MutableBitRef {
private:
	/** Points to the integer that the bit is in. */
	BitInt *int_;
	/** All zeros except for a single one at the bit that is referenced. */
	BitInt mask_;

public:
	MutableBitRef() = default;

	/**
	 * Reference a specific bit in an array. Note that #data does *not* have to point to the
	 * exact int the bit is in.
	 */
	MutableBitRef(BitInt *data, const size_t bit_index) {
		int_ = int_containing_bit(data, bit_index);
		mask_ = mask_single_bit(bit_index & BitIndexMask);
	}

	/**
	 * Support implicitly casting to a read-only #BitRef.
	 */
	operator BitRef() const {
		BitRef bit_ref;
		bit_ref.int_ = int_;
		bit_ref.mask_ = mask_;
		return bit_ref;
	}

	/**
	 * Return true when the bit is currently 1 and false otherwise.
	 */
	bool test() const {
		const BitInt value = *int_;
		const BitInt masked_value = value & mask_;
		return masked_value != 0;
	}

	operator bool() const {
		return this->test();
	}

	/**
	 * Change the bit to a 1.
	 */
	void set() {
		*int_ |= mask_;
	}

	/**
	 * Change the bit to a 0.
	 */
	void reset() {
		*int_ &= ~mask_;
	}

	/**
	 * Change the bit to a 1 if #value is true and 0 otherwise. If the value is highly unpredictable
	 * by the CPU branch predictor, it can be faster to use #set_branchless instead.
	 */
	void set(const bool value) {
		if (value) {
			this->set();
		}
		else {
			this->reset();
		}
	}

	/**
	 * Does the same as #set, but does not use a branch. This is faster when the input value is
	 * unpredictable for the CPU branch predictor (best case for this function is a uniform random
	 * distribution with 50% probability for true and false). If the value is predictable, this is
	 * likely slower than #set.
	 */
	void set_branchless(const bool value) {
		const BitInt value_int = BitInt(value);
		ROSE_assert(ELEM(value_int, 0, 1));
		const BitInt old = *int_;
		*int_ =
			/* Unset bit. */
			(~mask_ & old)
			/* Optionally set it again. The -1 turns a 1 into `0x00...` and a 0 into `0xff...`. */
			| (mask_ & ~(value_int - 1));
	}

	MutableBitRef &operator|=(const bool value) {
		if (value) {
			this->set();
		}
		return *this;
	}

	MutableBitRef &operator&=(const bool value) {
		if (!value) {
			this->reset();
		}
		return *this;
	}
};

std::ostream &operator<<(std::ostream &stream, const BitRef &bit);
std::ostream &operator<<(std::ostream &stream, const MutableBitRef &bit);

}  // namespace rose::bits

namespace rose {
using bits::BitRef;
using bits::MutableBitRef;
}  // namespace rose

#endif	// LIB_BIT_REF_HH
