#ifndef RLO_READ_WRITE_HH
#define RLO_READ_WRITE_HH

#include "DNA_customdata_types.h"
#include "DNA_ID.h"

#include "LIB_stack.hh"
#include "LIB_function_ref.hh"
#include "LIB_memory_utils.hh"

#include "RLO_read_write.h"

/**
 * Specific code to prepare IDs to be written.
 *
 * Required for writing properly embedded IDs currently.
 *
 * \note Once there is a better generic handling of embedded IDs,
 * this may go back to private code in `writefile.cc`.
 */
struct RLO_Write_IDBuffer {
private:
	static constexpr int static_size = 8192;
	rose::DynamicStackBuffer<static_size> buffer_;

public:
	RLO_Write_IDBuffer(ID &id, bool is_placeholder);
	RLO_Write_IDBuffer(ID &id, RoseWriter *writer);

	ID *get() {
		return static_cast<ID *>(buffer_.buffer());
	};
};

/**
 * Needs to be called if the pointer is somewhere written before the call to #BLO_write_shared.
 */
void RLO_write_shared_tag(struct RoseWriter *writer, const void *data);
void RLO_write_shared(struct RoseWriter *writer, const void *data, size_t approximate_size_in_bytes, const ImplicitSharingInfoHandle *info, rose::FunctionRef<void()> write_fn);
const ImplicitSharingInfoHandle *RLO_read_shared(struct RoseDataReader *reader, void **data, rose::FunctionRef<const ImplicitSharingInfoHandle *()> read_fn);

#endif // !RLO_READ_WRITE_HH
