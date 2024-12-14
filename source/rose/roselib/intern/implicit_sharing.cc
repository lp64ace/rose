#include "MEM_guardedalloc.h"

#include "LIB_assert.h"
#include "LIB_implicit_sharing.hh"

#include <algorithm>
#include <cstring>

namespace rose::implicit_sharing {

class MEMFreeImplicitSharing : public ImplicitSharingInfo {
public:
	void *data;

	MEMFreeImplicitSharing(void *data) : data(data) {
		ROSE_assert(data != nullptr);
	}

private:
	void delete_self_with_data() override {
		MEM_freeN(data);
		MEM_delete(this);
	}
};

const ImplicitSharingInfo *info_for_mem_free(void *data) {
	return MEM_new<MEMFreeImplicitSharing>(__func__, data);
}

namespace detail {

void *make_trivial_data_mutable_impl(void *old_data, const size_t size, const size_t alignment, const ImplicitSharingInfo **sharing_info) {
	if (!old_data) {
		ROSE_assert(size == 0);
		return nullptr;
	}

	ROSE_assert(*sharing_info != nullptr);
	if ((*sharing_info)->is_mutable()) {
		(*sharing_info)->tag_ensured_mutable();
	}
	else {
		void *new_data = MEM_mallocN_aligned(size, alignment, __func__);
		memcpy(new_data, old_data, size);
		(*sharing_info)->remove_user_and_delete_if_last();
		*sharing_info = info_for_mem_free(new_data);
		return new_data;
	}

	return old_data;
}

void *resize_trivial_array_impl(void *old_data, const size_t old_size, const size_t new_size, const size_t alignment, const ImplicitSharingInfo **sharing_info) {
	if (new_size == 0) {
		if (*sharing_info) {
			(*sharing_info)->remove_user_and_delete_if_last();
			*sharing_info = nullptr;
		}
		return nullptr;
	}

	if (!old_data) {
		ROSE_assert(old_size == 0);
		ROSE_assert(*sharing_info == nullptr);
		void *new_data = MEM_mallocN_aligned(new_size, alignment, __func__);
		*sharing_info = info_for_mem_free(new_data);
		return new_data;
	}

	ROSE_assert(old_size != 0);
	if ((*sharing_info)->is_mutable()) {
		if (auto *info = const_cast<MEMFreeImplicitSharing *>(dynamic_cast<const MEMFreeImplicitSharing *>(*sharing_info))) {
			/* If the array was allocated with the MEM allocator, we can use realloc directly, which
			 * could theoretically give better performance if the data can be reused in place. */
			void *new_data = static_cast<int *>(MEM_reallocN(old_data, new_size));
			info->data = new_data;
			(*sharing_info)->tag_ensured_mutable();
			return new_data;
		}
	}

	void *new_data = MEM_mallocN_aligned(new_size, alignment, __func__);
	memcpy(new_data, old_data, std::min(old_size, new_size));
	(*sharing_info)->remove_user_and_delete_if_last();
	*sharing_info = info_for_mem_free(new_data);
	return new_data;
}

}  // namespace detail

}  // namespace rose::implicit_sharing
