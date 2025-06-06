#include "LIB_cache_mutex.hh"
#include "LIB_task.hh"

namespace rose {

void CacheMutex::ensure(const FunctionRef<void()> compute_cache) {
	if (cache_valid_.load(std::memory_order_acquire)) {
		return;
	}
	std::scoped_lock lock{mutex_};
	/** Double checked lock. */
	if (cache_valid_.load(std::memory_order_acquire)) {
		return;
	}
	/** Use task isolation because a mutex is locked and the cache computation might use multi-threading. */
	threading::isolate_task(compute_cache);

	cache_valid_.store(true, std::memory_order_release);
}

}  // namespace rose
