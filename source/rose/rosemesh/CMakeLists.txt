# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PUBLIC .
	
	# Internal Include Directories
	${CMAKE_SOURCE_DIR}/intern/memalloc
	${CMAKE_SOURCE_DIR}/source/rose/makesdna
	${CMAKE_SOURCE_DIR}/source/rose/rosekernel
	${CMAKE_SOURCE_DIR}/source/rose/roselib
	
)

# -----------------------------------------------------------------------------
# Declare System Include Directories

set(INC_SYS
	# External Include Directories
	${PTHREADS_INC}
	
)

# -----------------------------------------------------------------------------
# Declare Source Files

set(SRC
	RM_class.h
	RM_include.h
	intern/rmesh_construct.c
	intern/rmesh_core.c
	intern/rmesh_query.c
	intern/rmesh_structure.c
	intern/rmesh_iterators.cc
	intern/rmesh_mesh.cc
	intern/rmesh_presets.cc
	intern/rmesh_construct.h
	intern/rmesh_core.h
	intern/rmesh_inline.h
	intern/rmesh_iterators.h
	intern/rmesh_iterators_inline.h
	intern/rmesh_mesh.h
	intern/rmesh_presets.h
	intern/rmesh_private.h
	intern/rmesh_query.h
	intern/rmesh_query_inline.h
	intern/rmesh_structure.h
	intern/rmesh_structure_inline.h
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(rf_source_rosemesh "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rf::source::rosemesh ALIAS rf_source_rosemesh)
