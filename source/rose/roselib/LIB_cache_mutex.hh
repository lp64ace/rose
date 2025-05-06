#ifndef LIB_CACHE_MUTEX_HH
#define LIB_CACHE_MUTEX_HH

#include "LIB_function_ref.hh"

#include <atomic>
#include <mutex>

namespace rose {

class CacheMutex {
private:
	std::mutex mutex_;
	std::atomic<bool> cache_valid_ = false;

public:
	/**
	 * Make sure the cache exists and is up to date. This calls `compute_cache` once to update the
	 * cache (which is stored outside of this class) if it is dirty, otherwise it does nothing.
	 *
	 * This function is thread-safe under the assumption that the same parameters are passed from
	 * every thread.
	 */
	void ensure(FunctionRef<void()> compute_cache);

	/**
	 * Reset the cache. The next time #ensure is called, it will recompute that code.
	 */
	void tag_dirty() {
		cache_valid_.store(false);
	}

	/**
	 * Return true if the cache currently does not exist or has been invalidated.
	 */
	bool is_dirty() const {
		return !this->is_cached();
	}

	/**
	 * Return true if the cache exists and is valid.
	 */
	bool is_cached() const {
		return cache_valid_.load(std::memory_order_relaxed);
	}
};

}  // namespace rose

#endif	// LIB_CACHE_MUTEX_HH
