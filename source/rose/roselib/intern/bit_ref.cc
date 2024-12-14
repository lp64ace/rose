#include "LIB_bit_ref.hh"

#include <ostream>

namespace rose::bits {

std::ostream &operator<<(std::ostream &stream, const BitRef &bit) {
	return stream << (bit ? '1' : '0');
}

std::ostream &operator<<(std::ostream &stream, const MutableBitRef &bit) {
	return stream << BitRef(bit);
}

}  // namespace rose::bits
