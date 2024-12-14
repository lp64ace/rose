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
	${EIGEN3_INCLUDE_DIRS}
	
)

# -----------------------------------------------------------------------------
# Define Source Files

set(SRC
	eigen_capi.h

	intern/eigenvalues.cc
	intern/linear_solver.cc
	intern/matrix.cc
	intern/svd.cc

	intern/eigenvalues.h
	intern/linear_solver.h
	intern/matrix.h
	intern/svd.h
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(eigen "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::intern::eigen ALIAS eigen)
