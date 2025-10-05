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
	${EIGEN3_INCLUDE_DIRS}
	
)

# -----------------------------------------------------------------------------
# Define Source Files

set(SRC
	LIB_alloca.h
	LIB_allocator.hh
	LIB_any.hh
	LIB_array.h
	LIB_array.hh
	LIB_array_utils.hh
	LIB_assert.h
	LIB_binary_search.hh
	LIB_bitmap.h
	LIB_bit_bool_conversion.hh
	LIB_bit_group_vector.hh
	LIB_bit_ref.hh
	LIB_bit_span.hh
	LIB_bit_span_ops.hh
	LIB_bit_span_to_index_ranges.hh
	LIB_bit_vector.hh
	LIB_bounds_types.hh
	LIB_cache_mutex.hh
	LIB_color.hh
	LIB_compiler_checktype.h
	LIB_compute_context.hh
	LIB_cpp_type.hh
	LIB_cpp_types.hh
	LIB_cpp_types_make.hh
	LIB_cpp_type_make.hh
	LIB_devirtualize_parameters.hh
	LIB_dot_export.hh
	LIB_dot_export_attribute_enums.hh
	LIB_endian_switch.h
	LIB_enumerable_thread_specific.hh
	LIB_fileops.h
	LIB_filereader.h
	LIB_function_ref.hh
	LIB_generic_array.hh
	LIB_generic_pointer.hh
	LIB_generic_span.hh
	LIB_generic_vector_array.hh
	LIB_generic_virtual_array.hh
	LIB_generic_virtual_vector_array.hh
	LIB_ghash.h
	LIB_gsqueue.h
	LIB_hash.h
	LIB_hash.hh
	LIB_hash_mm2a.h
	LIB_hash_tables.hh
	LIB_implicit_sharing.hh
	LIB_implicit_sharing_ptr.hh
	LIB_index_mask.hh
	LIB_index_mask_expression.hh
	LIB_index_mask_fwd.hh
	LIB_index_range.hh
	LIB_index_ranges_builder.hh
	LIB_index_ranges_builder_fwd.hh
	LIB_lazy_threading.hh
	LIB_linear_allocator.hh
	LIB_linklist.h
	LIB_listbase.h
	LIB_map.hh
	LIB_map_slots.hh
	LIB_math_angle_types.hh
	LIB_math_axis_angle_types.hh
	LIB_math_base.h
	LIB_math_base.hh
	LIB_math_basis_types.hh
	LIB_math_bit.h
	LIB_math_color.h
	LIB_math_color.hh
	LIB_math_color_blend.h
	LIB_math_euler.hh
	LIB_math_euler_types.hh
	LIB_math_geom.h
	LIB_math_matrix.h
	LIB_math_matrix.hh
	LIB_math_matrix_types.hh
	LIB_math_numbers.hh
	LIB_math_quaternion.hh
	LIB_math_quaternion_types.hh
	LIB_math_rotation.h
	LIB_math_rotation_types.hh
	LIB_math_vector.h
	LIB_math_vector.hh
	LIB_math_vector_types.hh
	LIB_memarena.h
	LIB_memory_utils.hh
	LIB_memblock.h
	LIB_mempool.h
	LIB_multi_value_map.hh
	LIB_offset_indices.hh
	LIB_offset_span.hh
	LIB_parameter_pack_utils.hh
	LIB_path_utils.h
	LIB_polyfill_2d.h
	LIB_probing_strategies.hh
	LIB_random_access_iterator_mixin.hh
	LIB_rect.h
	LIB_resource_scope.hh
	LIB_set.hh
	LIB_set_slots.hh
	LIB_shared_cache.hh
	LIB_sort.hh
	LIB_sort_utils.h
	LIB_span.hh
	LIB_stack.hh
	LIB_string.h
	LIB_string_ref.hh
	LIB_string_utf.h
	LIB_string_utils.h
	LIB_struct_equality_utils.hh
	LIB_sys_types.h
	LIB_task.h
	LIB_task.hh
	LIB_thread.h
	LIB_unique_sorted_indices.hh
	LIB_unroll.hh
	LIB_utildefines.h
	LIB_utildefines_variadic.h
	LIB_utility_mixins.hh
	LIB_vector.hh
	LIB_vector_set.hh
	LIB_vector_set_slots.hh
	LIB_virtual_array.hh
	LIB_virtual_array_fwd.hh
	LIB_virtual_vector_array.hh
	
	algorithm/rabin_karp.c
	algorithm/rabin_karp.h
	
	intern/array.c
	intern/array_utils.cc
	intern/assert.c
	intern/bitmap.c
	intern/bit_bool_conversion.cc
	intern/bit_ref.cc
	intern/bit_span.cc
	intern/cache_mutex.cc
	intern/color.cc
	intern/compute_context.cc
	intern/cpp_type.cc
	intern/cpp_types.cc
	intern/dot_export.cc
	intern/endian_switch.c
	intern/fileops.c
	intern/filereader.c
	intern/generic_vector_array.cc
	intern/generic_virtual_array.cc
	intern/generic_virtual_vector_array.cc
	intern/ghash.c
	intern/ghash_utils.c
	intern/gsqueue.c
	intern/hash_mm2a.c
	intern/implicit_sharing.cc
	intern/index_mask.cc
	intern/index_mask_expression.cc
	intern/index_range.cc
	intern/lazy_threading.cc
	intern/linklist.c
	intern/listbase.c
	intern/math_base.c
	intern/math_basis_types.cc
	intern/math_bit.c
	intern/math_color.c
	intern/math_geom.c
	intern/math_matrix.c
	intern/math_matrix.cc
	intern/math_rotation.c
	intern/math_vector.c
	intern/memarena.c
	intern/memblock.c
	intern/mempool.c
	intern/path_utils.c
	intern/polyfill_2d.c
	intern/rect.c
	intern/sort_utils.c
	intern/string.c
	intern/string_ref.cc
	intern/string_utf.c
	intern/string_utils.c
	intern/task.cc
	intern/task_graph.cc
	intern/task_iterator.c
	intern/task_pool.cc
	intern/task_range.cc
	intern/task_scheduler.cc
	intern/thread.c
	intern/utildefines.c
	intern/virtual_array.cc
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::intern::atomic
	rose::intern::wcwidth
	rose::intern::eigen
	
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
