#ifndef LIB_HASH_TABLES_HH
#define LIB_HASH_TABLES_HH

#include <algorithm>
#include <iostream>
#include <cmath>

#include "LIB_allocator.hh"
#include "LIB_memory_utils.hh"
#include "LIB_utildefines.h"
#include "LIB_vector.hh"

#include "LIB_hash.h"
#include "LIB_math_bit.h"

namespace rose {

/* -------------------------------------------------------------------- */
/** \name Constexpr Utility Functions
 *
 * Those should eventually be de-duplicated with functions in LIB_math_base.h.
 * \{ */

inline constexpr size_t is_power_of_2_constexpr(const size_t x) {
	ROSE_assert(x >= 0);
	return (x & (x - 1)) == 0;
}

inline constexpr size_t log2_floor_constexpr(const size_t x) {
	ROSE_assert(x >= 0);
	return x <= 1 ? 0 : 1 + log2_floor_constexpr(x >> 1);
}

inline constexpr size_t log2_ceil_constexpr(const size_t x) {
	ROSE_assert(x >= 0);
	return (is_power_of_2_constexpr(int(x))) ? log2_floor_constexpr(x) : log2_floor_constexpr(x) + 1;
}

inline constexpr size_t power_of_2_max_constexpr(const size_t x) {
	ROSE_assert(x >= 0);
	return 1ll << log2_ceil_constexpr(x);
}

template<typename IntT> inline constexpr IntT ceil_division(const IntT x, const IntT y) {
	ROSE_assert(x >= 0);
	ROSE_assert(y >= 0);
	return x / y + ((x % y) != 0);
}

template<typename IntT> inline constexpr IntT floor_division(const IntT x, const IntT y) {
	ROSE_assert(x >= 0);
	ROSE_assert(y >= 0);
	return x / y;
}

inline constexpr size_t ceil_division_by_fraction(const size_t x, const size_t numerator, const size_t denominator) {
	return size_t(ceil_division(size_t(x) * size_t(denominator), size_t(numerator)));
}

inline constexpr size_t floor_multiplication_with_fraction(const size_t x, const size_t numerator, const size_t denominator) {
	return size_t((size_t(x) * size_t(numerator) / size_t(denominator)));
}

inline constexpr size_t total_slot_amount_for_usable_slots(const size_t min_usable_slots, const size_t max_load_factor_numerator, const size_t max_load_factor_denominator) {
	return power_of_2_max_constexpr(ceil_division_by_fraction(min_usable_slots, max_load_factor_numerator, max_load_factor_denominator));
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Load Factor
 *
 * This is an abstraction for a fractional load factor. The hash table using this class is assumed
 * to use arrays with a size that is a power of two.
 *
 * \{ */

class LoadFactor {
private:
	uint8_t numerator_;
	uint8_t denominator_;

public:
	LoadFactor(uint8_t numerator, uint8_t denominator) : numerator_(numerator), denominator_(denominator) {
		ROSE_assert(numerator > 0);
		ROSE_assert(numerator < denominator);
	}

	void compute_total_and_usable_slots(size_t min_total_slots, size_t min_usable_slots, size_t *r_total_slots, size_t *r_usable_slots) const {
		ROSE_assert(is_power_of_2_i((int)min_total_slots));

		size_t total_slots = this->compute_total_slots(min_usable_slots, numerator_, denominator_);
		total_slots = std::max(total_slots, min_total_slots);
		const size_t usable_slots = floor_multiplication_with_fraction(total_slots, numerator_, denominator_);
		ROSE_assert(min_usable_slots <= usable_slots);

		*r_total_slots = total_slots;
		*r_usable_slots = usable_slots;
	}

	static constexpr size_t compute_total_slots(size_t min_usable_slots, uint8_t numerator, uint8_t denominator) {
		return total_slot_amount_for_usable_slots(min_usable_slots, numerator, denominator);
	}
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Intrusive Key Info
 *
 * A hash table slot has to maintain state about whether the slot is empty, occupied or removed.
 * Usually, this state information is stored in its own variable. While it only needs two bits in
 * theory, in practice often 4 or 8 bytes are used, due to alignment requirements.
 *
 * One solution to deal with this problem is to embed the state information in the key. That means,
 * two values of the key type are selected to indicate whether the slot is empty or removed.
 *
 * The classes below tell a slot implementation which special key values it can use. They can be
 * used as #KeyInfo in slot types like #IntrusiveSetSlot and #IntrusiveMapSlot.
 *
 * A #KeyInfo type has to implement a couple of static methods that are descried in
 * #TemplatedKeyInfo.
 *
 * \{ */

/**
 * The template arguments EmptyValue and RemovedValue define which special are used. This can be
 * used when a hash table has integer keys and there are two specific integers that will never be
 * used as keys.
 */
template<typename Key, Key EmptyValue, Key RemovedValue> struct TemplatedKeyInfo {
	/**
	 * Get the value that indicates that the slot is empty. This is used to indicate new slots.
	 */
	static Key get_empty() {
		return EmptyValue;
	}

	/**
	 * Modify the given key so that it represents a removed slot.
	 */
	static void remove(Key &key) {
		key = RemovedValue;
	}

	/**
	 * Return true, when the given key indicates that the slot is empty.
	 */
	static bool is_empty(const Key &key) {
		return key == EmptyValue;
	}

	/**
	 * Return true, when the given key indicates that the slot is removed.
	 */
	static bool is_removed(const Key &key) {
		return key == RemovedValue;
	}

	/**
	 * Return true, when the key is valid, i.e. it can be contained in an occupied slot.
	 */
	static bool is_not_empty_or_removed(const Key &key) {
		return key != EmptyValue && key != RemovedValue;
	}
};

/**
 * `0xffff...ffff` indicates an empty slot.
 * `0xffff...fffe` indicates a removed slot.
 *
 * Those specific values are used, because with them a single comparison is enough to check whether
 * a slot is occupied. The keys `0x0000...0000` and `0x0000...0001` also satisfy this constraint.
 * However, nullptr is much more likely to be used as valid key.
 */
template<typename Pointer> struct PointerKeyInfo {
	static Pointer get_empty() {
		return (Pointer)UINTPTR_MAX;
	}

	static void remove(Pointer &pointer) {
		pointer = (Pointer)(UINTPTR_MAX - 1);
	}

	static bool is_empty(Pointer pointer) {
		return uintptr_t(pointer) == UINTPTR_MAX;
	}

	static bool is_removed(Pointer pointer) {
		return uintptr_t(pointer) == UINTPTR_MAX - 1;
	}

	static bool is_not_empty_or_removed(Pointer pointer) {
		return uintptr_t(pointer) < UINTPTR_MAX - 1;
	}
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Hash Table Stats
 *
 * A utility class that makes it easier for hash table implementations to provide statistics to the
 * developer. These statistics can be helpful when trying to figure out why a hash table is slow.
 *
 * To use this utility, a hash table has to implement various methods, that are mentioned below.
 *
 * \{ */

class HashTableStats {
private:
	Vector<size_t> keys_by_collision_count_;
	size_t total_collisions_;
	float average_collisions_;
	size_t size_;
	size_t capacity_;
	size_t removed_amount_;
	float load_factor_;
	float removed_load_factor_;
	size_t size_per_element_;
	size_t size_in_bytes_;
	const void *address_;

public:
	/**
	 * Requires that the hash table has the following methods:
	 * - count_collisions(key) -> size_t
	 * - size() -> size_t
	 * - capacity() -> size_t
	 * - removed_amount() -> size_t
	 * - size_per_element() -> size_t
	 * - size_in_bytes() -> size_t
	 */
	template<typename HashTable, typename Keys> HashTableStats(const HashTable &hash_table, const Keys &keys) {
		total_collisions_ = 0;
		size_ = hash_table.size();
		capacity_ = hash_table.capacity();
		removed_amount_ = hash_table.removed_amount();
		size_per_element_ = hash_table.size_per_element();
		size_in_bytes_ = hash_table.size_in_bytes();
		address_ = static_cast<const void *>(&hash_table);

		for (const auto &key : keys) {
			size_t collisions = hash_table.count_collisions(key);
			if (keys_by_collision_count_.size() <= collisions) {
				keys_by_collision_count_.append_n_times(0, collisions - keys_by_collision_count_.size() + 1);
			}
			keys_by_collision_count_[collisions]++;
			total_collisions_ += collisions;
		}

		average_collisions_ = (size_ == 0) ? 0 : (float)total_collisions_ / (float)size_;
		load_factor_ = (float)size_ / (float)capacity_;
		removed_load_factor_ = (float)removed_amount_ / (float)capacity_;
	}

	void print(const rose::StringRef name) const {
		std::cout << "Hash Table Stats: " << name << "\n";
		std::cout << "  Address: " << address_ << "\n";
		std::cout << "  Total Slots: " << capacity_ << "\n";
		std::cout << "  Occupied Slots:  " << size_ << " (" << load_factor_ * 100.0f << " %)\n";
		std::cout << "  Removed Slots: " << removed_amount_ << " (" << removed_load_factor_ * 100.0f << " %)\n";

		char memory_size_str[64];
		LIB_strnformat_byte_size(memory_size_str, ARRAY_SIZE(memory_size_str), size_in_bytes_, 3);
		std::cout << "  Size: ~" << memory_size_str << "\n";
		std::cout << "  Size per Slot: " << size_per_element_ << " bytes\n";

		std::cout << "  Average Collisions: " << average_collisions_ << "\n";
		for (size_t collision_count : keys_by_collision_count_.index_range()) {
			std::cout << "  " << collision_count << " Collisions: " << keys_by_collision_count_[collision_count] << "\n";
		}
	}
};

/** \} */

/**
 * This struct provides an equality operator that returns true for all objects that compare equal
 * when one would use the `==` operator. This is different from std::equal_to<T>, because that
 * requires the parameters to be of type T. Our hash tables support lookups using other types
 * without conversion, therefore DefaultEquality needs to be more generic.
 */
template<typename T> struct DefaultEquality {
	template<typename T1, typename T2> bool operator()(const T1 &a, const T2 &b) const {
		return a == b;
	}
};

/**
 * Support comparing different kinds of raw and smart pointers.
 */
struct PointerComparison {
	template<typename T1, typename T2> bool operator()(const T1 &a, const T2 &b) const {
		return &*a == &*b;
	}
};

template<typename T> struct DefaultEquality<std::unique_ptr<T>> : public PointerComparison {};
template<typename T> struct DefaultEquality<std::shared_ptr<T>> : public PointerComparison {};

struct SequenceComparison {
	template<typename T1, typename T2> bool operator()(const T1 &a, const T2 &b) const {
		const auto a_begin = a.begin();
		const auto a_end = a.end();
		const auto b_begin = b.begin();
		const auto b_end = b.end();
		if (a_end - a_begin != b_end - b_begin) {
			return false;
		}
		return std::equal(a_begin, a_end, b_begin);
	}
};

template<typename T, size_t InlineBufferCapacity, typename Allocator> struct DefaultEquality<Vector<T, InlineBufferCapacity, Allocator>> : public SequenceComparison {};

}  // namespace rose

#endif	// LIB_HASH_TABLES_HH
