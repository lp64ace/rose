#include "LIB_task.hh"

#ifdef WITH_TBB
#	include <tbb/blocked_range.h>
#	include <tbb/enumerable_thread_specific.h>
#	include <tbb/parallel_for.h>
#	include <tbb/parallel_reduce.h>
#endif

namespace rose::threading::detail {

#ifdef WITH_TBB
static void parallel_for_impl_static_size(const IndexRange range, const size_t grain_size, const FunctionRef<void(IndexRange)> function) {
	tbb::parallel_for(tbb::blocked_range<size_t>(range.first(), range.one_after_last(), grain_size), [function](const tbb::blocked_range<size_t> &subrange) { function(IndexRange(subrange.begin(), subrange.size())); });
}
#endif /* WITH_TBB */

void parallel_for_impl(IndexRange range, size_t grain_size, FunctionRef<void(IndexRange)> function) {
#ifdef WITH_TBB
	lazy_threading::send_hint();
	parallel_for_impl_static_size(range, grain_size, function);
#else
	UNUSED_VARS(grain_size);
	function(range);
#endif
}

}  // namespace rose::threading::detail
