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
	NET_address.h
	NET_socket.h

	intern/address.c
	intern/socket.c
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc

	# External Library Dependencies

)

# -----------------------------------------------------------------------------
# Define Library

rose_add_lib(network "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::intern::network ALIAS network)

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
	test/address.cc
	test/socket.cc
)

# -----------------------------------------------------------------------------
# Define Library Dependencies (Test)

set(LIB
	# Internal Library Dependencies
	rose::intern::network

	# External Library Dependencies

)

# -----------------------------------------------------------------------------
# Decalre Test

rose_add_test_executable(network "${TEST}" "${INC}" "${INC_SYS}" "${LIB}")



