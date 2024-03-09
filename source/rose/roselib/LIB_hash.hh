#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "LIB_string_ref.hh"

#include "LIB_hash.h"
#include "LIB_utildefines.h"

namespace rose {

template<typename T> struct DefaultHash {
	uint64_t operator()(const T &value) const {
		if constexpr (std::is_enum_v<T>) {
			return uint64_t(value);
		}
		else {
			return value.hash();
		}
	}

	template<typename U> uint64_t operator()(const U &value) const {
		return T::hash_as(value);
	}
};

template<typename T> struct DefaultHash<const T> {
	uint64_t operator()(const T &value) const {
		return DefaultHash<T>{}(value);
	}
};

#define TRIVIAL_DEFAULT_INT_HASH(TYPE) \
	template<> struct DefaultHash<TYPE> { \
		uint64_t operator()(TYPE value) const { \
			return uint64_t(value); \
		} \
	}

TRIVIAL_DEFAULT_INT_HASH(int8_t);
TRIVIAL_DEFAULT_INT_HASH(uint8_t);
TRIVIAL_DEFAULT_INT_HASH(int16_t);
TRIVIAL_DEFAULT_INT_HASH(uint16_t);
TRIVIAL_DEFAULT_INT_HASH(int32_t);
TRIVIAL_DEFAULT_INT_HASH(uint32_t);
TRIVIAL_DEFAULT_INT_HASH(int64_t);
TRIVIAL_DEFAULT_INT_HASH(uint64_t);

template<> struct DefaultHash<float> {
	uint64_t operator()(float value) const {
		return *reinterpret_cast<uint32_t *>(&value);
	}
};

template<> struct DefaultHash<double> {
	uint64_t operator()(double value) const {
		return *reinterpret_cast<uint64_t *>(&value);
	}
};

template<> struct DefaultHash<bool> {
	uint64_t operator()(bool value) const {
		return uint64_t((value != false) * 1298191);
	}
};

ROSE_INLINE uint64_t hash_string(StringRef str) {
	uint64_t hash = 5381;
	for (char c : str) {
		hash = hash * 33 + c;
	}
	return hash;
}

template<> struct DefaultHash<std::string> {
	/**
	 * Take a #StringRef as parameter to support heterogeneous lookups in hash table implementations
	 * when std::string is used as key.
	 */
	uint64_t operator()(StringRef value) const {
		return hash_string(value);
	}
};

template<> struct DefaultHash<StringRef> {
	uint64_t operator()(StringRef value) const {
		return hash_string(value);
	}
};

template<> struct DefaultHash<StringRefNull> {
	uint64_t operator()(StringRef value) const {
		return hash_string(value);
	}
};

template<> struct DefaultHash<std::string_view> {
	uint64_t operator()(StringRef value) const {
		return hash_string(value);
	}
};

template<typename T> struct DefaultHash<T *> {
	uint64_t operator()(const T *value) const {
		uintptr_t ptr = uintptr_t(value);
		uint64_t hash = uint64_t(ptr >> 4);
		return hash;
	}
};

template<typename T> uint64_t get_default_hash(const T &v) {
	return DefaultHash<std::decay_t<T>>{}(v);
}

}  // namespace rose
