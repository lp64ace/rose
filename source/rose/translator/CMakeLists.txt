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
	RT_context.h
	RT_object.h
	RT_preprocessor.h
	RT_parser.h
	RT_source.h
	RT_token.h
	
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
	intern/ast/node/binary.h
	intern/ast/node/block.c
	intern/ast/node/block.h
	intern/ast/node/cast.c
	intern/ast/node/cast.h
	intern/ast/node/conditional.c
	intern/ast/node/conditional.h
	intern/ast/node/constant.c
	intern/ast/node/constant.h
	intern/ast/node/consteval.c
	intern/ast/node/funcall.c
	intern/ast/node/funcall.h
	intern/ast/node/member.c
	intern/ast/node/member.h
	intern/ast/node/object.c
	intern/ast/node/object.h
	intern/ast/node/unary.c
	intern/ast/node/unary.h
	
	intern/ast/node.h
	intern/ast/type.h
	
	intern/context.c
	intern/object.c
	intern/parser.c
	intern/preprocessor.c
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

rose_add_lib(translator "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::translator ALIAS translator)

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
	test/utils.cc
)

# -----------------------------------------------------------------------------
# Define Library Dependencies (Test)

set(LIB
	# Internal Library Dependencies
	rose::source::translator
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Test

rose_add_test_executable(translator "${TEST}" "${INC}" "${INC_SYS}" "${LIB}")
