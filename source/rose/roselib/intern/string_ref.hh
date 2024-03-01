#include "LIB_string_ref.hh"

#include <ostream>

namespace rose {

std::ostream &operator<<(std::ostream &stream, StringRef ref) {
	stream << std::string(ref);
	return stream;
}

std::ostream &operator<<(std::ostream &stream, StringRefNull ref) {
	stream << std::string(ref.data(), size_t(ref.size()));
	return stream;
}

}  // namespace rose
