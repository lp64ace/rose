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
	COM_ast.h
	COM_context.h
	COM_decl.h
	COM_parser.h
	COM_preprocessor.h
	COM_scope.h
	COM_source.h
	COM_source_location.h
	COM_token.h
	COM_type.h
	
	intern/context.c
	intern/token.c
	intern/type.c
	intern/type.cc
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::source::roselib
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(rosec "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::rosec ALIAS rosec)

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
	test/token.cc
)

# -----------------------------------------------------------------------------
# Define Library Dependencies (Test)

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::source::roselib
	rose::source::rosec
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Test

rose_add_test_executable(rosec "${TEST}" "${INC}" "${INC_SYS}" "${LIB}")
