#ifndef LIB_LINEAR_ALLOCATOR_HH
#define LIB_LINEAR_ALLOCATOR_HH

#include "LIB_string_ref.hh"
#include "LIB_utility_mixins.hh"
#include "LIB_vector.hh"

namespace rose {

template<typename Allocator = GuardedAllocator> class LinearAllocator : NonCopyable, NonMovable {
private:
	ROSE_NO_UNIQUE_ADDRESS Allocator allocator_;
	Vector<void *, 2> owned_buffers_;

	uintptr_t current_begin_;
	uintptr_t current_end_;

	/* Buffers larger than that are not packed together with smaller allocations to avoid wasting
	 * memory. */
	constexpr static inline size_t large_buffer_threshold = 4096;

public:
	LinearAllocator() {
		current_begin_ = 0;
		current_end_ = 0;
	}

	~LinearAllocator() {
		for (void *ptr : owned_buffers_) {
			allocator_.deallocate(ptr);
		}
	}

	/**
	 * Get a pointer to a memory buffer with the given size an alignment. The memory buffer will be
	 * freed when this LinearAllocator is destructed.
	 *
	 * The alignment has to be a power of 2.
	 */
	void *allocate(const size_t size, const size_t alignment) {
		ROSE_assert(size >= 0);
		ROSE_assert(alignment >= 1);
		ROSE_assert(is_power_of_2_i(alignment));

		const uintptr_t alignment_mask = alignment - 1;
		const uintptr_t potential_allocation_begin = (current_begin_ + alignment_mask) & ~alignment_mask;
		const uintptr_t potential_allocation_end = potential_allocation_begin + size;

		if (potential_allocation_end <= current_end_) {
			current_begin_ = potential_allocation_end;
			return reinterpret_cast<void *>(potential_allocation_begin);
		}
		if (size <= large_buffer_threshold) {
			this->allocate_new_buffer(size + alignment, alignment);
			return this->allocate(size, alignment);
		}
		return this->allocator_large_buffer(size, alignment);
	};

	/**
	 * Allocate a memory buffer that can hold an instance of T.
	 *
	 * This method only allocates memory and does not construct the instance.
	 */
	template<typename T> T *allocate() {
		return static_cast<T *>(this->allocate(sizeof(T), alignof(T)));
	}

	/**
	 * Allocate a memory buffer that can hold T array with the given size.
	 *
	 * This method only allocates memory and does not construct the instance.
	 */
	template<typename T> MutableSpan<T> allocate_array(size_t size) {
		T *array = static_cast<T *>(this->allocate(sizeof(T) * size, alignof(T)));
		return MutableSpan<T>(array, size);
	}

	/**
	 * Construct an instance of T in memory provided by this allocator.
	 *
	 * Arguments passed to this method will be forwarded to the constructor of T.
	 *
	 * You must not call `delete` on the returned value.
	 * Instead, only the destructor has to be called.
	 */
	template<typename T, typename... Args> destruct_ptr<T> construct(Args &&...args) {
		void *buffer = this->allocate(sizeof(T), alignof(T));
		T *value = new (buffer) T(std::forward<Args>(args)...);
		return destruct_ptr<T>(value);
	}

	/**
	 * Construct multiple instances of a type in an array. The constructor of is called with the
	 * given arguments. The caller is responsible for calling the destructor (and not `delete`) on
	 * the constructed elements.
	 */
	template<typename T, typename... Args> MutableSpan<T> construct_array(size_t size, Args &&...args) {
		MutableSpan<T> array = this->allocate_array<T>(size);
		for (const size_t i : IndexRange(size)) {
			new (&array[i]) T(std::forward<Args>(args)...);
		}
		return array;
	}

	/**
	 * Copy the given array into a memory buffer provided by this allocator.
	 */
	template<typename T> MutableSpan<T> construct_array_copy(Span<T> src) {
		if (src.is_empty()) {
			return {};
		}
		MutableSpan<T> dst = this->allocate_array<T>(src.size());
		uninitialized_copy_n(src.data(), src.size(), dst.data());
		return dst;
	}

	/**
	 * Copy the given string into a memory buffer provided by this allocator. The returned string is
	 * always null terminated.
	 */
	StringRefNull copy_string(StringRef str) {
		const size_t alloc_size = str.size() + 1;
		char *buffer = static_cast<char *>(this->allocate(alloc_size, 1));
		str.copy(buffer, alloc_size);
		return StringRefNull(static_cast<const char *>(buffer));
	}

	MutableSpan<void *> allocate_elements_and_pointer_array(size_t element_amount, size_t element_size, size_t element_alignment) {
		void *pointer_buffer = this->allocate(element_amount * sizeof(void *), alignof(void *));
		void *elements_buffer = this->allocate(element_amount * element_size, element_alignment);

		MutableSpan<void *> pointers(static_cast<void **>(pointer_buffer), element_amount);
		void *next_element_buffer = elements_buffer;
		for (size_t i : IndexRange(element_amount)) {
			pointers[i] = next_element_buffer;
			next_element_buffer = POINTER_OFFSET(next_element_buffer, element_size);
		}

		return pointers;
	}

	template<typename T, typename... Args> Span<T *> construct_elements_and_pointer_array(size_t n, Args &&...args) {
		MutableSpan<void *> void_pointers = this->allocate_elements_and_pointer_array(n, sizeof(T), alignof(T));
		MutableSpan<T *> pointers = void_pointers.cast<T *>();

		for (size_t i : IndexRange(n)) {
			new (static_cast<void *>(pointers[i])) T(std::forward<Args>(args)...);
		}

		return pointers;
	}

	/**
	 * Tell the allocator to use up the given memory buffer, before allocating new memory from the
	 * system.
	 */
	void provide_buffer(void *buffer, const size_t size) {
		ROSE_assert(owned_buffers_.is_empty());
		current_begin_ = uintptr_t(buffer);
		current_end_ = current_begin_ + size;
	}

	template<size_t Size, size_t Alignment> void provide_buffer(AlignedBuffer<Size, Alignment> &aligned_buffer) {
		this->provide_buffer(aligned_buffer.ptr(), Size);
	}

	/**
	 * Some algorithms can be implemented more efficiently by over-allocating the destination memory
	 * a bit. This allows the algorithm not to worry about having enough memory. Generally, this can
	 * be a useful strategy if the actual required memory is not known in advance, but an upper bound
	 * can be found. Ideally, one can free the over-allocated memory in the end again to reduce
	 * memory consumption.
	 *
	 * A linear allocator generally does allow freeing any memory. However, there is one exception.
	 * One can free the end of the last allocation (but not any previous allocation). While uses of
	 * this approach are quite limited, it's still the best option in some situations.
	 */
	void free_end_of_previous_allocation(const size_t original_allocation_size, const void *free_after) {
		/* If the original allocation size was large, it might have been separately allocated. In this
		 * case, we can't free the end of it anymore. */
		if (original_allocation_size <= large_buffer_threshold) {
			const size_t new_begin = uintptr_t(free_after);
			ROSE_assert(new_begin <= current_begin_);
#ifndef NDEBUG
			/* This condition is not really necessary but it helps finding the cases where memory was
			 * freed. */
			const size_t freed_bytes_num = current_begin_ - new_begin;
			if (freed_bytes_num > 0) {
				current_begin_ = new_begin;
			}
#else
			current_begin_ = new_begin;
#endif
		}
	}

	/**
	 * This allocator takes ownership of the buffers owned by `other`. Therefor, when `other` is
	 * destructed, memory allocated using it is not freed.
	 *
	 * Note that the caller is responsible for making sure that buffers passed into #provide_buffer
	 * of `other` live at least as long as this allocator.
	 */
	void transfer_ownership_from(LinearAllocator<> &other) {
		owned_buffers_.extend(other.owned_buffers_);
		other.owned_buffers_.clear();
		std::destroy_at(&other);
		new (&other) LinearAllocator<>();
	}

private:
	void allocate_new_buffer(size_t min_allocation_size, size_t min_alignment) {
		/* Possibly allocate more bytes than necessary for the current allocation. This way more small
		 * allocations can be packed together. Large buffers are allocated exactly to avoid wasting too
		 * much memory. */
		size_t size_in_bytes = min_allocation_size;
		if (size_in_bytes <= large_buffer_threshold) {
			/* Gradually grow buffer size with each allocation, up to a maximum. */
			const int grow_size = 1 << std::min<int>(owned_buffers_.size() + 6, 20);
			size_in_bytes = std::min(large_buffer_threshold, std::max<size_t>(size_in_bytes, grow_size));
		}

		void *buffer = this->allocated_owned(size_in_bytes, min_alignment);
		current_begin_ = uintptr_t(buffer);
		current_end_ = current_begin_ + size_in_bytes;
	}

	void *allocator_large_buffer(const size_t size, const size_t alignment) {
		return this->allocated_owned(size, alignment);
	}

	void *allocated_owned(const size_t size, const size_t alignment) {
		void *buffer = allocator_.allocate(size, alignment, __func__);
		owned_buffers_.append(buffer);
		return buffer;
	}
};

}  // namespace rose

#endif	// LIB_LINEAR_ALLOCATOR_HH
