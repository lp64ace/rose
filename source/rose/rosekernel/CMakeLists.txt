# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PUBLIC .
	
	# Internal Include Directories
	${CMAKE_SOURCE_DIR}/intern/memalloc
	${CMAKE_SOURCE_DIR}/source/rose/makesdna
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
	KER_anonymous_attribute_id.hh
	KER_context.h
	KER_cpp_types.h
	KER_customdata.h
	KER_global.h
	KER_lib_id.h
	KER_lib_query.h
	KER_lib_remap.h
	KER_library.h
	KER_idtype.h
	KER_main.h
	KER_namemap.h
	KER_rose.h
	KER_screen.h
	KER_workspace.h
	intern/anonymous_attribute_id.cc
	intern/context.c
	intern/cpp_types.cc
	intern/customdata.cc
	intern/global.c
	intern/lib_id.c
	intern/lib_query.c
	intern/lib_remap.cc
	intern/library.c
	intern/idtype.c
	intern/main.c
	intern/namemap.cc
	intern/rose.c
	intern/screen.c
	intern/workspace.c
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	rf::intern::memalloc
	rf::source::roselib
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(rf_source_rosekernel "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rf::source::rosekernel ALIAS rf_source_rosekernel)
