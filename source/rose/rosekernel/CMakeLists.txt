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
	${PTHREADS_INCLUDE_DIRS}
	
)

# -----------------------------------------------------------------------------
# Define Source Files

set(SRC
	KER_context.h
	KER_global.h
	KER_idprop.h
	KER_idtype.h
	KER_lib_id.h
	KER_lib_query.h
	KER_lib_remap.h
	KER_library.h
	KER_main.h
	KER_rose.h
	KER_screen.h

	intern/context.c
	intern/idprop.c
	intern/idtype.c
	intern/lib_id.c
	intern/lib_query.c
	intern/lib_remap.c
	intern/library.c
	intern/main.c
	intern/rose.c
	intern/screen.c
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	PUBLIC rose::source::roselib
	PUBLIC rose::source::dna
	PUBLIC rose::source::editors::interface
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(rosekernel "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::rosekernel ALIAS rosekernel)

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
	test/lib_id_free.cc
	test/lib_remap.cc
)

# -----------------------------------------------------------------------------
# Define Library Dependencies (Test)

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::source::roselib
	rose::source::dna
	rose::source::rosekernel
	rose::source::windowmanager
	
	# External Library Dependencies
	${PTHREADS_LIBRARIES}
	
)

# -----------------------------------------------------------------------------
# Declare Test

rose_add_test_executable(rosekernel "${TEST}" "${INC}" "${INC_SYS}" "${LIB}")

