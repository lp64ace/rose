#include "LIB_compute_context.hh"
#include "LIB_hash_mm2a.h"

#include <sstream>

namespace rose {

void ComputeContextHash::mix_in(const void *data, size_t len) {
	const size_t hash_size = sizeof(ComputeContextHash);
	const size_t buffer_len = hash_size + len;
	DynamicStackBuffer<> buffer_owner(buffer_len, 8);
	char *buffer = static_cast<char *>(buffer_owner.buffer());
	memcpy(buffer, this, hash_size);
	memcpy(buffer + hash_size, data, len);

	this->v1 = LIB_hash_mm2((const unsigned char *)buffer, buffer_len, this->v2);
	this->v2 = LIB_hash_mm2((const unsigned char *)buffer, buffer_len, this->v1);
}

std::ostream &operator<<(std::ostream &stream, const ComputeContextHash &hash) {
	std::stringstream ss;
	ss << "0x" << std::hex << hash.v1 << hash.v2;
	stream << ss.str();
	return stream;
}

void ComputeContext::print_stack(std::ostream &stream, StringRef name) const {
	Stack<const ComputeContext *> stack;
	for (const ComputeContext *current = this; current; current = current->parent_) {
		stack.push(current);
	}
	stream << "Context Stack: " << name << "\n";
	while (!stack.is_empty()) {
		const ComputeContext *current = stack.pop();
		stream << "-> ";
		current->print_current_in_line(stream);
		const ComputeContextHash &current_hash = current->hash_;
		stream << " \t(hash: " << current_hash << ")\n";
	}
}

std::ostream &operator<<(std::ostream &stream, const ComputeContext &compute_context) {
	compute_context.print_stack(stream, "");
	return stream;
}

}  // namespace rose
