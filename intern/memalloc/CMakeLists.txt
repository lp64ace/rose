# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PUBLIC .
	
	# Internal Include Directories
	
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
	MEM_alloc.h
	intern/leak_detector.cc
	intern/linklist.c
	intern/linklist.h
	intern/mallocn.c
	intern/mallocn_guarded_impl.c
	intern/mallocn_inline.h
	intern/mallocn_intern.h
	intern/mallocn_lockfree_impl.c
	intern/memory_usage.cc
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	
	# External Include Directories
	${PTHREADS_LIB}
)

# -----------------------------------------------------------------------------
# Declare Executable

rose_add_lib(rf_intern_memalloc "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rf::intern::memalloc ALIAS rf_intern_memalloc)
