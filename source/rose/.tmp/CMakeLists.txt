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
	lexer.h
	object.h
	parser.h
	source.h
	test
	token.h
	type.h

	intern/lexer.c
	intern/object.c
	intern/parser.c
	intern/source.c
	intern/token.c
	intern/type.c
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	PUBLIC rose::source::roselib
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(rosecompiler "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::rosecompiler ALIAS rosecompiler)

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
	test/simple.cc
)

# -----------------------------------------------------------------------------
# Define Library Dependencies (Test)

set(LIB
	# Internal Library Dependencies
	rose::source::rosecompiler
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Test

rose_add_test_executable(rosecompiler "${TEST}" "${INC}" "${INC_SYS}" "${LIB}")
