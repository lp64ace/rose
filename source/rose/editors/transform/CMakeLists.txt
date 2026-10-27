# -----------------------------------------------------------------------------
# Define Include Directories

set(INC
	# Internal Include Directories
	PUBLIC ../include
	
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
	transform.c
	transform.h
	transform_convert.c
	transform_convert.h
	transform_convert_object.c
	transform_data.c
	transform_data.h
	transform_ops.c
	
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::source::roselib
	rose::source::rosekernel
	rose::source::dna
	rose::source::rna
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(transform "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::editors::transform ALIAS transform)
