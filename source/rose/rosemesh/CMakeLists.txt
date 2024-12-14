# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PUBLIC .
	
	# Internal Include Directories
	
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
	RM_type.h
	RM_include.h
	
	intern/rm_construct.c
	intern/rm_construct.h
	intern/rm_core.c
	intern/rm_core.h
	intern/rm_inline.h
	intern/rm_iterators.cc
	intern/rm_iterators.h
	intern/rm_iterators_inline.h
	intern/rm_mesh.cc
	intern/rm_mesh.h
	intern/rm_mesh_convert.cc
	intern/rm_mesh_convert.h
	intern/rm_presets.cc
	intern/rm_presets.h
	intern/rm_private.h
	intern/rm_query.c
	intern/rm_query.h
	intern/rm_query_inline.h
	intern/rm_structure.c
	intern/rm_structure.h
	intern/rm_structure_inline.h
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	
	# External Include Directories
	rose::intern::guardedalloc
	PUBLIC rose::source::roselib
	PUBLIC rose::source::rosekernel
	rose::source::dna
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(rosemesh "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::rosemesh ALIAS rosemesh)
