# -----------------------------------------------------------------------------
# Define Include Directories

set(INC
	# Internal Include Directories
	PUBLIC .
	./intern
	
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
	context.h
	object.h
	parser.h
	source.h
	
	intern/ast/type/array.c
	intern/ast/type/array.h
	intern/ast/type/base.c
	intern/ast/type/base.h
	intern/ast/type/basic.c
	intern/ast/type/basic.h
	intern/ast/type/enum.c
	intern/ast/type/enum.h
	intern/ast/type/function.c
	intern/ast/type/function.h
	intern/ast/type/pointer.c
	intern/ast/type/pointer.h
	intern/ast/type/qualified.c
	intern/ast/type/qualified.h
	intern/ast/type/struct.c
	intern/ast/type/struct.h
	
	intern/ast/node/base.c
	intern/ast/node/base.h
	intern/ast/node/binary.c
	intern/ast/node/constant.c
	intern/ast/node/declaration.c
	intern/ast/node/unary.c
	
	intern/ast/node.h
	intern/ast/type.h
	
	intern/context.c
	intern/object.c
	intern/parser.c
	intern/source.c
	intern/token.c
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
	test/token.cc
	test/tokenizer.cc
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
