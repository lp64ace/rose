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
	importer/fbx_import_util.cc
	importer/fbx_import_util.hh
	importer/fbx_import_armature.cc
	importer/fbx_import_armature.hh

	IO_fbx.cc
	IO_fbx.h
	
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::source::roselib
	rose::source::rosekernel
	rose::source::editors::armature
	
	# External Library Dependencies
	rose::extern::ufbx
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(io_fbx "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::io::fbx ALIAS io_fbx)
