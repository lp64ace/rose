#ifndef LIB_SET_HH
#define LIB_SET_HH

#include "LIB_array.hh"
#include "LIB_hash.hh"
#include "LIB_hash_tables.hh"
#include "LIB_probing_strategies.hh"
#include "LIB_set_slots.hh"

namespace rose {

template<
	/**
	 * Type of the elements that are stored in this set. It has to be movable.
	 * Furthermore, the hash and is-equal functions have to support it.
	 */
	typename Key,
	/**
	 * The minimum number of elements that can be stored in this Set without doing a heap
	 * allocation. This is useful when you expect to have many small sets. However, keep in mind
	 * that (unlike vector) initializing a set has a O(n) cost in the number of slots.
	 */
	size_t InlineBufferCapacity = default_inline_buffer_capacity(sizeof(Key)),
	/**
	 * The strategy used to deal with collisions. They are defined in ROSE_probing_strategies.hh.
	 */
	typename ProbingStrategy = DefaultProbingStrategy,
	/**
	 * The hash function used to hash the keys. There is a default for many types. See ROSE_hash.hh
	 * for examples on how to define a custom hash function.
	 */
	typename Hash = DefaultHash<Key>,
	/**
	 * The equality operator used to compare keys. By default it will simply compare keys using the
	 * `==` operator.
	 */
	typename IsEqual = DefaultEquality<Key>,
	/**
	 * This is what will actually be stored in the hash table array. At a minimum a slot has to
	 * be able to hold a key and information about whether the slot is empty, occupied or removed.
	 * Using a non-standard slot type can improve performance or reduce the memory footprint. For
	 * example, a hash can be stored in the slot, to make inequality checks more efficient. Some
	 * types have special values that can represent an empty or removed state, eliminating the need
	 * for an additional variable. Also see ROSE_set_slots.hh.
	 */
	typename Slot = typename DefaultSetSlot<Key>::type,
	/**
	 * The allocator used by this set. Should rarely be changed, except when you don't want that
	 * MEM_* is used internally.
	 */
	typename Allocator = GuardedAllocator>
class Set {
public:
	class Iterator;
	using value_type = Key;
	using pointer = Key *;
	using const_pointer = const Key *;
	using reference = Key &;
	using const_reference = const Key &;
	using iterator = Iterator;
	using size_type = size_t;

private:
	/**
	 * Slots are either empty, occupied or removed. The number of occupied slots can be computed by
	 * subtracting the removed slots from the occupied-and-removed slots.
	 */
	size_type removed_slots_;
	size_type occupied_and_removed_slots_;

	/**
	 * The maximum number of slots that can be used (either occupied or removed) until the set has to
	 * grow. This is the total number of slots times the max load factor.
	 */
	size_type usable_slots_;

	/**
	 * The number of slots minus one. This is a bit mask that can be used to turn any integer into a
	 * valid slot index efficiently.
	 */
	uint64_t slot_mask_;

	/** This is called to hash incoming keys. */
	ROSE_NO_UNIQUE_ADDRESS Hash hash_;

	/** This is called to check equality of two keys. */
	ROSE_NO_UNIQUE_ADDRESS IsEqual is_equal_;

	/** The max load factor is 1/2 = 50% by default. */
#define LOAD_FACTOR 1, 2
	LoadFactor max_load_factor_ = LoadFactor(LOAD_FACTOR);
	using SlotArray = Array<Slot, LoadFactor::compute_total_slots(InlineBufferCapacity, LOAD_FACTOR), Allocator>;
#undef LOAD_FACTOR

	/**
	 * This is the array that contains the actual slots. There is always at least one empty slot and
	 * the size of the array is a power of two.
	 */
	SlotArray slots_;

	/** Iterate over a slot index sequence for a given hash. */
#define SET_SLOT_PROBING_BEGIN(HASH, R_SLOT)                          \
	SLOT_PROBING_BEGIN(ProbingStrategy, HASH, slot_mask_, SLOT_INDEX) \
	auto &R_SLOT = slots_[SLOT_INDEX];
#define SET_SLOT_PROBING_END() SLOT_PROBING_END()
public:
	/**
	 * Initialize an empty set. This is a cheap operation no matter how large the inline buffer
	 * is. This is necessary to avoid a high cost when no elements are added at all. An optimized
	 * grow operation is performed on the first insertion.
	 */
	Set(Allocator allocator = {}) noexcept : removed_slots_(0), occupied_and_removed_slots_(0), usable_slots_(0), slot_mask_(0), slots_(1, allocator) {
	}

	Set(NoExceptConstructor, Allocator allocator = {}) noexcept : Set(allocator) {
	}

	Set(Span<Key> values, Allocator allocator = {}) : Set(NoExceptConstructor(), allocator) {
		this->add_multiple(values);
	}

	/**
	 * Construct a set that contains the given keys. Duplicates will be removed automatically.
	 */
	Set(const std::initializer_list<Key> &values) : Set(Span<Key>(values)) {
	}

	~Set() = default;

	Set(const Set &other) = default;

	Set(Set &&other) noexcept(std::is_nothrow_move_constructible_v<SlotArray>) : Set(NoExceptConstructor(), other.slots_.allocator()) {
		if constexpr (std::is_nothrow_move_constructible_v<SlotArray>) {
			slots_ = std::move(other.slots_);
		}
		else {
			try {
				slots_ = std::move(other.slots_);
			}
			catch (...) {
				other.noexcept_reset();
				throw;
			}
		}
		removed_slots_ = other.removed_slots_;
		occupied_and_removed_slots_ = other.occupied_and_removed_slots_;
		usable_slots_ = other.usable_slots_;
		slot_mask_ = other.slot_mask_;
		hash_ = std::move(other.hash_);
		is_equal_ = std::move(other.is_equal_);
		other.noexcept_reset();
	}

	Set &operator=(const Set &other) {
		return copy_assign_container(*this, other);
	}

	Set &operator=(Set &&other) {
		return move_assign_container(*this, std::move(other));
	}

	/**
	 * Add a new key to the set. This invokes undefined behavior when the key is in the set already.
	 * When you know for certain that a key is not in the set yet, use this method for better
	 * performance. This also expresses the intent better.
	 */
	void add_new(const Key &key) {
		this->add_new__impl(key, hash_(key));
	}
	void add_new(Key &&key) {
		this->add_new__impl(std::move(key), hash_(key));
	}

	/**
	 * Add a key to the set. If the key exists in the set already, nothing is done. The return value
	 * is true if the key was newly added to the set.
	 *
	 * This is similar to std::unordered_set::insert.
	 */
	bool add(const Key &key) {
		return this->add_as(key);
	}
	bool add(Key &&key) {
		return this->add_as(std::move(key));
	}
	template<typename ForwardKey> bool add_as(ForwardKey &&key) {
		return this->add__impl(std::forward<ForwardKey>(key), hash_(key));
	}

	/**
	 * Convenience function to add many keys to the set at once. Duplicates are removed
	 * automatically.
	 *
	 * We might be able to make this faster than sequentially adding all keys, but that is not
	 * implemented yet.
	 */
	void add_multiple(Span<Key> keys) {
		for (const Key &key : keys) {
			this->add(key);
		}
	}

	/**
	 * Convenience function to add many new keys to the set at once. The keys must not exist in the
	 * set before and there must not be duplicates in the array.
	 */
	void add_multiple_new(Span<Key> keys) {
		for (const Key &key : keys) {
			this->add_new(key);
		}
	}

	/**
	 * Returns true if the key is in the set.
	 *
	 * This is similar to std::unordered_set::find() != std::unordered_set::end().
	 */
	bool contains(const Key &key) const {
		return this->contains_as(key);
	}
	template<typename ForwardKey> bool contains_as(const ForwardKey &key) const {
		return this->contains__impl(key, hash_(key));
	}

	/**
	 * Returns the key that is stored in the set that compares equal to the given key. This invokes
	 * undefined behavior when the key is not in the set.
	 */
	const Key &lookup_key(const Key &key) const {
		return this->lookup_key_as(key);
	}
	template<typename ForwardKey> const Key &lookup_key_as(const ForwardKey &key) const {
		return this->lookup_key__impl(key, hash_(key));
	}

	/**
	 * Returns the key that is stored in the set that compares equal to the given key. If the key is
	 * not in the set, the given default value is returned instead.
	 */
	const Key &lookup_key_default(const Key &key, const Key &default_value) const {
		return this->lookup_key_default_as(key, default_value);
	}
	template<typename ForwardKey> const Key &lookup_key_default_as(const ForwardKey &key, const Key &default_key) const {
		const Key *ptr = this->lookup_key_ptr__impl(key, hash_(key));
		if (ptr == nullptr) {
			return default_key;
		}
		return *ptr;
	}

	/**
	 * Returns a pointer to the key that is stored in the set that compares equal to the given key.
	 * If the key is not in the set, nullptr is returned instead.
	 */
	const Key *lookup_key_ptr(const Key &key) const {
		return this->lookup_key_ptr_as(key);
	}
	template<typename ForwardKey> const Key *lookup_key_ptr_as(const ForwardKey &key) const {
		return this->lookup_key_ptr__impl(key, hash_(key));
	}

	/**
	 * Returns the key in the set that compares equal to the given key. If it does not exist, the key
	 * is newly added.
	 */
	const Key &lookup_key_or_add(const Key &key) {
		return this->lookup_key_or_add_as(key);
	}
	const Key &lookup_key_or_add(Key &&key) {
		return this->lookup_key_or_add_as(std::move(key));
	}
	template<typename ForwardKey> const Key &lookup_key_or_add_as(ForwardKey &&key) {
		return this->lookup_key_or_add__impl(std::forward<ForwardKey>(key), hash_(key));
	}

	/**
	 * Deletes the key from the set. Returns true when the key did exist beforehand, otherwise false.
	 *
	 * This is similar to std::unordered_set::erase.
	 */
	bool remove(const Key &key) {
		return this->remove_as(key);
	}
	template<typename ForwardKey> bool remove_as(const ForwardKey &key) {
		return this->remove__impl(key, hash_(key));
	}

	/**
	 * Deletes the key from the set. This invokes undefined behavior when the key is not in the map.
	 */
	void remove_contained(const Key &key) {
		this->remove_contained_as(key);
	}
	template<typename ForwardKey> void remove_contained_as(const ForwardKey &key) {
		this->remove_contained__impl(key, hash_(key));
	}

	/**
	 * An iterator that can iterate over all keys in the set. The iterator is invalidated when the
	 * set is moved or when it is grown.
	 *
	 * Keys returned by this iterator are always const. They should not change, because this might
	 * also change their hash.
	 */
	class Iterator {
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = Key;
		using pointer = const Key *;
		using reference = const Key &;
		using difference_type = std::ptrdiff_t;

	private:
		const Slot *slots_;
		size_t total_slots_;
		size_t current_slot_;

		friend Set;

	public:
		Iterator(const Slot *slots, size_t total_slots, size_t current_slot) : slots_(slots), total_slots_(total_slots), current_slot_(current_slot) {
		}

		Iterator &operator++() {
			while (++current_slot_ < total_slots_) {
				if (slots_[current_slot_].is_occupied()) {
					break;
				}
			}
			return *this;
		}

		Iterator operator++(int) {
			Iterator copied_iterator = *this;
			++(*this);
			return copied_iterator;
		}

		const Key &operator*() const {
			return *slots_[current_slot_].key();
		}

		const Key *operator->() const {
			return slots_[current_slot_].key();
		}

		friend bool operator!=(const Iterator &a, const Iterator &b) {
			ROSE_assert(a.slots_ == b.slots_);
			ROSE_assert(a.total_slots_ == b.total_slots_);
			return a.current_slot_ != b.current_slot_;
		}

		friend bool operator==(const Iterator &a, const Iterator &b) {
			return !(a != b);
		}

	protected:
		const Slot &current_slot() const {
			return slots_[current_slot_];
		}
	};

	Iterator begin() const {
		for (size_type i = 0; i < slots_.size(); i++) {
			if (slots_[i].is_occupied()) {
				return Iterator(slots_.data(), slots_.size(), i);
			}
		}
		return this->end();
	}

	Iterator end() const {
		return Iterator(slots_.data(), slots_.size(), slots_.size());
	}

	/**
	 * Remove the key that the iterator is currently pointing at. It is valid to call this method
	 * while iterating over the set. However, after this method has been called, the removed element
	 * must not be accessed anymore.
	 */
	void remove(const Iterator &it) {
		/* The const cast is valid because this method itself is not const. */
		Slot &slot = const_cast<Slot &>(it.current_slot());
		ROSE_assert(slot.is_occupied());
		slot.remove();
		removed_slots_++;
	}

	/**
	 * Remove all values for which the given predicate is true and return the number of removed
	 * values.
	 *
	 * This is similar to std::erase_if.
	 */
	template<typename Predicate> size_type remove_if(Predicate &&predicate) {
		const size_type prev_size = this->size();
		for (Slot &slot : slots_) {
			if (slot.is_occupied()) {
				const Key &key = *slot.key();
				if (predicate(key)) {
					slot.remove();
					removed_slots_++;
				}
			}
		}
		return prev_size - this->size();
	}

	/**
	 * Print common statistics like size and collision count. This is useful for debugging purposes.
	 */
	void print_stats(const char *name) const {
		HashTableStats stats(*this, *this);
		stats.print(name);
	}

	/**
	 * Get the number of collisions that the probing strategy has to go through to find the key or
	 * determine that it is not in the set.
	 */
	size_t count_collisions(const Key &key) const {
		return this->count_collisions__impl(key, hash_(key));
	}

	/**
	 * Remove all elements from the set.
	 */
	void clear() {
		for (Slot &slot : slots_) {
			slot.~Slot();
			new (&slot) Slot();
		}

		removed_slots_ = 0;
		occupied_and_removed_slots_ = 0;
	}

	/**
	 * Removes all keys from the set and frees any allocated memory.
	 */
	void clear_and_shrink() {
		std::destroy_at(this);
		new (this) Set(NoExceptConstructor{});
	}

	/**
	 * Creates a new slot array and reinserts all keys inside of that. This method can be used to get
	 * rid of removed slots. Also this is useful for benchmarking the grow function.
	 */
	void rehash() {
		this->realloc_and_reinsert(this->size());
	}

	/**
	 * Returns the number of keys stored in the set.
	 */
	size_type size() const {
		return occupied_and_removed_slots_ - removed_slots_;
	}

	/**
	 * Returns true if no keys are stored.
	 */
	bool is_empty() const {
		return occupied_and_removed_slots_ == removed_slots_;
	}

	/**
	 * Returns the number of available slots. This is mostly for debugging purposes.
	 */
	size_type capacity() const {
		return slots_.size();
	}

	/**
	 * Returns the amount of removed slots in the set. This is mostly for debugging purposes.
	 */
	size_type removed_amount() const {
		return removed_slots_;
	}

	/**
	 * Returns the bytes required per element. This is mostly for debugging purposes.
	 */
	size_type size_per_element() const {
		return sizeof(Slot);
	}

	/**
	 * Returns the approximate memory requirements of the set in bytes. This is more correct for
	 * larger sets.
	 */
	size_type size_in_bytes() const {
		return sizeof(Slot) * slots_.size();
	}

	/**
	 * Potentially resize the set such that it can hold the specified number of keys without another
	 * grow operation.
	 */
	void reserve(const size_type n) {
		if (usable_slots_ < n) {
			this->realloc_and_reinsert(n);
		}
	}

	/**
	 * Returns true if there is a key that exists in both sets.
	 */
	static bool Intersects(const Set &a, const Set &b) {
		/* Make sure we iterate over the shorter set. */
		if (a.size() > b.size()) {
			return Intersects(b, a);
		}

		for (const Key &key : a) {
			if (b.contains(key)) {
				return true;
			}
		}
		return false;
	}

	/**
	 * Returns true if no key from a is also in b and vice versa.
	 */
	static bool Disjoint(const Set &a, const Set &b) {
		return !Intersects(a, b);
	}

	friend bool operator==(const Set &a, const Set &b) {
		if (a.size() != b.size()) {
			return false;
		}
		for (const Key &key : a) {
			if (!b.contains(key)) {
				return false;
			}
		}
		return true;
	}

	friend bool operator!=(const Set &a, const Set &b) {
		return !(a == b);
	}

private:
	ROSE_NOINLINE void realloc_and_reinsert(const size_type min_usable_slots) {
		size_type total_slots, usable_slots;
		max_load_factor_.compute_total_and_usable_slots(SlotArray::inline_buffer_capacity(), min_usable_slots, &total_slots, &usable_slots);
		ROSE_assert(total_slots >= 1);
		const uint64_t new_slot_mask = uint64_t(total_slots) - 1;

		/**
		 * Optimize the case when the set was empty beforehand. We can avoid some copies here.
		 */
		if (this->size() == 0) {
			try {
				slots_.reinitialize(total_slots);
			}
			catch (...) {
				this->noexcept_reset();
				throw;
			}
			removed_slots_ = 0;
			occupied_and_removed_slots_ = 0;
			usable_slots_ = usable_slots;
			slot_mask_ = new_slot_mask;
			return;
		}

		/* The grown array that we insert the keys into. */
		SlotArray new_slots(total_slots);

		try {
			for (Slot &slot : slots_) {
				if (slot.is_occupied()) {
					this->add_after_grow(slot, new_slots, new_slot_mask);
					slot.remove();
				}
			}
			slots_ = std::move(new_slots);
		}
		catch (...) {
			this->noexcept_reset();
			throw;
		}

		occupied_and_removed_slots_ -= removed_slots_;
		usable_slots_ = usable_slots;
		removed_slots_ = 0;
		slot_mask_ = new_slot_mask;
	}

	void add_after_grow(Slot &old_slot, SlotArray &new_slots, const uint64_t new_slot_mask) {
		const uint64_t hash = old_slot.get_hash(Hash());

		SLOT_PROBING_BEGIN(ProbingStrategy, hash, new_slot_mask, slot_index) {
			Slot &slot = new_slots[slot_index];
			if (slot.is_empty()) {
				slot.occupy(std::move(*old_slot.key()), hash);
				return;
			}
		}
		SLOT_PROBING_END();
	}

	/**
	 * In some cases when exceptions are thrown, it's best to just reset the entire container to make
	 * sure that invariants are maintained. This should happen very rarely in practice.
	 */
	void noexcept_reset() noexcept {
		Allocator allocator = slots_.allocator();
		this->~Set();
		new (this) Set(NoExceptConstructor(), allocator);
	}

	template<typename ForwardKey> bool contains__impl(const ForwardKey &key, const uint64_t hash) const {
		SET_SLOT_PROBING_BEGIN(hash, slot) {
			if (slot.is_empty()) {
				return false;
			}
			if (slot.contains(key, is_equal_, hash)) {
				return true;
			}
		}
		SET_SLOT_PROBING_END();
	}

	template<typename ForwardKey> const Key &lookup_key__impl(const ForwardKey &key, const uint64_t hash) const {
		ROSE_assert(this->contains_as(key));

		SET_SLOT_PROBING_BEGIN(hash, slot) {
			if (slot.contains(key, is_equal_, hash)) {
				return *slot.key();
			}
		}
		SET_SLOT_PROBING_END();
	}

	template<typename ForwardKey> const Key *lookup_key_ptr__impl(const ForwardKey &key, const uint64_t hash) const {
		SET_SLOT_PROBING_BEGIN(hash, slot) {
			if (slot.contains(key, is_equal_, hash)) {
				return slot.key();
			}
			if (slot.is_empty()) {
				return nullptr;
			}
		}
		SET_SLOT_PROBING_END();
	}

	template<typename ForwardKey> void add_new__impl(ForwardKey &&key, const uint64_t hash) {
		ROSE_assert(!this->contains_as(key));

		this->ensure_can_add();

		SET_SLOT_PROBING_BEGIN(hash, slot) {
			if (slot.is_empty()) {
				slot.occupy(std::forward<ForwardKey>(key), hash);
				ROSE_assert(hash_(*slot.key()) == hash);
				occupied_and_removed_slots_++;
				return;
			}
		}
		SET_SLOT_PROBING_END();
	}

	template<typename ForwardKey> bool add__impl(ForwardKey &&key, const uint64_t hash) {
		this->ensure_can_add();

		SET_SLOT_PROBING_BEGIN(hash, slot) {
			if (slot.is_empty()) {
				slot.occupy(std::forward<ForwardKey>(key), hash);
				ROSE_assert(hash_(*slot.key()) == hash);
				occupied_and_removed_slots_++;
				return true;
			}
			if (slot.contains(key, is_equal_, hash)) {
				return false;
			}
		}
		SET_SLOT_PROBING_END();
	}

	template<typename ForwardKey> bool remove__impl(const ForwardKey &key, const uint64_t hash) {
		SET_SLOT_PROBING_BEGIN(hash, slot) {
			if (slot.contains(key, is_equal_, hash)) {
				slot.remove();
				removed_slots_++;
				return true;
			}
			if (slot.is_empty()) {
				return false;
			}
		}
		SET_SLOT_PROBING_END();
	}

	template<typename ForwardKey> void remove_contained__impl(const ForwardKey &key, const uint64_t hash) {
		ROSE_assert(this->contains_as(key));

		SET_SLOT_PROBING_BEGIN(hash, slot) {
			if (slot.contains(key, is_equal_, hash)) {
				slot.remove();
				removed_slots_++;
				return;
			}
		}
		SET_SLOT_PROBING_END();
	}

	template<typename ForwardKey> const Key &lookup_key_or_add__impl(ForwardKey &&key, const uint64_t hash) {
		this->ensure_can_add();

		SET_SLOT_PROBING_BEGIN(hash, slot) {
			if (slot.contains(key, is_equal_, hash)) {
				return *slot.key();
			}
			if (slot.is_empty()) {
				slot.occupy(std::forward<ForwardKey>(key), hash);
				ROSE_assert(hash_(*slot.key()) == hash);
				occupied_and_removed_slots_++;
				return *slot.key();
			}
		}
		SET_SLOT_PROBING_END();
	}

	template<typename ForwardKey> size_type count_collisions__impl(const ForwardKey &key, const uint64_t hash) const {
		size_type collisions = 0;

		SET_SLOT_PROBING_BEGIN(hash, slot) {
			if (slot.contains(key, is_equal_, hash)) {
				return collisions;
			}
			if (slot.is_empty()) {
				return collisions;
			}
			collisions++;
		}
		SET_SLOT_PROBING_END();
	}

	void ensure_can_add() {
		if (occupied_and_removed_slots_ >= usable_slots_) {
			this->realloc_and_reinsert(this->size() + 1);
			ROSE_assert(occupied_and_removed_slots_ < usable_slots_);
		}
	}
};

}  // namespace rose

#endif	// LIB_SET_HH
