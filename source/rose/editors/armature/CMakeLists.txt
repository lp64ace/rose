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
	armature_add.c
	armature_utils.c
	armature_intern.hh
	
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::source::roselib
	rose::source::rosekernel
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(armature "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::editors::armature ALIAS armature)
