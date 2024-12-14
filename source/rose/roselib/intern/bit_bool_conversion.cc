#include "LIB_bit_bool_conversion.hh"

namespace rose::bits {

bool or_bools_into_bits(const Span<bool> bools, MutableBitSpan r_bits, const size_t allowed_overshoot) {
	ROSE_assert(r_bits.size() >= bools.size());
	if (bools.is_empty()) {
		return false;
	}

	const bool *bools_ = bools.data();

	bool any_true = false;

	for (size_t bool_i = 0; bool_i < bools.size(); bool_i++) {
		if (bools_[bool_i]) {
			r_bits[bool_i].set();
			any_true = true;
		}
	}

	return any_true;
}

}  // namespace rose::bits
