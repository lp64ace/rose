#ifndef LIB_CPP_TYPE_HH
#define LIB_CPP_TYPE_HH

#include "LIB_hash.hh"
#include "LIB_index_mask.hh"
#include "LIB_map.hh"
#include "LIB_math_base.h"
#include "LIB_parameter_pack_utils.hh"
#include "LIB_string_ref.hh"
#include "LIB_utility_mixins.hh"

#include "LIB_utildefines.h"

enum class CPPTypeFlags {
	None = 0,
	Hashable = 1 << 0,
	Printable = 1 << 1,
	EqualityComparable = 1 << 2,
	IdentityDefaultValue = 1 << 3,

	BasicType = Hashable | Printable | EqualityComparable,
};
ENUM_OPERATORS(CPPTypeFlags, CPPTypeFlags::EqualityComparable);

namespace rose {

class CPPType : NonCopyable, NonMovable {
private:
	size_t size_ = 0;
	size_t alignment_ = 0;
	uintptr_t alignment_mask_ = 0;

	bool is_trivial_ = false;
	bool is_trivially_destructible_ = false;
	bool has_special_member_functions_ = false;

	void (*default_construct_)(void *ptr) = nullptr;
	void (*default_construct_indices_)(void *ptr, const IndexMask &mask) = nullptr;

	void (*value_initialize_)(void *ptr) = nullptr;
	void (*value_initialize_indices_)(void *ptr, const IndexMask &mask) = nullptr;

	void (*destruct_)(void *ptr) = nullptr;
	void (*destruct_indices_)(void *ptr, const IndexMask &mask) = nullptr;

	void (*copy_assign_)(const void *src, void *dst) = nullptr;
	void (*copy_assign_indices_)(const void *src, void *dst, const IndexMask &mask) = nullptr;
	void (*copy_assign_compressed_)(const void *src, void *dst, const IndexMask &mask) = nullptr;

	void (*copy_construct_)(const void *src, void *dst) = nullptr;
	void (*copy_construct_indices_)(const void *src, void *dst, const IndexMask &mask) = nullptr;
	void (*copy_construct_compressed_)(const void *src, void *dst, const IndexMask &mask) = nullptr;

	void (*move_assign_)(void *src, void *dst) = nullptr;
	void (*move_assign_indices_)(void *src, void *dst, const IndexMask &mask) = nullptr;

	void (*move_construct_)(void *src, void *dst) = nullptr;
	void (*move_construct_indices_)(void *src, void *dst, const IndexMask &mask) = nullptr;

	void (*relocate_assign_)(void *src, void *dst) = nullptr;
	void (*relocate_assign_indices_)(void *src, void *dst, const IndexMask &mask) = nullptr;

	void (*relocate_construct_)(void *src, void *dst) = nullptr;
	void (*relocate_construct_indices_)(void *src, void *dst, const IndexMask &mask) = nullptr;

	void (*fill_assign_indices_)(const void *value, void *dst, const IndexMask &mask) = nullptr;

	void (*fill_construct_indices_)(const void *value, void *dst, const IndexMask &mask) = nullptr;

	void (*print_)(const void *value, std::stringstream &ss) = nullptr;
	bool (*is_equal_)(const void *a, const void *b) = nullptr;
	uint64_t (*hash_)(const void *value) = nullptr;

	const void *default_value_ = nullptr;
	std::string debug_name_;

public:
	template<typename T, CPPTypeFlags Flags> CPPType(TypeTag<T>, TypeForValue<CPPTypeFlags, Flags>, StringRef);
	virtual ~CPPType() = default;

	/**
	 * Two types only compare equal when their pointer is equal. No two instances of CPPType for the
	 * same C++ type should be created.
	 */
	friend bool operator==(const CPPType &a, const CPPType &b) {
		return &a == &b;
	}

	friend bool operator!=(const CPPType &a, const CPPType &b) {
		return !(&a == &b);
	}

	/**
	 * Get the `CPPType` that corresponds to a specific static type.
	 * This only works for types that actually implement the template specialization using
	 * `ROSE_CPP_TYPE_MAKE`.
	 */
	template<typename T> static const CPPType &get() {
		/* Store the #CPPType locally to avoid making the function call in most cases. */
		static const CPPType &type = CPPType::get_impl<std::decay_t<T>>();
		return type;
	}
	template<typename T> static const CPPType &get_impl();

	/**
	 * Returns the name of the type for debugging purposes. This name should not be used as
	 * identifier.
	 */
	StringRefNull name() const {
		return debug_name_;
	}

	/**
	 * Required memory in bytes for an instance of this type.
	 *
	 * C++ equivalent:
	 *   `sizeof(T);`
	 */
	size_t size() const {
		return size_;
	}

	/**
	 * Required memory alignment for an instance of this type.
	 *
	 * C++ equivalent:
	 *   alignof(T);
	 */
	size_t alignment() const {
		return alignment_;
	}

	/**
	 * When true, the destructor does not have to be called on this type. This can sometimes be used
	 * for optimization purposes.
	 *
	 * C++ equivalent:
	 *   std::is_trivially_destructible_v<T>;
	 */
	bool is_trivially_destructible() const {
		return is_trivially_destructible_;
	}

	/**
	 * When true, the value is like a normal C type, it can be copied around with #memcpy and does
	 * not have to be destructed.
	 *
	 * C++ equivalent:
	 *   std::is_trivial_v<T>;
	 */
	bool is_trivial() const {
		return is_trivial_;
	}

	bool is_default_constructible() const {
		return default_construct_ != nullptr;
	}

	bool is_copy_constructible() const {
		return copy_assign_ != nullptr;
	}

	bool is_move_constructible() const {
		return move_assign_ != nullptr;
	}

	bool is_destructible() const {
		return destruct_ != nullptr;
	}

	bool is_copy_assignable() const {
		return copy_assign_ != nullptr;
	}

	bool is_move_assignable() const {
		return copy_construct_ != nullptr;
	}

	bool is_printable() const {
		return print_ != nullptr;
	}

	bool is_equality_comparable() const {
		return is_equal_ != nullptr;
	}

	bool is_hashable() const {
		return hash_ != nullptr;
	}

	/**
	 * Returns true, when the type has the following functions:
	 * - Default constructor.
	 * - Copy constructor.
	 * - Move constructor.
	 * - Copy assignment operator.
	 * - Move assignment operator.
	 * - Destructor.
	 */
	bool has_special_member_functions() const {
		return has_special_member_functions_;
	}

	/**
	 * Returns true, when the given pointer fulfills the alignment requirement of this type.
	 */
	bool pointer_has_valid_alignment(const void *ptr) const {
		return (uintptr_t(ptr) & alignment_mask_) == 0;
	}

	bool pointer_can_point_to_instance(const void *ptr) const {
		return ptr != nullptr && pointer_has_valid_alignment(ptr);
	}

	/**
	 * Call the default constructor at the given memory location.
	 * The memory should be uninitialized before this method is called.
	 * For some trivial types (like int), this method does nothing.
	 *
	 * C++ equivalent:
	 *   new (ptr) T;
	 */
	void default_construct(void *ptr) const {
		ROSE_assert(this->pointer_can_point_to_instance(ptr));

		default_construct_(ptr);
	}

	void default_construct_n(void *ptr, int64_t n) const {
		this->default_construct_indices(ptr, IndexMask(n));
	}

	void default_construct_indices(void *ptr, const IndexMask &mask) const {
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(ptr));

		default_construct_indices_(ptr, mask);
	}

	/**
	 * Same as #default_construct, but does zero initialization for trivial types.
	 *
	 * C++ equivalent:
	 *   new (ptr) T();
	 */
	void value_initialize(void *ptr) const {
		ROSE_assert(this->pointer_can_point_to_instance(ptr));

		value_initialize_(ptr);
	}

	void value_initialize_n(void *ptr, size_t n) const {
		this->value_initialize_indices(ptr, IndexMask(n));
	}

	void value_initialize_indices(void *ptr, const IndexMask &mask) const {
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(ptr));

		value_initialize_indices_(ptr, mask);
	}

	/**
	 * Call the destructor on the given instance of this type. The pointer must not be nullptr.
	 *
	 * For some trivial types, this does nothing.
	 *
	 * C++ equivalent:
	 *   ptr->~T();
	 */
	void destruct(void *ptr) const {
		ROSE_assert(this->pointer_can_point_to_instance(ptr));

		destruct_(ptr);
	}

	void destruct_n(void *ptr, size_t n) const {
		this->destruct_indices(ptr, IndexMask(n));
	}

	void destruct_indices(void *ptr, const IndexMask &mask) const {
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(ptr));

		destruct_indices_(ptr, mask);
	}

	/**
	 * Copy an instance of this type from src to dst.
	 *
	 * C++ equivalent:
	 *   dst = src;
	 */
	void copy_assign(const void *src, void *dst) const {
		ROSE_assert(this->pointer_can_point_to_instance(src));
		ROSE_assert(this->pointer_can_point_to_instance(dst));

		copy_assign_(src, dst);
	}

	void copy_assign_n(const void *src, void *dst, size_t n) const {
		this->copy_assign_indices(src, dst, IndexMask(n));
	}

	void copy_assign_indices(const void *src, void *dst, const IndexMask &mask) const {
		ROSE_assert(mask.size() == 0 || src != dst);
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(src));
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(dst));

		copy_assign_indices_(src, dst, mask);
	}

	/**
	 * Similar to #copy_assign_indices, but does not leave gaps in the #dst array.
	 */
	void copy_assign_compressed(const void *src, void *dst, const IndexMask &mask) const {
		ROSE_assert(mask.size() == 0 || src != dst);
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(src));
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(dst));

		copy_assign_compressed_(src, dst, mask);
	}

	/**
	 * Copy an instance of this type from src to dst.
	 *
	 * The memory pointed to by dst should be uninitialized.
	 *
	 * C++ equivalent:
	 *   new (dst) T(src);
	 */
	void copy_construct(const void *src, void *dst) const {
		ROSE_assert(src != dst || is_trivial_);
		ROSE_assert(this->pointer_can_point_to_instance(src));
		ROSE_assert(this->pointer_can_point_to_instance(dst));

		copy_construct_(src, dst);
	}

	void copy_construct_n(const void *src, void *dst, size_t n) const {
		this->copy_construct_indices(src, dst, IndexMask(n));
	}

	void copy_construct_indices(const void *src, void *dst, const IndexMask &mask) const {
		ROSE_assert(mask.size() == 0 || src != dst);
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(src));
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(dst));

		copy_construct_indices_(src, dst, mask);
	}

	/**
	 * Similar to #copy_construct_indices, but does not leave gaps in the #dst array.
	 */
	void copy_construct_compressed(const void *src, void *dst, const IndexMask &mask) const {
		ROSE_assert(mask.size() == 0 || src != dst);
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(src));
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(dst));

		copy_construct_compressed_(src, dst, mask);
	}

	/**
	 * Move an instance of this type from src to dst.
	 *
	 * The memory pointed to by dst should be initialized.
	 *
	 * C++ equivalent:
	 *   dst = std::move(src);
	 */
	void move_assign(void *src, void *dst) const {
		ROSE_assert(this->pointer_can_point_to_instance(src));
		ROSE_assert(this->pointer_can_point_to_instance(dst));

		move_assign_(src, dst);
	}

	void move_assign_n(void *src, void *dst, size_t n) const {
		this->move_assign_indices(src, dst, IndexMask(n));
	}

	void move_assign_indices(void *src, void *dst, const IndexMask &mask) const {
		ROSE_assert(mask.size() == 0 || src != dst);
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(src));
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(dst));

		move_assign_indices_(src, dst, mask);
	}

	/**
	 * Move an instance of this type from src to dst.
	 *
	 * The memory pointed to by dst should be uninitialized.
	 *
	 * C++ equivalent:
	 *   new (dst) T(std::move(src));
	 */
	void move_construct(void *src, void *dst) const {
		ROSE_assert(src != dst || is_trivial_);
		ROSE_assert(this->pointer_can_point_to_instance(src));
		ROSE_assert(this->pointer_can_point_to_instance(dst));

		move_construct_(src, dst);
	}

	void move_construct_n(void *src, void *dst, size_t n) const {
		this->move_construct_indices(src, dst, IndexMask(n));
	}

	void move_construct_indices(void *src, void *dst, const IndexMask &mask) const {
		ROSE_assert(mask.size() == 0 || src != dst);
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(src));
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(dst));

		move_construct_indices_(src, dst, mask);
	}

	/**
	 * Relocates an instance of this type from src to dst. src will point to uninitialized memory
	 * afterwards.
	 *
	 * C++ equivalent:
	 *   dst = std::move(src);
	 *   src->~T();
	 */
	void relocate_assign(void *src, void *dst) const {
		ROSE_assert(src != dst || is_trivial_);
		ROSE_assert(this->pointer_can_point_to_instance(src));
		ROSE_assert(this->pointer_can_point_to_instance(dst));

		relocate_assign_(src, dst);
	}

	void relocate_assign_n(void *src, void *dst, size_t n) const {
		this->relocate_assign_indices(src, dst, IndexMask(n));
	}

	void relocate_assign_indices(void *src, void *dst, const IndexMask &mask) const {
		ROSE_assert(mask.size() == 0 || src != dst);
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(src));
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(dst));

		relocate_assign_indices_(src, dst, mask);
	}

	/**
	 * Relocates an instance of this type from src to dst. src will point to uninitialized memory
	 * afterwards.
	 *
	 * C++ equivalent:
	 *   new (dst) T(std::move(src))
	 *   src->~T();
	 */
	void relocate_construct(void *src, void *dst) const {
		ROSE_assert(src != dst || is_trivial_);
		ROSE_assert(this->pointer_can_point_to_instance(src));
		ROSE_assert(this->pointer_can_point_to_instance(dst));

		relocate_construct_(src, dst);
	}

	void relocate_construct_n(void *src, void *dst, size_t n) const {
		this->relocate_construct_indices(src, dst, IndexMask(n));
	}

	void relocate_construct_indices(void *src, void *dst, const IndexMask &mask) const {
		ROSE_assert(mask.size() == 0 || src != dst);
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(src));
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(dst));

		relocate_construct_indices_(src, dst, mask);
	}

	/**
	 * Copy the given value to the first n elements in an array starting at dst.
	 *
	 * Other instances of the same type should live in the array before this method is called.
	 */
	void fill_assign_n(const void *value, void *dst, size_t n) const {
		this->fill_assign_indices(value, dst, IndexMask(n));
	}

	void fill_assign_indices(const void *value, void *dst, const IndexMask &mask) const {
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(value));
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(dst));

		fill_assign_indices_(value, dst, mask);
	}

	/**
	 * Copy the given value to the first n elements in an array starting at dst.
	 *
	 * The array should be uninitialized before this method is called.
	 */
	void fill_construct_n(const void *value, void *dst, size_t n) const {
		this->fill_construct_indices(value, dst, IndexMask(n));
	}

	void fill_construct_indices(const void *value, void *dst, const IndexMask &mask) const {
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(value));
		ROSE_assert(mask.size() == 0 || this->pointer_can_point_to_instance(dst));

		fill_construct_indices_(value, dst, mask);
	}

	bool can_exist_in_buffer(const size_t buffer_size, const size_t buffer_alignment) const {
		return size_ <= buffer_size && alignment_ <= buffer_alignment;
	}

	void print(const void *value, std::stringstream &ss) const {
		ROSE_assert(this->pointer_can_point_to_instance(value));
		print_(value, ss);
	}

	std::string to_string(const void *value) const;

	void print_or_default(const void *value, std::stringstream &ss, StringRef default_value) const;

	bool is_equal(const void *a, const void *b) const {
		ROSE_assert(this->pointer_can_point_to_instance(a));
		ROSE_assert(this->pointer_can_point_to_instance(b));
		return is_equal_(a, b);
	}

	bool is_equal_or_false(const void *a, const void *b) const {
		if (this->is_equality_comparable()) {
			return this->is_equal(a, b);
		}
		return false;
	}

	uint64_t hash(const void *value) const {
		ROSE_assert(this->pointer_can_point_to_instance(value));
		return hash_(value);
	}

	uint64_t hash_or_fallback(const void *value, uint64_t fallback_hash) const {
		if (this->is_hashable()) {
			return this->hash(value);
		}
		return fallback_hash;
	}

	/**
	 * Get a pointer to a constant value of this type. The specific value depends on the type.
	 * It is usually a zero-initialized or default constructed value.
	 */
	const void *default_value() const {
		return default_value_;
	}

	uint64_t hash() const {
		return get_default_hash(this);
	}

	void (*destruct_fn() const)(void *) {
		return destruct_;
	}

	template<typename T> bool is() const {
		return this == &CPPType::get<std::decay_t<T>>();
	}

	template<typename... T> bool is_any() const {
		return (this->is<T>() || ...);
	}

	/**
	 * Convert a #CPPType that is only known at run-time, to a static type that is known at
	 * compile-time. This allows the compiler to optimize a function for specific types, while all
	 * other types can still use a generic fallback function.
	 *
	 * \param Types: The types that code should be generated for.
	 * \param fn: The function object to call. This is expected to have a templated `operator()` and
	 * a non-templated `operator()`. The templated version will be called if the current #CPPType
	 *   matches any of the given types. Otherwise, the non-templated function is called.
	 */
	template<typename... Types, typename Fn> void to_static_type(const Fn &fn) const {
		using Callback = void (*)(const Fn &fn);

		/* Build a lookup table to avoid having to compare the current #CPPType with every type in
		 * #Types one after another. */
		static const Map<const CPPType *, Callback> callback_map = []() {
			Map<const CPPType *, Callback> callback_map;
			/* This adds an entry in the map for every type in #Types. */
			(callback_map.add_new(&CPPType::get<Types>(),
								  [](const Fn &fn) {
									  /* Call the templated `operator()` of the given function object. */
									  fn.template operator()<Types>();
								  }),
			 ...);
			return callback_map;
		}();

		const Callback callback = callback_map.lookup_default(this, nullptr);
		if (callback != nullptr) {
			callback(fn);
		}
		else {
			/* Call the non-templated `operator()` of the given function object. */
			fn();
		}
	}

private:
	template<typename Fn> struct TypeTagExecutor {
		const Fn &fn;

		template<typename T> void operator()() const {
			fn(TypeTag<T>{});
		}

		void operator()() const {
			fn(TypeTag<void>{});
		}
	};

public:
	/**
	 * Similar to #to_static_type but is easier to use with a lambda function. The function is
	 * expected to take a single `auto TypeTag` parameter. To extract the static type, use:
	 * `using T = typename decltype(TypeTag)::type;`
	 *
	 * If the current #CPPType is not in #Types, the type tag is `void`.
	 */
	template<typename... Types, typename Fn> void to_static_type_tag(const Fn &fn) const {
		TypeTagExecutor<Fn> executor{fn};
		this->to_static_type<Types...>(executor);
	}
};

/**
 * Initialize and register basic cpp types.
 */
void register_cpp_types();

}  // namespace rose

/* Utility for allocating an uninitialized buffer for a single value of the given #CPPType. */
#define BUFFER_FOR_CPP_TYPE_VALUE(type, variable_name)                                                    \
	rose::DynamicStackBuffer<64, 64> stack_buffer_for_##variable_name((type).size(), (type).alignment()); \
	void *variable_name = stack_buffer_for_##variable_name.buffer();

#endif	// LIB_CPP_TYPE_HH
