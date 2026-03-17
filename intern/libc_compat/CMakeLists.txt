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
	libc_compat.c
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies

	# External Library Dependencies

)

# -----------------------------------------------------------------------------
# Define Library

rose_add_lib(libc_compat "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::intern::libc_compat ALIAS libc_compat)

