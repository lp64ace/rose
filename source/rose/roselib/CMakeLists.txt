# -----------------------------------------------------------------------------
# Define Include Directories

set(INC
	# Internal Include Directories
	PUBLIC .
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Define System Include Directories

set(INC_SYS
	# External System Include Directories
	
)

# -----------------------------------------------------------------------------
# Define Source Files

set(SRC
	LIB_assert.h
	LIB_ghash.h
	LIB_hash_mm2a.h
	LIB_listbase.h
	LIB_memarena.h
	LIB_mempool.h
	LIB_string.h
	LIB_sys_types.h
	LIB_thread.h
	LIB_utildefines.h
	LIB_utildefines_variadic.h
	
	algorithm/rabin_karp.c
	algorithm/rabin_karp.h
	
	intern/assert.c
	intern/ghash.c
	intern/ghash_utils.c
	intern/hash_mm2a.c
	intern/listbase.c
	intern/listbase_inline.c
	intern/memarena.c
	intern/mempool.c
	intern/string.c
	intern/thread.c
	intern/utildefines.c
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	
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
	test/ghash.cc
	test/listbase.cc
	test/mempool.cc
	test/rabin_karp.cc
	test/string.cc
)

# -----------------------------------------------------------------------------
# Define Library Dependencies (Test)

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::source::roselib
	
	# External Library Dependencies
	${PTHREADS_LIBRARIES}
	
)

# -----------------------------------------------------------------------------
# Declare Test

rose_add_test_executable(roselib "${TEST}" "${INC}" "${INC_SYS}" "${LIB}")
