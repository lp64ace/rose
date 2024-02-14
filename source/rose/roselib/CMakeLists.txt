# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PUBLIC .
	
	# Internal Include Directories
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
	LIB_assert.h
	LIB_compiler_attrs.h
	LIB_endian.h
	LIB_hash.hh
	LIB_index_range.hh
	LIB_listbase.h
	LIB_listbase_inline.h
	LIB_math.h
	LIB_math_base.h
	LIB_math_base_safe.h
	LIB_math_bits.h
	LIB_math_geom.h
	LIB_math_inline.h
	LIB_math_matrix.h
	LIB_math_rotation.h
	LIB_math_solvers.h
	LIB_math_vector.h
	LIB_memory_utils.h
	LIB_memory_utils.hh
	LIB_span.hh
	LIB_spinlock.h
	LIB_string.h
	LIB_struct_equality_utils.hh
	LIB_system.h
	LIB_sys_types.h
	LIB_utildefines.h
	LIB_utildefines_variadic.h
	LIB_vector.hh
	intern/assert.c
	intern/listbase.c
	intern/math_base.c
	intern/math_matrix.c
	intern/math_rotation.c
	intern/math_solvers.c
	intern/math_vector.c
	intern/memory_utils.c
	intern/spinlock.c
	intern/string.c
	intern/system.c
	intern/system_unix.c
	intern/system_win32.c
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
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
# Declare Executable

rose_add_lib(rf_source_roselib "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rf::source::roselib ALIAS rf_source_roselib)
