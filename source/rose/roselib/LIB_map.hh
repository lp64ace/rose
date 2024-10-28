#ifndef LIB_MAP_HH
#define LIB_MAP_HH

#include <optional>

#include "LIB_array.hh"
#include "LIB_hash.hh"
#include "LIB_hash_tables.hh"
#include "LIB_map_slots.hh"
#include "LIB_probing_strategies.hh"

namespace rose {

/**
 * A key-value-pair stored in a #Map. This is used when looping over Map.items().
 */
template<typename Key, typename Value> struct MapItem {
	const Key &key;
	const Value &value;
};

/**
 * Same as #MapItem, but the value is mutable. The key is still const because changing it might
 * change its hash value which would lead to undefined behavior in the #Map.
 */
template<typename Key, typename Value> struct MutableMapItem {
	const Key &key;
	Value &value;

	operator MapItem<Key, Value>() const {
		return {this->key, this->value};
	}
};

template<
	/**
	 * Type of the keys stored in the map. Keys have to be movable. Furthermore, the hash and
	 * is-equal functions have to support it.
	 */
	typename Key,
	/**
	 * Type of the value that is stored per key. It has to be movable as well.
	 */
	typename Value,
	/**
	 * The minimum number of elements that can be stored in this Map without doing a heap
	 * allocation. This is useful when you expect to have many small maps. However, keep in mind
	 * that (unlike vector) initializing a map has a O(n) cost in the number of slots.
	 */
	size_t InlineBufferCapacity = default_inline_buffer_capacity(sizeof(Key) + sizeof(Value)),
	/**
	 * The strategy used to deal with collisions. They are defined in LIB_probing_strategies.hh.
	 */
	typename ProbingStrategy = DefaultProbingStrategy,
	/**
	 * The hash function used to hash the keys. There is a default for many types. See LIB_hash.hh
	 * for examples on how to define a custom hash function.
	 */
	typename Hash = DefaultHash<Key>,
	/**
	 * The equality operator used to compare keys. By default it will simply compare keys using the
	 * `==` operator.
	 */
	typename IsEqual = DefaultEquality<Key>,
	/**
	 * This is what will actually be stored in the hash table array. At a minimum a slot has to be
	 * able to hold a key, a value and information about whether the slot is empty, occupied or
	 * removed. Using a non-standard slot type can improve performance or reduce the memory
	 * footprint for some types. Slot types are defined in LIB_map_slots.hh
	 */
	typename Slot = typename DefaultMapSlot<Key, Value>::type,
	/**
	 * The allocator used by this map. Should rarely be changed, except when you don't want that
	 * MEM_* is used internally.
	 */
	typename Allocator = GuardedAllocator>
class Map {
public:
	using size_type = size_t;
	using Item = MapItem<Key, Value>;
	using MutableItem = MutableMapItem<Key, Value>;

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
	int64_t slot_mask_;

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
#define MAP_SLOT_PROBING_BEGIN(HASH, R_SLOT) \
	SLOT_PROBING_BEGIN(ProbingStrategy, HASH, slot_mask_, SLOT_INDEX) \
	auto &R_SLOT = slots_[SLOT_INDEX];
#define MAP_SLOT_PROBING_END() SLOT_PROBING_END()

public:
	/**
	 * Initialize an empty map. This is a cheap operation no matter how large the inline buffer is.
	 * This is necessary to avoid a high cost when no elements are added at all. An optimized grow
	 * operation is performed on the first insertion.
	 */
	Map(Allocator allocator = {}) noexcept : removed_slots_(0), occupied_and_removed_slots_(0), usable_slots_(0), slot_mask_(0), hash_(), is_equal_(), slots_(1, allocator) {
	}

	Map(NoExceptConstructor, Allocator allocator = {}) noexcept : Map(allocator) {
	}

	~Map() = default;

	Map(const Map &other) = default;

	Map(Map &&other) noexcept(std::is_nothrow_move_constructible_v<SlotArray>) : Map(NoExceptConstructor(), other.slots_.allocator()) {
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

	Map &operator=(const Map &other) {
		return copy_assign_container(*this, other);
	}

	Map &operator=(Map &&other) {
		return move_assign_container(*this, std::move(other));
	}

	/**
	 * Insert a new key-value-pair into the map. This invokes undefined behavior when the key is in
	 * the map already.
	 */
	void add_new(const Key &key, const Value &value) {
		this->add_new_as(key, value);
	}
	void add_new(const Key &key, Value &&value) {
		this->add_new_as(key, std::move(value));
	}
	void add_new(Key &&key, const Value &value) {
		this->add_new_as(std::move(key), value);
	}
	void add_new(Key &&key, Value &&value) {
		this->add_new_as(std::move(key), std::move(value));
	}
	template<typename ForwardKey, typename... ForwardValue> void add_new_as(ForwardKey &&key, ForwardValue &&...value) {
		this->add_new__impl(std::forward<ForwardKey>(key), hash_(key), std::forward<ForwardValue>(value)...);
	}

	/**
	 * Add a key-value-pair to the map. If the map contains the key already, nothing is changed.
	 * If you want to replace the currently stored value, use `add_overwrite`.
	 * Returns true when the key has been newly added.
	 *
	 * This is similar to std::unordered_map::insert.
	 */
	bool add(const Key &key, const Value &value) {
		return this->add_as(key, value);
	}
	bool add(const Key &key, Value &&value) {
		return this->add_as(key, std::move(value));
	}
	bool add(Key &&key, const Value &value) {
		return this->add_as(std::move(key), value);
	}
	bool add(Key &&key, Value &&value) {
		return this->add_as(std::move(key), std::move(value));
	}
	template<typename ForwardKey, typename... ForwardValue> bool add_as(ForwardKey &&key, ForwardValue &&...value) {
		return this->add__impl(std::forward<ForwardKey>(key), hash_(key), std::forward<ForwardValue>(value)...);
	}

	/**
	 * Adds a key-value-pair to the map. If the map contained the key already, the corresponding
	 * value will be replaced.
	 * Returns true when the key has been newly added.
	 *
	 * This is similar to std::unordered_map::insert_or_assign.
	 */
	bool add_overwrite(const Key &key, const Value &value) {
		return this->add_overwrite_as(key, value);
	}
	bool add_overwrite(const Key &key, Value &&value) {
		return this->add_overwrite_as(key, std::move(value));
	}
	bool add_overwrite(Key &&key, const Value &value) {
		return this->add_overwrite_as(std::move(key), value);
	}
	bool add_overwrite(Key &&key, Value &&value) {
		return this->add_overwrite_as(std::move(key), std::move(value));
	}
	template<typename ForwardKey, typename... ForwardValue> bool add_overwrite_as(ForwardKey &&key, ForwardValue &&...value) {
		return this->add_overwrite__impl(std::forward<ForwardKey>(key), hash_(key), std::forward<ForwardValue>(value)...);
	}

	/**
	 * Returns true if there is a key in the map that compares equal to the given key.
	 *
	 * This is similar to std::unordered_map::contains.
	 */
	bool contains(const Key &key) const {
		return this->contains_as(key);
	}
	template<typename ForwardKey> bool contains_as(const ForwardKey &key) const {
		return this->lookup_slot_ptr(key, hash_(key)) != nullptr;
	}

	/**
	 * Deletes the key-value-pair with the given key. Returns true when the key was contained and is
	 * now removed, otherwise false.
	 *
	 * This is similar to std::unordered_map::erase.
	 */
	bool remove(const Key &key) {
		return this->remove_as(key);
	}
	template<typename ForwardKey> bool remove_as(const ForwardKey &key) {
		Slot *slot = this->lookup_slot_ptr(key, hash_(key));
		if (slot == nullptr) {
			return false;
		}
		slot->remove();
		removed_slots_++;
		return true;
	}

	/**
	 * Deletes the key-value-pair with the given key. This invokes undefined behavior when the key is
	 * not in the map.
	 */
	void remove_contained(const Key &key) {
		this->remove_contained_as(key);
	}
	template<typename ForwardKey> void remove_contained_as(const ForwardKey &key) {
		Slot &slot = this->lookup_slot(key, hash_(key));
		slot.remove();
		removed_slots_++;
	}

	/**
	 * Get the value that is stored for the given key and remove it from the map. This invokes
	 * undefined behavior when the key is not in the map.
	 */
	Value pop(const Key &key) {
		return this->pop_as(key);
	}
	template<typename ForwardKey> Value pop_as(const ForwardKey &key) {
		Slot &slot = this->lookup_slot(key, hash_(key));
		Value value = std::move(*slot.value());
		slot.remove();
		removed_slots_++;
		return value;
	}

	/**
	 * Get the value that is stored for the given key and remove it from the map. If the key is not
	 * in the map, a value-less optional is returned.
	 */
	std::optional<Value> pop_try(const Key &key) {
		return this->pop_try_as(key);
	}
	template<typename ForwardKey> std::optional<Value> pop_try_as(const ForwardKey &key) {
		Slot *slot = this->lookup_slot_ptr(key, hash_(key));
		if (slot == nullptr) {
			return {};
		}
		std::optional<Value> value = std::move(*slot->value());
		slot->remove();
		removed_slots_++;
		return value;
	}

	/**
	 * Get the value that corresponds to the given key and remove it from the map. If the key is not
	 * in the map, return the given default value instead.
	 */
	Value pop_default(const Key &key, const Value &default_value) {
		return this->pop_default_as(key, default_value);
	}
	Value pop_default(const Key &key, Value &&default_value) {
		return this->pop_default_as(key, std::move(default_value));
	}
	template<typename ForwardKey, typename... ForwardValue> Value pop_default_as(const ForwardKey &key, ForwardValue &&...default_value) {
		Slot *slot = this->lookup_slot_ptr(key, hash_(key));
		if (slot == nullptr) {
			return Value(std::forward<ForwardValue>(default_value)...);
		}
		Value value = std::move(*slot->value());
		slot->remove();
		removed_slots_++;
		return value;
	}

	/**
	 * This method can be used to implement more complex custom behavior without having to do
	 * multiple lookups
	 *
	 * When the key did not yet exist in the map, the create_value function is called. Otherwise the
	 * modify_value function is called.
	 *
	 * Both functions are expected to take a single parameter of type `Value *`. In create_value,
	 * this pointer will point to uninitialized memory that has to be initialized by the function. In
	 * modify_value, it will point to an already initialized value.
	 *
	 * The function returns whatever is returned from the create_value or modify_value callback.
	 * Therefore, both callbacks have to have the same return type.
	 *
	 * In this example an integer is stored for every key. The initial value is five and we want to
	 * increase it every time the same key is used.
	 *   map.add_or_modify(key,
	 *                     [](int *value) { *value = 5; },
	 *                     [](int *value) { (*value)++; });
	 */
	template<typename CreateValueF, typename ModifyValueF> auto add_or_modify(const Key &key, const CreateValueF &create_value, const ModifyValueF &modify_value) -> decltype(create_value(nullptr)) {
		return this->add_or_modify_as(key, create_value, modify_value);
	}
	template<typename CreateValueF, typename ModifyValueF> auto add_or_modify(Key &&key, const CreateValueF &create_value, const ModifyValueF &modify_value) -> decltype(create_value(nullptr)) {
		return this->add_or_modify_as(std::move(key), create_value, modify_value);
	}
	template<typename ForwardKey, typename CreateValueF, typename ModifyValueF> auto add_or_modify_as(ForwardKey &&key, const CreateValueF &create_value, const ModifyValueF &modify_value) -> decltype(create_value(nullptr)) {
		return this->add_or_modify__impl(std::forward<ForwardKey>(key), create_value, modify_value, hash_(key));
	}

	/**
	 * Returns a pointer to the value that corresponds to the given key. If the key is not in the
	 * map, nullptr is returned.
	 *
	 * This is similar to std::unordered_map::find.
	 */
	const Value *lookup_ptr(const Key &key) const {
		return this->lookup_ptr_as(key);
	}
	Value *lookup_ptr(const Key &key) {
		return this->lookup_ptr_as(key);
	}
	template<typename ForwardKey> const Value *lookup_ptr_as(const ForwardKey &key) const {
		const Slot *slot = this->lookup_slot_ptr(key, hash_(key));
		return (slot != nullptr) ? slot->value() : nullptr;
	}
	template<typename ForwardKey> Value *lookup_ptr_as(const ForwardKey &key) {
		return const_cast<Value *>(const_cast<const Map *>(this)->lookup_ptr_as(key));
	}

	/**
	 * Returns a reference to the value that corresponds to the given key. This invokes undefined
	 * behavior when the key is not in the map.
	 */
	const Value &lookup(const Key &key) const {
		return this->lookup_as(key);
	}
	Value &lookup(const Key &key) {
		return this->lookup_as(key);
	}
	template<typename ForwardKey> const Value &lookup_as(const ForwardKey &key) const {
		const Value *ptr = this->lookup_ptr_as(key);
		ROSE_assert(ptr != nullptr);
		return *ptr;
	}
	template<typename ForwardKey> Value &lookup_as(const ForwardKey &key) {
		Value *ptr = this->lookup_ptr_as(key);
		ROSE_assert(ptr != nullptr);
		return *ptr;
	}

	/**
	 * Returns a copy of the value that corresponds to the given key. If the key is not in the
	 * map, the provided default_value is returned.
	 */
	Value lookup_default(const Key &key, const Value &default_value) const {
		return this->lookup_default_as(key, default_value);
	}
	template<typename ForwardKey, typename... ForwardValue> Value lookup_default_as(const ForwardKey &key, ForwardValue &&...default_value) const {
		const Value *ptr = this->lookup_ptr_as(key);
		if (ptr != nullptr) {
			return *ptr;
		}
		else {
			return Value(std::forward<ForwardValue>(default_value)...);
		}
	}

	/**
	 * Returns a reference to the value corresponding to the given key. If the key is not in the map,
	 * a new key-value-pair is added and a reference to the value in the map is returned.
	 */
	Value &lookup_or_add(const Key &key, const Value &value) {
		return this->lookup_or_add_as(key, value);
	}
	Value &lookup_or_add(const Key &key, Value &&value) {
		return this->lookup_or_add_as(key, std::move(value));
	}
	Value &lookup_or_add(Key &&key, const Value &value) {
		return this->lookup_or_add_as(std::move(key), value);
	}
	Value &lookup_or_add(Key &&key, Value &&value) {
		return this->lookup_or_add_as(std::move(key), std::move(value));
	}
	template<typename ForwardKey, typename... ForwardValue> Value &lookup_or_add_as(ForwardKey &&key, ForwardValue &&...value) {
		return this->lookup_or_add__impl(std::forward<ForwardKey>(key), hash_(key), std::forward<ForwardValue>(value)...);
	}

	/**
	 * Returns a reference to the value that corresponds to the given key. If the key is not yet in
	 * the map, it will be newly added.
	 *
	 * The create_value callback is only called when the key did not exist yet. It is expected to
	 * take no parameters and return the value to be inserted.
	 */
	template<typename CreateValueF> Value &lookup_or_add_cb(const Key &key, const CreateValueF &create_value) {
		return this->lookup_or_add_cb_as(key, create_value);
	}
	template<typename CreateValueF> Value &lookup_or_add_cb(Key &&key, const CreateValueF &create_value) {
		return this->lookup_or_add_cb_as(std::move(key), create_value);
	}
	template<typename ForwardKey, typename CreateValueF> Value &lookup_or_add_cb_as(ForwardKey &&key, const CreateValueF &create_value) {
		return this->lookup_or_add_cb__impl(std::forward<ForwardKey>(key), create_value, hash_(key));
	}

	/**
	 * Returns a reference to the value that corresponds to the given key. If the key is not yet in
	 * the map, it will be newly added. The newly added value will be default constructed.
	 */
	Value &lookup_or_add_default(const Key &key) {
		return this->lookup_or_add_default_as(key);
	}
	Value &lookup_or_add_default(Key &&key) {
		return this->lookup_or_add_default_as(std::move(key));
	}
	template<typename ForwardKey> Value &lookup_or_add_default_as(ForwardKey &&key) {
		return this->lookup_or_add_cb_as(std::forward<ForwardKey>(key), []() { return Value(); });
	}

	/**
	 * Returns the key that is stored in the set that compares equal to the given key. This invokes
	 * undefined behavior when the key is not in the map.
	 */
	const Key &lookup_key(const Key &key) const {
		return this->lookup_key_as(key);
	}
	template<typename ForwardKey> const Key &lookup_key_as(const ForwardKey &key) const {
		const Slot &slot = this->lookup_slot(key, hash_(key));
		return *slot.key();
	}

	/**
	 * Returns a pointer to the key that is stored in the map that compares equal to the given key.
	 * If the key is not in the map, null is returned.
	 */
	const Key *lookup_key_ptr(const Key &key) const {
		return this->lookup_key_ptr_as(key);
	}
	template<typename ForwardKey> const Key *lookup_key_ptr_as(const ForwardKey &key) const {
		const Slot *slot = this->lookup_slot_ptr(key, hash_(key));
		if (slot == nullptr) {
			return nullptr;
		}
		return slot->key();
	}

	/**
	 * Calls the provided callback for every key-value-pair in the map. The callback is expected
	 * to take a `const Key &` as first and a `const Value &` as second parameter.
	 */
	template<typename FuncT> void foreach_item(const FuncT &func) const {
		size_type size = slots_.size();
		for (size_type i = 0; i < size; i++) {
			const Slot &slot = slots_[i];
			if (slot.is_occupied()) {
				const Key &key = *slot.key();
				const Value &value = *slot.value();
				func(key, value);
			}
		}
	}

	/* Common base class for all iterators below. */
	struct BaseIterator {
	public:
		using iterator_category = std::forward_iterator_tag;
		using difference_type = std::ptrdiff_t;

	protected:
		/* We could have separate base iterators for const and non-const iterators, but that would add
		 * more complexity than benefits right now. */
		Slot *slots_;
		size_type total_slots_;
		size_type current_slot_;

		friend Map;

	public:
		BaseIterator(const Slot *slots, const size_type total_slots, const size_type current_slot) : slots_(const_cast<Slot *>(slots)), total_slots_(total_slots), current_slot_(current_slot) {
		}

		BaseIterator &operator++() {
			while (++current_slot_ < total_slots_) {
				if (slots_[current_slot_].is_occupied()) {
					break;
				}
			}
			return *this;
		}

		BaseIterator operator++(int) {
			BaseIterator copied_iterator = *this;
			++(*this);
			return copied_iterator;
		}

		friend bool operator!=(const BaseIterator &a, const BaseIterator &b) {
			ROSE_assert(a.slots_ == b.slots_);
			ROSE_assert(a.total_slots_ == b.total_slots_);
			return a.current_slot_ != b.current_slot_;
		}

		friend bool operator==(const BaseIterator &a, const BaseIterator &b) {
			return !(a != b);
		}

	protected:
		Slot &current_slot() const {
			return slots_[current_slot_];
		}
	};

	/**
	 * A utility iterator that reduces the amount of code when implementing the actual iterators.
	 * This uses the "curiously recurring template pattern" (CRTP).
	 */
	template<typename SubIterator> class BaseIteratorRange : public BaseIterator {
	public:
		BaseIteratorRange(const Slot *slots, size_type total_slots, size_type current_slot) : BaseIterator(slots, total_slots, current_slot) {
		}

		SubIterator begin() const {
			for (size_type i = 0; i < this->total_slots_; i++) {
				if (this->slots_[i].is_occupied()) {
					return SubIterator(this->slots_, this->total_slots_, i);
				}
			}
			return this->end();
		}

		SubIterator end() const {
			return SubIterator(this->slots_, this->total_slots_, this->total_slots_);
		}
	};

	class KeyIterator final : public BaseIteratorRange<KeyIterator> {
	public:
		using value_type = Key;
		using pointer = const Key *;
		using reference = const Key &;

		KeyIterator(const Slot *slots, size_type total_slots, size_type current_slot) : BaseIteratorRange<KeyIterator>(slots, total_slots, current_slot) {
		}

		const Key &operator*() const {
			return *this->current_slot().key();
		}
	};

	class ValueIterator final : public BaseIteratorRange<ValueIterator> {
	public:
		using value_type = Value;
		using pointer = const Value *;
		using reference = const Value &;

		ValueIterator(const Slot *slots, size_type total_slots, size_type current_slot) : BaseIteratorRange<ValueIterator>(slots, total_slots, current_slot) {
		}

		const Value &operator*() const {
			return *this->current_slot().value();
		}
	};

	class MutableValueIterator final : public BaseIteratorRange<MutableValueIterator> {
	public:
		using value_type = Value;
		using pointer = Value *;
		using reference = Value &;

		MutableValueIterator(Slot *slots, size_type total_slots, size_type current_slot) : BaseIteratorRange<MutableValueIterator>(slots, total_slots, current_slot) {
		}

		Value &operator*() {
			return *this->current_slot().value();
		}
	};

	class ItemIterator final : public BaseIteratorRange<ItemIterator> {
	public:
		using value_type = Item;
		using pointer = Item *;
		using reference = Item &;

		ItemIterator(const Slot *slots, size_type total_slots, size_type current_slot) : BaseIteratorRange<ItemIterator>(slots, total_slots, current_slot) {
		}

		Item operator*() const {
			const Slot &slot = this->current_slot();
			return {*slot.key(), *slot.value()};
		}
	};

	class MutableItemIterator final : public BaseIteratorRange<MutableItemIterator> {
	public:
		using value_type = MutableItem;
		using pointer = MutableItem *;
		using reference = MutableItem &;

		MutableItemIterator(Slot *slots, size_type total_slots, size_type current_slot) : BaseIteratorRange<MutableItemIterator>(slots, total_slots, current_slot) {
		}

		MutableItem operator*() const {
			Slot &slot = this->current_slot();
			return {*slot.key(), *slot.value()};
		}
	};

	/**
	 * Allows writing a range-for loop that iterates over all keys. The iterator is invalidated, when
	 * the map is changed.
	 */
	KeyIterator keys() const {
		return KeyIterator(slots_.data(), slots_.size(), 0);
	}

	/**
	 * Returns an iterator over all values in the map. The iterator is invalidated, when the map is
	 * changed.
	 */
	ValueIterator values() const {
		return ValueIterator(slots_.data(), slots_.size(), 0);
	}

	/**
	 * Returns an iterator over all values in the map and allows you to change the values. The
	 * iterator is invalidated, when the map is changed.
	 */
	MutableValueIterator values() {
		return MutableValueIterator(slots_.data(), slots_.size(), 0);
	}

	/**
	 * Returns an iterator over all key-value-pairs in the map. The key-value-pairs are stored in a
	 * #MapItem. The iterator is invalidated, when the map is changed.
	 */
	ItemIterator items() const {
		return ItemIterator(slots_.data(), slots_.size(), 0);
	}

	/**
	 * Returns an iterator over all key-value-pairs in the map. The key-value-pairs are stored in a
	 * #MutableMapItem. The iterator is invalidated, when the map is changed.
	 *
	 * This iterator also allows you to modify the value (but not the key).
	 */
	MutableItemIterator items() {
		return MutableItemIterator(slots_.data(), slots_.size(), 0);
	}

	/**
	 * Remove the key-value-pair that the iterator is currently pointing at.
	 * It is valid to call this method while iterating over the map. However, after this method has
	 * been called, the removed element must not be accessed anymore.
	 */
	void remove(const BaseIterator &iterator) {
		Slot &slot = iterator.current_slot();
		ROSE_assert(slot.is_occupied());
		slot.remove();
		removed_slots_++;
	}

	/**
	 * Remove all key-value-pairs for that the given predicate is true and return the number of
	 * removed pairs.
	 *
	 * This is similar to std::erase_if.
	 */
	template<typename Predicate> size_type remove_if(Predicate &&predicate) {
		const size_type prev_size = this->size();
		for (Slot &slot : slots_) {
			if (slot.is_occupied()) {
				const Key &key = *slot.key();
				Value &value = *slot.value();
				if (predicate(MutableItem{key, value})) {
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
		HashTableStats stats(*this, this->keys());
		stats.print(name);
	}

	/**
	 * Return the number of key-value-pairs that are stored in the map.
	 */
	size_type size() const {
		return occupied_and_removed_slots_ - removed_slots_;
	}

	/**
	 * Returns true if there are no elements in the map.
	 *
	 * This is similar to std::unordered_map::empty.
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
	 * Returns the approximate memory requirements of the map in bytes. This becomes more exact the
	 * larger the map becomes.
	 */
	size_type size_in_bytes() const {
		return size_type(sizeof(Slot) * slots_.size());
	}

	/**
	 * Potentially resize the map such that the specified number of elements can be added without
	 * another grow operation.
	 */
	void reserve(size_type n) {
		if (usable_slots_ < n) {
			this->realloc_and_reinsert(n);
		}
	}

	/**
	 * Removes all key-value-pairs from the map.
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
	 * Removes all key-value-pairs from the map and frees any allocated memory.
	 */
	void clear_and_shrink() {
		std::destroy_at(this);
		new (this) Map(NoExceptConstructor{});
	}

	/**
	 * Get the number of collisions that the probing strategy has to go through to find the key or
	 * determine that it is not in the map.
	 */
	size_type count_collisions(const Key &key) const {
		return this->count_collisions__impl(key, hash_(key));
	}

	/**
	 * True if both maps have the same key-value-pairs.
	 */
	friend bool operator==(const Map &a, const Map &b) {
		if (a.size() != b.size()) {
			return false;
		}
		for (const Item item : a.items()) {
			const Key &key = item.key;
			const Value &value_a = item.value;
			const Value *value_b = b.lookup_ptr(key);
			if (value_b == nullptr) {
				return false;
			}
			if (value_a != *value_b) {
				return false;
			}
		}
		return true;
	}

	friend bool operator!=(const Map &a, const Map &b) {
		return !(a == b);
	}

private:
	ROSE_NOINLINE void realloc_and_reinsert(size_type min_usable_slots) {
		size_type total_slots, usable_slots;
		max_load_factor_.compute_total_and_usable_slots(SlotArray::inline_buffer_capacity(), min_usable_slots, &total_slots, &usable_slots);
		ROSE_assert(total_slots >= 1);
		const uint64_t new_slot_mask = uint64_t(total_slots) - 1;

		/**
		 * Optimize the case when the map was empty beforehand. We can avoid some copies here.
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

	void add_after_grow(Slot &old_slot, SlotArray &new_slots, uint64_t new_slot_mask) {
		uint64_t hash = old_slot.get_hash(Hash());
		SLOT_PROBING_BEGIN(ProbingStrategy, hash, new_slot_mask, slot_index) {
			Slot &slot = new_slots[slot_index];
			if (slot.is_empty()) {
				slot.occupy(std::move(*old_slot.key()), hash, std::move(*old_slot.value()));
				return;
			}
		}
		SLOT_PROBING_END();
	}

	void noexcept_reset() noexcept {
		Allocator allocator = slots_.allocator();
		this->~Map();
		new (this) Map(NoExceptConstructor(), allocator);
	}

	template<typename ForwardKey, typename... ForwardValue> void add_new__impl(ForwardKey &&key, uint64_t hash, ForwardValue &&...value) {
		ROSE_assert(!this->contains_as(key));

		this->ensure_can_add();

		MAP_SLOT_PROBING_BEGIN(hash, slot) {
			if (slot.is_empty()) {
				slot.occupy(std::forward<ForwardKey>(key), hash, std::forward<ForwardValue>(value)...);
				ROSE_assert(hash_(*slot.key()) == hash);
				occupied_and_removed_slots_++;
				return;
			}
		}
		MAP_SLOT_PROBING_END();
	}

	template<typename ForwardKey, typename... ForwardValue> bool add__impl(ForwardKey &&key, uint64_t hash, ForwardValue &&...value) {
		this->ensure_can_add();

		MAP_SLOT_PROBING_BEGIN(hash, slot) {
			if (slot.is_empty()) {
				slot.occupy(std::forward<ForwardKey>(key), hash, std::forward<ForwardValue>(value)...);
				ROSE_assert(hash_(*slot.key()) == hash);
				occupied_and_removed_slots_++;
				return true;
			}
			if (slot.contains(key, is_equal_, hash)) {
				return false;
			}
		}
		MAP_SLOT_PROBING_END();
	}

	template<typename ForwardKey, typename CreateValueF, typename ModifyValueF> auto add_or_modify__impl(ForwardKey &&key, const CreateValueF &create_value, const ModifyValueF &modify_value, uint64_t hash) -> decltype(create_value(nullptr)) {
		using CreateReturnT = decltype(create_value(nullptr));
		using ModifyReturnT = decltype(modify_value(nullptr));
		ROSE_STATIC_ASSERT((std::is_same_v<CreateReturnT, ModifyReturnT>), "Both callbacks should return the same type.");

		this->ensure_can_add();

		MAP_SLOT_PROBING_BEGIN(hash, slot) {
			if (slot.is_empty()) {
				Value *value_ptr = slot.value();
				if constexpr (std::is_void_v<CreateReturnT>) {
					create_value(value_ptr);
					slot.occupy_no_value(std::forward<ForwardKey>(key), hash);
					occupied_and_removed_slots_++;
					return;
				}
				else {
					auto &&return_value = create_value(value_ptr);
					slot.occupy_no_value(std::forward<ForwardKey>(key), hash);
					occupied_and_removed_slots_++;
					return return_value;
				}
			}
			if (slot.contains(key, is_equal_, hash)) {
				Value *value_ptr = slot.value();
				return modify_value(value_ptr);
			}
		}
		MAP_SLOT_PROBING_END();
	}

	template<typename ForwardKey, typename CreateValueF> Value &lookup_or_add_cb__impl(ForwardKey &&key, const CreateValueF &create_value, uint64_t hash) {
		this->ensure_can_add();

		MAP_SLOT_PROBING_BEGIN(hash, slot) {
			if (slot.is_empty()) {
				slot.occupy(std::forward<ForwardKey>(key), hash, create_value());
				ROSE_assert(hash_(*slot.key()) == hash);
				occupied_and_removed_slots_++;
				return *slot.value();
			}
			if (slot.contains(key, is_equal_, hash)) {
				return *slot.value();
			}
		}
		MAP_SLOT_PROBING_END();
	}

	template<typename ForwardKey, typename... ForwardValue> Value &lookup_or_add__impl(ForwardKey &&key, uint64_t hash, ForwardValue &&...value) {
		this->ensure_can_add();

		MAP_SLOT_PROBING_BEGIN(hash, slot) {
			if (slot.is_empty()) {
				slot.occupy(std::forward<ForwardKey>(key), hash, std::forward<ForwardValue>(value)...);
				ROSE_assert(hash_(*slot.key()) == hash);
				occupied_and_removed_slots_++;
				return *slot.value();
			}
			if (slot.contains(key, is_equal_, hash)) {
				return *slot.value();
			}
		}
		MAP_SLOT_PROBING_END();
	}

	template<typename ForwardKey, typename... ForwardValue> bool add_overwrite__impl(ForwardKey &&key, uint64_t hash, ForwardValue &&...value) {
		auto create_func = [&](Value *ptr) {
			new (static_cast<void *>(ptr)) Value(std::forward<ForwardValue>(value)...);
			return true;
		};
		auto modify_func = [&](Value *ptr) {
			*ptr = Value(std::forward<ForwardValue>(value)...);
			return false;
		};
		return this->add_or_modify__impl(std::forward<ForwardKey>(key), create_func, modify_func, hash);
	}

	template<typename ForwardKey> const Slot &lookup_slot(const ForwardKey &key, const uint64_t hash) const {
		ROSE_assert(this->contains_as(key));
		MAP_SLOT_PROBING_BEGIN(hash, slot) {
			if (slot.contains(key, is_equal_, hash)) {
				return slot;
			}
		}
		MAP_SLOT_PROBING_END();
	}

	template<typename ForwardKey> Slot &lookup_slot(const ForwardKey &key, const uint64_t hash) {
		return const_cast<Slot &>(const_cast<const Map *>(this)->lookup_slot(key, hash));
	}

	template<typename ForwardKey> const Slot *lookup_slot_ptr(const ForwardKey &key, const uint64_t hash) const {
		MAP_SLOT_PROBING_BEGIN(hash, slot) {
			if (slot.contains(key, is_equal_, hash)) {
				return &slot;
			}
			if (slot.is_empty()) {
				return nullptr;
			}
		}
		MAP_SLOT_PROBING_END();
	}

	template<typename ForwardKey> Slot *lookup_slot_ptr(const ForwardKey &key, const uint64_t hash) {
		return const_cast<Slot *>(const_cast<const Map *>(this)->lookup_slot_ptr(key, hash));
	}

	template<typename ForwardKey> size_type count_collisions__impl(const ForwardKey &key, uint64_t hash) const {
		size_type collisions = 0;

		MAP_SLOT_PROBING_BEGIN(hash, slot) {
			if (slot.contains(key, is_equal_, hash)) {
				return collisions;
			}
			if (slot.is_empty()) {
				return collisions;
			}
			collisions++;
		}
		MAP_SLOT_PROBING_END();
	}

	void ensure_can_add() {
		if (occupied_and_removed_slots_ >= usable_slots_) {
			this->realloc_and_reinsert(this->size() + 1);
			ROSE_assert(occupied_and_removed_slots_ < usable_slots_);
		}
	}
};

}  // namespace rose

#endif // LIB_MAP_HH
