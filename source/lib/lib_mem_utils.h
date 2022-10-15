#pragma once

#include <memory>
#include <new>
#include <type_traits>

#if __cplusplus >= 201703L
template<typename T> inline constexpr bool is_trivial_extended_v = std::is_trivial_v<T>;
template<typename T> inline constexpr bool is_trivially_destructible_extended_v = is_trivial_extended_v<T> || std::is_trivially_destructible_v<T>;
template<typename T> inline constexpr bool is_trivially_copy_constructible_extended_v = is_trivial_extended_v<T> || std::is_trivially_copy_constructible_v<T>;
template<typename T> inline constexpr bool is_trivially_move_constructible_extended_v = is_trivial_extended_v<T> || std::is_trivially_move_constructible_v<T>;
#else
template<typename T> constexpr bool is_trivial_extended_v = std::is_trivial_v<T>;
template<typename T> constexpr bool is_trivially_destructible_extended_v = is_trivial_extended_v<T> || std::is_trivially_destructible_v<T>;
template<typename T> constexpr bool is_trivially_copy_constructible_extended_v = is_trivial_extended_v<T> || std::is_trivially_copy_constructible_v<T>;
template<typename T> constexpr bool is_trivially_move_constructible_extended_v = is_trivial_extended_v<T> || std::is_trivially_move_constructible_v<T>;
#endif

template<typename T> void destruct_n ( T *ptr , size_t number ) {
	static_assert ( std::is_nothrow_destructible_v<T> , "This should be true for all types. Destructors are noexcept by default." );
	/* This is not strictly necessary, because the loop below will be optimized away anyway. It is
	* nice to make behavior this explicitly, though. */
	if ( !is_trivially_destructible_extended_v<T> ) {
		for ( size_t index = 0; index < number; index++ ) {
			ptr [ index ].~T ( );
		}
	}
}

template<typename T> void default_construct_n ( T *ptr , size_t n ) {
	/* This is not strictly necessary, because the loop below will be optimized away anyway. It is
	* nice to make behavior this explicitly, though. */
	if ( !std::is_trivially_constructible_v<T> ) {
		size_t current = 0;
		try {
			for ( ; current < n; current++ ) {
				new ( static_cast< void * >( ptr + current ) ) T;
			}
		} catch ( ... ) {
			destruct_n ( ptr , current );
			throw;
		}
	}
}

template<typename T> void initialized_copy_n ( const T *src , size_t n , T *dst ) {
	for ( size_t i = 0; i < n; i++ ) {
		dst [ i ] = src [ i ];
	}
}

template<typename T> void uninitialized_copy_n ( const T *src , size_t n , T *dst ) {
	size_t current = 0;
	try {
		for ( ; current < n; current++ ) {
			new ( static_cast< void * >( dst + current ) ) T ( src [ current ] );
		}
	} catch ( ... ) {
		destruct_n ( dst , current );
		throw;
	}
}

template<typename From , typename To> void uninitialized_convert_n ( const From *src , size_t n , To *dst ) {
	size_t current = 0;
	try {
		for ( ; current < n; current++ ) {
			new ( static_cast< void * >( dst + current ) ) To ( static_cast< To >( src [ current ] ) );
		}
	} catch ( ... ) {
		destruct_n ( dst , current );
		throw;
	}
}

template<typename T> void initialized_move_n ( T *src , size_t n , T *dst ) {
	for ( size_t i = 0; i < n; i++ ) {
		dst [ i ] = std::move ( src [ i ] );
	}
}

template<typename T> void uninitialized_move_n ( T *src , size_t n , T *dst ) {
	size_t current = 0;
	try {
		for ( ; current < n; current++ ) {
			new ( static_cast< void * >( dst + current ) ) T ( std::move ( src [ current ] ) );
		}
	} catch ( ... ) {
		destruct_n ( dst , current );
		throw;
	}
}

template<typename T> void initialized_relocate_n ( T *src , size_t n , T *dst ) {
	initialized_move_n ( src , n , dst );
	destruct_n ( src , n );
}

template<typename T> void uninitialized_relocate_n ( T *src , size_t n , T *dst ) {
	uninitialized_move_n ( src , n , dst );
	destruct_n ( src , n );
}

template<typename T> void initialized_fill_n ( T *dst , size_t n , const T &value ) {
	for ( size_t i = 0; i < n; i++ ) {
		dst [ i ] = value;
	}
}

template<typename T> void uninitialized_fill_n ( T *dst , size_t n , const T &value ) {
	size_t current = 0;
	try {
		for ( ; current < n; current++ ) {
			new ( static_cast< void * >( dst + current ) ) T ( value );
		}
	} catch ( ... ) {
		destruct_n ( dst , current );
		throw;
	}
}
