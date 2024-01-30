# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PUBLIC .
	
	# Internal Include Directories
	${CMAKE_SOURCE_DIR}/intern/memalloc
	${CMAKE_SOURCE_DIR}/source/rose/makesdna
	${CMAKE_SOURCE_DIR}/source/rose/roselib
	
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
	KER_context.h
	KER_global.h
	KER_lib_id.h
	KER_library.h
	KER_idtype.h
	KER_main.h
	KER_rose.h
	intern/context.c
	intern/global.c
	intern/lib_id.c
	intern/library.c
	intern/idtype.c
	intern/main.c
	intern/rose.c
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	rf::intern::memalloc
	rf::source::roselib
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Declare Executable

rose_add_lib(rf_source_rosekernel "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rf::source::rosekernel ALIAS rf_source_rosekernel)
