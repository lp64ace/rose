#ifndef LIB_UNROLL_HH
#define LIB_UNROLL_HH

#include "LIB_utildefines.h"

namespace rose {

template<class Fn, size_t... I> void unroll_impl(Fn fn, std::index_sequence<I...> /*indices*/) {
	(fn(I), ...);
}

/**
 * Variadic templates are used to unroll loops manually. This helps GCC avoid branching during math
 * operations and makes the code generation more explicit and predictable. Unrolling should always
 * be worth it because the vector size is expected to be small.
 */
template<int N, class Fn> void unroll(Fn fn) {
	unroll_impl(fn, std::make_index_sequence<N>());
}

}  // namespace rose

#endif // LIB_UNROLL_HH
