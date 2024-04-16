# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PUBLIC .
	
	# Internal Include Directories
	${CMAKE_SOURCE_DIR}/extern/wcwidth
	${CMAKE_SOURCE_DIR}/intern/ghost
	${CMAKE_SOURCE_DIR}/source/rose/makesdna
	
)

# -----------------------------------------------------------------------------
# Declare System Include Directories

set(INC_SYS
	# External Include Directories
	${PTHREADS_INC}
	
)

# -----------------------------------------------------------------------------
# Declare Source Files

set(SRC
	LIB_allocator.hh
	LIB_array.hh
	LIB_assert.h
	LIB_binary_search.hh
	LIB_bit_ref.hh
	LIB_bit_span.hh
	LIB_bitmap.h
	LIB_color.hh
	LIB_compiler_attrs.h
	LIB_cpp_type.hh
	LIB_cpp_type_make.hh
	LIB_cpp_types.hh
	LIB_cpp_types_make.hh
	LIB_dynstr.h
	LIB_endian.h
	LIB_endian_switch.h
	LIB_fileops.h
	LIB_function_ref.hh
	LIB_ghash.h
	LIB_hash.h
	LIB_hash.hh
	LIB_hash_mm2a.h
	LIB_hash_tables.hh
	LIB_implicit_sharing.hh
	LIB_implicit_sharing_ptr.hh
	LIB_index_mask.hh
	LIB_index_range.hh
	LIB_linear_allocator.hh
	LIB_linklist.h
	LIB_listbase.h
	LIB_listbase_inline.h
	LIB_map.hh
	LIB_map_slots.hh
	LIB_math.h
	LIB_math_base.h
	LIB_math_base_safe.h
	LIB_math_bits.h
	LIB_math_color.h
	LIB_math_color_blend.h
	LIB_math_geom.h
	LIB_math_inline.h
	LIB_math_matrix.h
	LIB_math_mat_types.hh
	LIB_math_rotation.h
	LIB_math_solvers.h
	LIB_math_vector.h
	LIB_math_vector_types.hh
	LIB_memory_utils.h
	LIB_memory_utils.hh
	LIB_mempool.h
	LIB_offset_span.hh
	LIB_parameter_pack_utils.hh
	LIB_probing_strategies.hh
	LIB_rect.h
	LIB_set.hh
	LIB_set_slots.hh
	LIB_simd.h
	LIB_span.hh
	LIB_spinlock.h
	LIB_stack.hh
	LIB_string.h
	LIB_string_cursor_utf8.h
	LIB_string_ref.hh
	LIB_string_utf8.h
	LIB_string_utils.h
	LIB_struct_equality_utils.hh
	LIB_system.h
	LIB_sys_types.h
	LIB_threads.h
	LIB_task.hh
	LIB_time.h
	LIB_unique_sorted_indices.hh
	LIB_utildefines.h
	LIB_utildefines_variadic.h
	LIB_utility_mixins.hh
	LIB_vector.hh
	intern/assert.c
	intern/bitmap.c
	intern/cpp_type.cc
	intern/cpp_types.cc
	intern/color.cc
	intern/dynstr.c
	intern/endian_switch.c
	intern/fileops.c
	intern/ghash.c
	intern/ghash_utils.c
	intern/hash_mm2a.c
	intern/implicit_sharing.cc
	intern/lazy_threading.cc
	intern/linklist.c
	intern/listbase.c
	intern/math_base.c
	intern/math_color.c
	intern/math_geom.c
	intern/math_matrix.c
	intern/math_matrix.cc
	intern/math_rotation.c
	intern/math_solvers.c
	intern/math_vector.c
	intern/memory_utils.c
	intern/mempool.c
	intern/rct.c
	intern/spinlock.c
	intern/string.c
	intern/string_cursor_utf8.c
	intern/string_ref.cc
	intern/string_utf8.c
	intern/string_utils.c
	intern/system.c
	intern/system_unix.c
	intern/system_win32.c
	intern/threads.c
	intern/time.c
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	rf::extern::wcwidth
	rf::intern::memalloc
	
	# External Include Directories
	${PTHREADS_LIB}
)

# -----------------------------------------------------------------------------
# Operating System Dependencies

if(WIN32)
	list(APPEND LIB Dbghelp.lib Shlwapi.lib) # Needed for system_win32.c
endif()

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(rf_source_roselib "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rf::source::roselib ALIAS rf_source_roselib)

# -----------------------------------------------------------------------------
# Module Testing

set(TEST_SRC
	tests/lib_ghash_test.cc
	tests/lib_math_mat_types_test.cc
	tests/lib_math_vec_types_test.cc
	tests/lib_mempool_test.cc
	
)

set(TEST_INC
	
)

set(TEST_LIB
	rf::source::roselib
	
)

rose_add_test_executable(roselib "${TEST_SRC}" "${INC};${TEST_INC}" "${INC_SYS}" "${LIB};${TEST_LIB}")
