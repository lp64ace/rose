#ifndef LIB_UTILITY_MIXINS_HH
#define LIB_UTILITY_MIXINS_HH

namespace rose {

/**
 * A type that inherits from NonCopyable cannot be copied anymore.
 */
class NonCopyable {
public:
	/* Disable copy construction and assignment. */
	NonCopyable(const NonCopyable &other) = delete;
	NonCopyable &operator=(const NonCopyable &other) = delete;

	/* Explicitly enable default construction, move construction and move assignment. */
	NonCopyable() = default;
	NonCopyable(NonCopyable &&other) = default;
	NonCopyable &operator=(NonCopyable &&other) = default;
};

/**
 * A type that inherits from NonMovable cannot be moved anymore.
 */
class NonMovable {
public:
	/* Disable move construction and assignment. */
	NonMovable(NonMovable &&other) = delete;
	NonMovable &operator=(NonMovable &&other) = delete;

	/* Explicitly enable default construction, copy construction and copy assignment. */
	NonMovable() = default;
	NonMovable(const NonMovable &other) = default;
	NonMovable &operator=(const NonMovable &other) = default;
};

}  // namespace rose

#endif	// LIB_UTILITY_MIXINS_HH
