#include "MEM_alloc.h"

#include "DNA_defines.h"

#include "LIB_assert.h"
#include "LIB_bitmap.h"
#include "LIB_ghash.h"
#include "LIB_string.h"
#include "LIB_string_utils.h"
#include "LIB_utildefines.h"

#include "LIB_map.hh"
#include "LIB_set.hh"
#include "LIB_string_ref.hh"

#include "KER_namemap.h"

/* Assumes and ensure that the suffix number can never go beyond 1 billion. */
#define MAX_NUMBER 1000000000
/* We do not want to get "name.000", so minimal number is 1. */
#define MIN_NUMBER 1

static bool id_name_final_build(char *name, size_t maxlength, char *base_name, size_t base_name_length, size_t number) {
	char number_str[22]; /** dot + twenty digits + null terminator. */
	size_t number_str_len = LIB_snprintf(number_str, ARRAY_SIZE(number_str), ".%.3zu", number);
	
	if(base_name_length + number_str_len >= MAX_NAME || number >= MAX_NUMBER) {
		if(base_name_length + number_str_len >= MAX_NAME) {
			base_name_length = MAX_NAME - number_str_len - 1;
		} else {
			base_name_length--;
		}
		base_name[base_name_length] = '\0';
		return false;
	}
	
	LIB_strncpy(name + base_name_length, number_str, number_str_len + 1);
	return true;
}

typedef struct UniqueName_Key {
	char name[MAX_NAME];

	uint64_t hash() const {
		return LIB_ghashutil_strhash_n(name, MAX_NAME);
	}
	bool operator==(const UniqueName_Key &other) const {
		return !LIB_ghashutil_strcmp(name, other.name);
	}
} UniqueName_Key;

typedef struct UniqueName_Value {
	static constexpr size_t max_exact_tracking = 1024;
	ROSE_BITMAP_DECLARE(mask, max_exact_tracking);
	size_t max_value = 0;

	void mark_used(size_t number) {
		if (number >= 0 && number < max_exact_tracking) {
			ROSE_BITMAP_ENABLE(mask, number);
		}
		if (number < MAX_NUMBER) {
			max_value = ROSE_MAX(max_value, number);
		}
	}

	void mark_unused(size_t number) {
		if (number >= 0 && number < max_exact_tracking) {
			ROSE_BITMAP_DISABLE(mask, number);
		}
		if (number > 0 && number == max_value) {
			--max_value;
		}
	}

	bool use_if_unused(size_t number) {
		if (number >= 0 && number < max_exact_tracking) {
			if (!ROSE_BITMAP_TEST_BOOL(mask, number)) {
				ROSE_BITMAP_ENABLE(mask, number);
				max_value = ROSE_MAX(max_value, number);
				return true;
			}
		}
		return false;
	}

	size_t use_smallest_unused() {
		unsigned int prev_first = mask[0];
		mask[0] |= 1;

		size_t result = LIB_bitmap_find_first_unset(mask, max_exact_tracking);
		if (result >= 0) {
			ROSE_BITMAP_ENABLE(mask, result);
			max_value = ROSE_MAX(max_value, result);
		}

		mask[0] = (mask[0] & (~0u << 1)) | (prev_first & 1);
		return result;
	}
} UniqueName_Value;

using namespace rose;

typedef struct NameMapBucket {
	Set<UniqueName_Key> full_names;
	Map<UniqueName_Key, UniqueName_Value> base_name_to_num_suffix;
} NameMapBucket;

typedef struct NameMapInternal {
	Vector<NameMapBucket> buckets;

	NameMapBucket *bucket(size_t index) {
		return (index < buckets.size()) ? &buckets[index] : nullptr;
	}
	const NameMapBucket *bucket(size_t index) const {
		return (index < buckets.size()) ? &buckets[index] : nullptr;
	}
} NameMapInternal;

NameMap *KER_namemap_new(size_t buckets) {
	NameMapInternal *map = MEM_new<NameMapInternal>("rose::kernel::NameMap");

	map->buckets.resize(buckets);

	return reinterpret_cast<NameMap *>(map);
}

void KER_namemap_free(NameMap *namemap) {
	NameMapInternal *map = reinterpret_cast<NameMapInternal *>(namemap);

	MEM_delete(map);
}

bool KER_namemap_check_valid(const NameMap *namemap, size_t bucket_index, const char *name) {
	const NameMapInternal *map = reinterpret_cast<const NameMapInternal *>(namemap);
	
	UniqueName_Key key;
	LIB_strncpy(key.name, name, ARRAY_SIZE(key.name));

	const NameMapBucket *bucket = map->bucket(bucket_index);
	if(bucket) {
		return bucket->full_names.contains(key) == false;
	}
	return false;
}

bool KER_namemap_ensure_valid(NameMap *namemap, size_t bucket_index, char *r_name) {
	NameMapInternal *map = reinterpret_cast<NameMapInternal *>(namemap);
	
	NameMapBucket *bucket = map->bucket(bucket_index);
	if(bucket) {
		UniqueName_Key key;
		LIB_strncpy(key.name, r_name, ARRAY_SIZE(key.name));
		
		const bool has_dup = bucket->full_names.contains(key);
		
		size_t number = MIN_NUMBER;
		size_t base_name_length = LIB_string_split_name_number(r_name, '.', key.name, &number);
		
		bool added_new = false;
		UniqueName_Value &val = bucket->base_name_to_num_suffix.lookup_or_add_cb(key, [&]() {
			added_new = true;
			return UniqueName_Value();
		});
		
		if(added_new || !has_dup) {
			val.mark_used(number);
			
			if(!has_dup) {
				LIB_strncpy(key.name, r_name, ARRAY_SIZE(key.name));
				bucket->full_names.add(key);
			}
			
			return false;
		}
		
		size_t number_to_use = -1;
		if(val.use_if_unused(number)) {
			number_to_use = number;
		} else {
			number_to_use = val.use_smallest_unused();
			
			if(number_to_use == LIB_NPOS) {
				if(number >= MIN_NUMBER && number > val.max_value) {
					val.max_value = number;
					number_to_use = number;
				} else {
					val.max_value++;
					number_to_use = val.max_value;
				}
			}
		}
		
		ROSE_assert(number_to_use >= MIN_NUMBER);
		if(id_name_final_build(r_name, MAX_NAME, key.name, base_name_length, number_to_use)) {
			LIB_strncpy(key.name, r_name, ARRAY_SIZE(key.name));
			bucket->full_names.add(key);
		}
		
		return true;
	}
	
	ROSE_assert_unreachable();
	return false;
}

bool KER_namemap_remove_entry(NameMap *namemap, size_t bucket_index, const char *name) {
	NameMapInternal *map = reinterpret_cast<NameMapInternal *>(namemap);
	
	NameMapBucket *bucket = map->bucket(bucket_index);
	if(bucket) {
		UniqueName_Key key;
		LIB_strncpy(key.name, name, ARRAY_SIZE(key.name));
		
		bucket->full_names.remove(key);
		
		size_t number = MIN_NUMBER;
		LIB_string_split_name_number(name, '.', key.name, &number);
		UniqueName_Value *val = bucket->base_name_to_num_suffix.lookup_ptr(key);
		if(val == nullptr) {
			return false;
		}
		if(number == 0 && val->max_value == 0) {
			bucket->base_name_to_num_suffix.remove(key);
		} else {
			val->mark_unused(number);
		}
		return true;
	}
	
	ROSE_assert_unreachable();
	return false;
}

void KER_namemap_clear_ex(NameMap *namemap, size_t bucket_index) {
	NameMapInternal *map = reinterpret_cast<NameMapInternal *>(namemap);
	
	NameMapBucket *bucket = map->bucket(bucket_index);
	if(bucket) {
		bucket->full_names.clear();
		bucket->base_name_to_num_suffix.clear();
	}
}

void KER_namemap_clear(NameMap *namemap) {
	NameMapInternal *map = reinterpret_cast<NameMapInternal *>(namemap);
	
	for(size_t index = 0; index < map->buckets.size(); index++) {
		KER_namemap_clear_ex(namemap, index);
	}
}
