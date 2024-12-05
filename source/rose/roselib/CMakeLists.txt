# -----------------------------------------------------------------------------
# Define Include Directories

set(INC
	# Internal Include Directories
	PUBLIC .
	PUBLIC ${CMAKE_SOURCE_DIR}/source/rose/makedna
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Define System Include Directories

set(INC_SYS
	# External System Include Directories
	${PTHREADS_INCLUDE_DIRS}
	
)

# -----------------------------------------------------------------------------
# Define Source Files

set(SRC
	LIB_allocator.hh
	LIB_array.hh
	LIB_assert.h
	LIB_bitmap.h
	LIB_endian_switch.h
	LIB_filereader.h
	LIB_ghash.h
	LIB_hash.h
	LIB_hash.hh
	LIB_hash_mm2a.h
	LIB_hash_tables.hh
	LIB_index_range.hh
	LIB_listbase.h
	LIB_map.hh
	LIB_map_slots.hh
	LIB_math_base.h
	LIB_math_bit.h
	LIB_math_matrix.h
	LIB_math_matrix_types.hh
	LIB_math_rotation.h
	LIB_math_vector.h
	LIB_math_vector_types.hh
	LIB_memarena.h
	LIB_memory_utils.hh
	LIB_mempool.h
	LIB_path_utils.h
	LIB_probing_strategies.hh
	LIB_random_access_iterator_mixin.hh
	LIB_rect.h
	LIB_set.hh
	LIB_set_slots.hh
	LIB_span.hh
	LIB_string.h
	LIB_string_ref.hh
	LIB_string_utils.h
	LIB_sys_types.h
	LIB_thread.h
	LIB_unroll.hh
	LIB_utildefines.h
	LIB_utildefines_variadic.h
	LIB_utility_mixins.hh
	LIB_vector.hh
	
	algorithm/rabin_karp.c
	algorithm/rabin_karp.h
	
	intern/assert.c
	intern/bitmap.c
	intern/endian_switch.c
	intern/fileops.c
	intern/filereader.c
	intern/ghash.c
	intern/ghash_utils.c
	intern/hash_mm2a.c
	intern/listbase.c
	intern/math_base.c
	intern/math_bit.c
	intern/math_matrix.c
	intern/math_matrix.cc
	intern/math_rotation.c
	intern/math_vector.c
	intern/memarena.c
	intern/mempool.c
	intern/path_utils.c
	intern/rect.c
	intern/string.c
	intern/string_ref.cc
	intern/string_utils.c
	intern/thread.c
	intern/utildefines.c
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::intern::wcwidth
	
	# External Library Dependencies
	PUBLIC ${PTHREADS_LIBRARIES}
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(roselib "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::roselib ALIAS roselib)

# -----------------------------------------------------------------------------
# Define Include Directories (Test)

set(INC
	# Internal Include Directories
	PUBLIC .
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Define System Include Directories (Test)

set(INC_SYS
	# External System Include Directories
	
)

# -----------------------------------------------------------------------------
# Define Source Files (Test)

set(TEST
	test/bitmap.cc
	test/endian.cc
	test/ghash.cc
	test/listbase.cc
	test/math_bit.cc
	test/math_matrix_types.cc
	test/math_vector_types.cc
	test/mempool.cc
	test/rabin_karp.cc
	test/span.cc
	test/string.cc
	test/vector.cc
)

# -----------------------------------------------------------------------------
# Define Library Dependencies (Test)

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::intern::wcwidth
	rose::source::roselib
	
	# External Library Dependencies
	${PTHREADS_LIBRARIES}
	
)

# -----------------------------------------------------------------------------
# Declare Test

rose_add_test_executable(roselib "${TEST}" "${INC}" "${INC_SYS}" "${LIB}")
