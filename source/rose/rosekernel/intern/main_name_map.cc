#include "LIB_bitmap.h"
#include "LIB_bit_span_ops.hh"
#include "LIB_bit_vector.hh"
#include "LIB_map.hh"
#include "LIB_set.hh"
#include "LIB_ghash.h"
#include "LIB_string_utils.h"

#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_main.h"
#include "KER_main_name_map.h"

constexpr int MAX_NAME = ARRAY_SIZE(ID::name) - 2;
/* Assumes and ensure that the suffix number can never go beyond 1 billion. */
constexpr int MAX_NUMBER = 999999999;
/* Value representing that there is no available number. Must be negative value. */
constexpr int NO_AVAILABLE_NUMBER = -1;

/**
 * Helper building final ID name from given base_name and number.
 *
 * If everything goes well and we do generate a valid final ID name in given name, we return
 * true. In case the final name would overflow the allowed ID name length, or given number is
 * bigger than maximum allowed value, we truncate further the base_name (and given name, which is
 * assumed to have the same 'base_name' part), and return false.
 */
static bool id_name_final_build(char *name, char *base_name, size_t base_name_len, int number) {
	char number_str[11]; /* Dot + nine digits + NULL terminator. */
	size_t number_str_len = LIB_strnformat(number_str, ARRAY_SIZE(number_str), ".%.3d", number);

	/* If the number would lead to an overflow of the maximum ID name length, we need to truncate
	 * the base name part and do all the number checks again. */
	if (base_name_len + number_str_len >= MAX_NAME || number >= MAX_NUMBER) {
		if (base_name_len + number_str_len >= MAX_NAME) {
			base_name_len = MAX_NAME - number_str_len - 1;
		}
		else {
			base_name_len--;
		}
		base_name[base_name_len] = '\0';

		/* Also truncate orig name, and start the whole check again. */
		name[base_name_len] = '\0';
		return false;
	}

	/* We have our final number, we can put it in name and exit the function. */
	LIB_strncpy(name + base_name_len, MAX_NAME - base_name_len, number_str, number_str_len);
	return true;
}

/* Key used in set/map lookups: just a string name. */
struct UniqueName_Key {
	char name[MAX_NAME];
	uint64_t hash() const {
		return LIB_ghashutil_strhash_n(name, MAX_NAME);
	}
	bool operator==(const UniqueName_Key &o) const {
		return !LIB_ghashutil_strcmp(name, o.name);
	}
};

/* Tracking of used numeric suffixes. For each base name:
 *
 * - Exactly track which of the lowest 1024 suffixes are in use,
 *   whenever there is a name collision we pick the lowest "unused"
 *   one. This is done with a bit map.
 * - Above 1024, do not track them exactly, just track the maximum
 *   suffix value seen so far. Upon collision, assign number that is
 *   one larger.
 */
struct UniqueName_Value {
	static constexpr uint max_exact_tracking = 1024;
	ROSE_BITMAP_DECLARE(mask, max_exact_tracking);
	int max_value = 0;

	void mark_used(int number) {
		if (number >= 0 && number < max_exact_tracking) {
			ROSE_BITMAP_ENABLE(mask, number);
		}
		if (number < MAX_NUMBER) {
			max_value = ROSE_MAX(max_value, number);
		}
	}

	void mark_unused(int number) {
		if (number >= 0 && number < max_exact_tracking) {
			ROSE_BITMAP_DISABLE(mask, number);
		}
		if (number > 0 && number == max_value) {
			--max_value;
		}
	}

	bool use_if_unused(int number) {
		if (number >= 0 && number < max_exact_tracking) {
			if (!ROSE_BITMAP_TEST_BOOL(mask, number)) {
				ROSE_BITMAP_ENABLE(mask, number);
				max_value = ROSE_MAX(max_value, number);
				return true;
			}
		}
		return false;
	}

	int use_smallest_unused() {
		/* Find the smallest available one <1k.
		 * However we never want to pick zero ("none") suffix, even if it is
		 * available, e.g. if Foo.001 was used and we want to create another
		 * Foo.001, we should return Foo.002 and not Foo.
		 * So while searching, mark #0 as "used" to make sure we don't find it,
		 * and restore the value afterwards. */

		BitMap prev_first = mask[0];
		mask[0] |= 1;
		int result = LIB_bitmap_find_first_unset(mask, max_exact_tracking);
		if (result >= 0) {
			ROSE_BITMAP_ENABLE(mask, result);
			max_value = ROSE_MAX(max_value, result);
		}
		mask[0] |= prev_first & 1; /* Restore previous value of #0 bit. */
		return result;
	}
};

/* Tracking of names for a single ID type. */
struct UniqueName_TypeMap {
	/* Set of full names that are in use. */
	rose::Set<UniqueName_Key> full_names;
	/* For each base name (i.e. without numeric suffix), track the
	 * numeric suffixes that are in use. */
	rose::Map<UniqueName_Key, UniqueName_Value> base_name_to_num_suffix;
};

struct UniqueName_Map {
	UniqueName_TypeMap type_maps[INDEX_ID_MAX];

	UniqueName_TypeMap *find_by_type(short id_type) {
		int index = KER_idtype_idcode_to_index(id_type);
		return index >= 0 ? &type_maps[index] : nullptr;
	}
};

ROSE_INLINE void main_namemap_populate(UniqueName_Map *name_map, struct Main *main, ID *ignore_id) {
	ROSE_assert_msg(name_map != nullptr, "name_map should not be null");
	for (UniqueName_TypeMap &type_map : name_map->type_maps) {
		type_map.base_name_to_num_suffix.clear();
	}
	Library *library = ignore_id->lib;
	ID *id;
	FOREACH_MAIN_ID_BEGIN(main, id) {
		if ((id == ignore_id) || (id->lib != library)) {
			continue;
		}
		UniqueName_TypeMap *type_map = name_map->find_by_type(GS(id->name));
		ROSE_assert(type_map != nullptr);

		/* Insert the full name into the set. */
		UniqueName_Key key;
		LIB_strcpy(key.name, MAX_NAME, id->name + 2);
		type_map->full_names.add(key);

		/* Get the name and number parts ("name.number"). */
		int number = 1;
		LIB_string_split_name_number(id->name + 2, '.', key.name, &number);

		/* Get and update the entry for this base name. */
		UniqueName_Value &val = type_map->base_name_to_num_suffix.lookup_or_add_default(key);
		val.mark_used(number);
	}
	FOREACH_MAIN_ID_END;
}

/* Get the name map object used for the given Main/ID.
 * Lazily creates and populates the contents of the name map, if ensure_created is true.
 * NOTE: if the contents are populated, the name of the given ID itself is not added. */
ROSE_INLINE UniqueName_Map *get_namemap_for(Main *main, ID *id, bool ensure_created) {
	if (id->lib != nullptr) {
		if (ensure_created && id->lib->runtime.name_map == nullptr) {
			id->lib->runtime.name_map = MEM_new<UniqueName_Map>("UniqueName_Map");
			main_namemap_populate(id->lib->runtime.name_map, main, id);
		}
		return id->lib->runtime.name_map;
	}
	if (ensure_created && main->name_map == nullptr) {
		main->name_map = MEM_new<UniqueName_Map>("UniqueName_Map");
		main_namemap_populate(main->name_map, main, id);
	}
	return main->name_map;
}

bool KER_main_name_map_get_unique_name(struct Main *main, struct ID *id, char *r_name) {
	UniqueName_Map *name_map = get_namemap_for(main, id, true);
	ROSE_assert(name_map != nullptr);
	ROSE_assert(LIB_strlen(r_name) < MAX_NAME);
	UniqueName_TypeMap *type_map = name_map->find_by_type(GS(id->name));
	ROSE_assert(type_map != nullptr);

	bool is_name_changed = false;

	UniqueName_Key key;
	while (true) {
		/* Check if the full original name has a duplicate. */
		LIB_strcpy(key.name, MAX_NAME, r_name);
		const bool has_dup = type_map->full_names.contains(key);

		/* Get the name and number parts ("name.number"). */
		int number = 1;
		size_t base_name_len = LIB_string_split_name_number(r_name, '.', key.name, &number);

		bool added_new = false;
		UniqueName_Value &val = type_map->base_name_to_num_suffix.lookup_or_add_cb(key, [&]() {
			added_new = true;
			return UniqueName_Value();
		});
		if (added_new || !has_dup) {
			/* This base name is not used at all yet, or the full original
			 * name has no duplicates. The latter could happen if splitting
			 * by number would produce the same values, for different name
			 * strings (e.g. Foo.001 and Foo.1). */
			val.mark_used(number);

			if (!has_dup) {
				LIB_strcpy(key.name, MAX_NAME, r_name);
				type_map->full_names.add(key);
			}
			return is_name_changed;
		}

		/* The base name is already used. But our number suffix might not be used yet. */
		int number_to_use = -1;
		if (val.use_if_unused(number)) {
			/* Our particular number suffix is not used yet: use it. */
			number_to_use = number;
		}
		else {
			/* Find lowest free under 1k and use it. */
			number_to_use = val.use_smallest_unused();

			/* Did not find one under 1k. */
			if (number_to_use == -1) {
				if (number >= 1 && number > val.max_value) {
					val.max_value = number;
					number_to_use = number;
				}
				else {
					val.max_value++;
					number_to_use = val.max_value;
				}
			}
		}

		/* Name had to be truncated, or number too large: mark
		 * the output name as definitely changed, and proceed with the
		 * truncated name again. */
		is_name_changed = true;

		/* Try to build final name from the current base name and the number.
		 * Note that this can fail due to too long base name, or a too large number,
		 * in which case it will shorten the base name, and we'll start again. */
		ROSE_assert(number_to_use >= 1);
		if (id_name_final_build(r_name, key.name, base_name_len, number_to_use)) {
			/* All good, add final name to the set. */
			LIB_strcpy(key.name, MAX_NAME, r_name);
			type_map->full_names.add(key);
			break;
		}
	}
	return is_name_changed;
}

void KER_main_name_map_remove_id(Main *main, ID *id, const char *name) {
	/* Name is empty or not initialized yet, nothing to remove. */
	if (name[0] == '\0') {
		return;
	}

	struct UniqueName_Map *name_map = get_namemap_for(main, id, false);
	if (name_map == nullptr) {
		return;
	}
	ROSE_assert(strlen(name) < MAX_NAME);
	UniqueName_TypeMap *type_map = name_map->find_by_type(GS(id->name));
	ROSE_assert(type_map != nullptr);

	UniqueName_Key key;
	/* Remove full name from the set. */
	LIB_strcpy(key.name, MAX_NAME, name);
	type_map->full_names.remove(key);

	int number = 1;
	LIB_string_split_name_number(name, '.', key.name, &number);
	UniqueName_Value *val = type_map->base_name_to_num_suffix.lookup_ptr(key);
	if (val == nullptr) {
		return;
	}
	if (number == 0 && val->max_value == 0) {
		/* This was the only base name usage, remove whole key. */
		type_map->base_name_to_num_suffix.remove(key);
		return;
	}
	val->mark_unused(number);
}

void KER_main_name_map_clear(Main *main) {
	for (Main *main_iter = main; main_iter != nullptr; main_iter = main_iter->next) {
		if (main_iter->name_map != nullptr) {
			KER_main_namemap_destroy(&main_iter->name_map);
		}
		for (Library *lib_iter = static_cast<Library *>(main_iter->libraries.first); lib_iter != nullptr; lib_iter = static_cast<Library *>(lib_iter->id.next)) {
			if (lib_iter->runtime.name_map != nullptr) {
				KER_main_namemap_destroy(&lib_iter->runtime.name_map);
			}
		}
	}
}

void KER_main_namemap_destroy(UniqueName_Map **name_map) {
	MEM_delete<UniqueName_Map>(*name_map);
	*name_map = nullptr;
}
