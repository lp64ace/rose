#ifndef LIB_SORT_HH
#define LIB_SORT_HH

#ifdef WITH_TBB
#	include <tbb/parallel_sort.h>
#else
#	include <algorithm>
#endif

namespace rose {

#ifdef WITH_TBB
using tbb::parallel_sort;
#else
template<typename RandomAccessIterator> void parallel_sort(RandomAccessIterator begin, RandomAccessIterator end) {
	std::sort<RandomAccessIterator>(begin, end);
}
template<typename RandomAccessIterator, typename Compare> void parallel_sort(RandomAccessIterator begin, RandomAccessIterator end, const Compare &comp) {
	std::sort<RandomAccessIterator, Compare>(begin, end, comp);
}
#endif

}  // namespace rose

#endif	// LIB_SORT_HH
