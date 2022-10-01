#pragma once

#include <stdint.h>

#define ARRAY_SIZE(a)	(sizeof(a)/sizeof((a)[0]))

#define UNPACK2(a) ((a)[0]), ((a)[1])
#define UNPACK3(a) UNPACK2(a), ((a)[2])
#define UNPACK4(a) UNPACK3(a), ((a)[3])

#define SWAP(type, a, b) \
  { \
    type sw_ap; \
    sw_ap = (a); \
    (a) = (b); \
    (b) = sw_ap; \
  } \
  (void)0

#define CLAMP(a, b, c) \
  { \
    if ((a) < (b)) { \
      (a) = (b); \
    } \
    else if ((a) > (c)) { \
      (a) = (c); \
    } \
  } \
  (void)0

#define _VA_NARGS_GLUE(x, y) x y
#define _VA_NARGS_RETURN_COUNT(\
  _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, \
  _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, \
  _33_, _34_, _35_, _36_, _37_, _38_, _39_, _40_, _41_, _42_, _43_, _44_, _45_, _46_, _47_, _48_, \
  _49_, _50_, _51_, _52_, _53_, _54_, _55_, _56_, _57_, _58_, _59_, _60_, _61_, _62_, _63_, _64_, \
  count, ...) count
#define _VA_NARGS_EXPAND(args) _VA_NARGS_RETURN_COUNT args
#define _VA_NARGS_OVERLOAD_MACRO2(name, count) name##count
#define _VA_NARGS_OVERLOAD_MACRO1(name, count) _VA_NARGS_OVERLOAD_MACRO2(name, count)
#define _VA_NARGS_OVERLOAD_MACRO(name,  count) _VA_NARGS_OVERLOAD_MACRO1(name, count)
/* --- expose for re-use --- */
/* 64 args max */
#define VA_NARGS_COUNT(...) _VA_NARGS_EXPAND((__VA_ARGS__, \
  64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, \
  48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, \
  32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, \
  16, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2, 1, 0))
#define VA_NARGS_CALL_OVERLOAD(name, ...) \
  _VA_NARGS_GLUE(_VA_NARGS_OVERLOAD_MACRO(name, VA_NARGS_COUNT(__VA_ARGS__)), (__VA_ARGS__))

/* -------------------------------------------------------------------- */
/** \name Equal to Any Element (ELEM) Macro
 * \{ */

 /* Manual line breaks for readability. */
 /* clang-format off */

 /* ELEM#(v, ...): is the first arg equal any others? */
 /* internal helpers. */
#define _VA_ELEM2(v, a) ((v) == (a))
#define _VA_ELEM3(v, a, b) \
  (_VA_ELEM2(v, a) || _VA_ELEM2(v, b))
#define _VA_ELEM4(v, a, b, c) \
  (_VA_ELEM3(v, a, b) || _VA_ELEM2(v, c))
#define _VA_ELEM5(v, a, b, c, d) \
  (_VA_ELEM4(v, a, b, c) || _VA_ELEM2(v, d))
#define _VA_ELEM6(v, a, b, c, d, e) \
  (_VA_ELEM5(v, a, b, c, d) || _VA_ELEM2(v, e))
#define _VA_ELEM7(v, a, b, c, d, e, f) \
  (_VA_ELEM6(v, a, b, c, d, e) || _VA_ELEM2(v, f))
#define _VA_ELEM8(v, a, b, c, d, e, f, g) \
  (_VA_ELEM7(v, a, b, c, d, e, f) || _VA_ELEM2(v, g))
#define _VA_ELEM9(v, a, b, c, d, e, f, g, h) \
  (_VA_ELEM8(v, a, b, c, d, e, f, g) || _VA_ELEM2(v, h))
#define _VA_ELEM10(v, a, b, c, d, e, f, g, h, i) \
  (_VA_ELEM9(v, a, b, c, d, e, f, g, h) || _VA_ELEM2(v, i))
#define _VA_ELEM11(v, a, b, c, d, e, f, g, h, i, j) \
  (_VA_ELEM10(v, a, b, c, d, e, f, g, h, i) || _VA_ELEM2(v, j))
#define _VA_ELEM12(v, a, b, c, d, e, f, g, h, i, j, k) \
  (_VA_ELEM11(v, a, b, c, d, e, f, g, h, i, j) || _VA_ELEM2(v, k))
#define _VA_ELEM13(v, a, b, c, d, e, f, g, h, i, j, k, l) \
  (_VA_ELEM12(v, a, b, c, d, e, f, g, h, i, j, k) || _VA_ELEM2(v, l))
#define _VA_ELEM14(v, a, b, c, d, e, f, g, h, i, j, k, l, m) \
  (_VA_ELEM13(v, a, b, c, d, e, f, g, h, i, j, k, l) || _VA_ELEM2(v, m))
#define _VA_ELEM15(v, a, b, c, d, e, f, g, h, i, j, k, l, m, n) \
  (_VA_ELEM14(v, a, b, c, d, e, f, g, h, i, j, k, l, m) || _VA_ELEM2(v, n))
#define _VA_ELEM16(v, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) \
  (_VA_ELEM15(v, a, b, c, d, e, f, g, h, i, j, k, l, m, n) || _VA_ELEM2(v, o))
#define _VA_ELEM17(v, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
  (_VA_ELEM16(v, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) || _VA_ELEM2(v, p))
/* clang-format on */

/* reusable ELEM macro */

#define ELEM(...) VA_NARGS_CALL_OVERLOAD(_VA_ELEM, __VA_ARGS__)

/* -------------------------------------------------------------------- */
/** \name Maximum element.
 * \{ */

#define _VA_MAX2(a,b)		(((a)>(b))?(a):(b))
#define _VA_MAX3(a,b,c)		(_VA_MAX2(_VA_MAX2(a,b),c))
#define _VA_MAX4(a,b,c,d)	(_VA_MAX2(_VA_MAX2(a,b),_VA_MAX2(c,d)))

 /* reusable ROSE_MAX macro */

#define ROSE_MAX(...) VA_NARGS_CALL_OVERLOAD(_VA_MAX, __VA_ARGS__)

/* -------------------------------------------------------------------- */
/** \name Minimum element.
 * \{ */

#define _VA_MIN2(a,b)		(((a)<(b))?(a):(b))
#define _VA_MIN3(a,b,c)		(_VA_MIN2(_VA_MIN2(a,b),c))
#define _VA_MIN4(a,b,c,d)	(_VA_MIN2(_VA_MIN2(a,b),_VA_MIN2(c,d)))

 /* reusable ROSE_MIN macro */

#define ROSE_MIN(...) VA_NARGS_CALL_OVERLOAD(_VA_MIN, __VA_ARGS__)

#ifdef __cplusplus
// Useful to port C code using enums to C++ where enums are strongly typed.
// To use after the enum declaration.
// If any enumerator `C` is set to say `A|B`, then `C` would be the max enum value.
#  define ENUM_OPERATORS(_enum_type, _max_enum_value) \
	extern "C++" { \
		inline constexpr _enum_type operator|(_enum_type a, _enum_type b) { \
			return static_cast<_enum_type>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b)); \
		} \
		inline constexpr _enum_type operator&(_enum_type a, _enum_type b) { \
			return static_cast<_enum_type>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b)); \
		} \
		inline constexpr _enum_type operator~(_enum_type a) { \
			return static_cast<_enum_type>(~static_cast<uint64_t>(a) & (2 * static_cast<uint64_t>(_max_enum_value) - 1)); \
		} \
		inline _enum_type &operator|=(_enum_type &a, _enum_type b) { \
			return a = static_cast<_enum_type>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b)); \
		} \
		inline _enum_type &operator&=(_enum_type &a, _enum_type b) { \
			return a = static_cast<_enum_type>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b)); \
		} \
	} /* extern "C++" */
#else
/* Output nothing. */
#  define ENUM_OPERATORS(_type, _max)
#endif

/* -------------------------------------------------------------------- */
/** \name Branch Prediction Macros
 * \{ */

 /* hints for branch prediction, only use in code that runs a _lot_ where */
#ifdef __GNUC__
#  define LIKELY(x) __builtin_expect(!!(x), 1)
#  define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#  define LIKELY(x)	(x)
#  define UNLIKELY(x)	(x)
#endif

/* -------------------------------------------------------------------- */
/** \name Flag Macros
 * \{ */

 /* Set flag from a single test */
#define SET_FLAG_FROM_TEST(value, test, flag) \
  { \
    if (test) { \
      (value) |= (flag); \
    } \
    else { \
      (value) &= ~(flag); \
    } \
  } \
  ((void)0)
