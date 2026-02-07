# -----------------------------------------------------------------------------
# Define Common Include Directories

set(INC
	# Internal Include Directories
	.
	PUBLIC ..
	PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/../
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Define System Include Directories

set(INC_SYS
	# External System Include Directories
	${PTHREADS_INCLUDE_DIRS}
	
)

# -----------------------------------------------------------------------------
# Define Source Files

set(LIB_SRC
	../RNA_define.h
	../RNA_types.h
	
	rna_define.c
	rna_define_dna_compat.c
	rna_internal_types.h
	rna_internal.h
	
)

set(RUN_SRC
	../RNA_access.h

	rna_access.c
	rna_access.cc
	rna_id_runtime.c
	rna_pose_runtime.c
	rna_rna_runtime.c
	rna_rna_runtime.cc
	rna_wm_runtime.c
)

# -----------------------------------------------------------------------------
# Define Generator Sources

set(IN_SRC
	rna_id.c
	rna_ob.c
	rna_pose.c
	rna_rna.c
	rna_wm.c

)

string(
	REGEX REPLACE
	"rna_([a-zA-Z0-9_-]*).c" "${CMAKE_CURRENT_BINARY_DIR}/rna_\\1_gen.c"
	OUT_SRC "${IN_SRC}"
)
list(APPEND OUT_SRC
	"${CMAKE_CURRENT_BINARY_DIR}/../RNA_prototypes.h"
)
set_source_files_properties(${OUT_SRC} PROPERTIES GENERATED TRUE)

# -----------------------------------------------------------------------------
# Define Executable Source Files

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::intern::atomic
	PUBLIC rose::source::roselib
	PUBLIC rose::source::dna
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Executable Source Files

set(EXE_SRC
	makerna.c

)

# -----------------------------------------------------------------------------
# Declare Executable

list(APPEND EXE_SRC ${LIB_SRC})
list(APPEND EXE_SRC ${IN_SRC})

add_executable(makerna "${EXE_SRC}")

rose_target_link_libraries(makerna "${LIB}")
rose_target_include_dirs(makerna "${INC}")

target_compile_definitions(makerna PRIVATE ROSE_MAKERNA)

# -----------------------------------------------------------------------------
# Generate Files

add_custom_command(
	OUTPUT
		${OUT_SRC}
	COMMAND
		"$<TARGET_FILE:makerna>"
			"--src" "${CMAKE_SOURCE_DIR}/source/rose/makerna"
			"--bin" "${CMAKE_CURRENT_BINARY_DIR}"
	DEPENDS
		makerna
		${IN_SRC}
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::intern::atomic
	PUBLIC rose::source::roselib
	PUBLIC rose::source::rosekernel
	PUBLIC rose::source::depsgraph
	PUBLIC rose::source::dna
	PUBLIC rose::source::windowmanager
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

list(APPEND LIB_SRC ${RUN_SRC})
list(APPEND LIB_SRC ${OUT_SRC})

rose_add_lib(rna "${LIB_SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::rna ALIAS rna)

target_compile_definitions(rna PUBLIC RNA_RUNTIME)
