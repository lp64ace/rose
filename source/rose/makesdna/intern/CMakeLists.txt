# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PUBLIC ..
	
	# Internal Include Directories
	${CMAKE_SOURCE_DIR}/intern/memalloc
	
)

# -----------------------------------------------------------------------------
# Declare System Include Directories

set(INC_SYS
	# External Include Directories

)

# -----------------------------------------------------------------------------
# Declare Source Files

set(SRC
	${DNA_TYPES_HEADERS}
	${DNA_DEFAULTS_HEADERS}
	
	dna_utils.c
	dna_utils.h
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	rf::intern::memalloc
	
	# External Include Directories

)

# -----------------------------------------------------------------------------
# Misc

set(dna_header_include_file "${CMAKE_CURRENT_BINARY_DIR}/dna_includes_all.h")
set(dna_header_string_file "${CMAKE_CURRENT_BINARY_DIR}/dna_includes_as_strings.h")

set(DNA_INCLUDE_TEXT "/* Do not edit manually, changes will be overwritten. */\n")
set(DNA_FILE_LIST "/* Do not edit manually, changes will be overwritten. */\n")
foreach(header ${DNA_TYPES_HEADERS})
  get_filename_component(dna_header_file ${header} NAME)
  string(APPEND DNA_INCLUDE_TEXT "#include \"${header}\"\n")
  string(APPEND DNA_FILE_LIST "\t\"${dna_header_file}\",\n")
endforeach()

file(GENERATE OUTPUT ${dna_header_include_file} CONTENT "${DNA_INCLUDE_TEXT}")
file(GENERATE OUTPUT ${dna_header_string_file} CONTENT "${DNA_FILE_LIST}")

# -----------------------------------------------------------------------------
# Declare DNA Interface Library

rose_add_lib(rf_source_dna "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rf::source::dna ALIAS rf_source_dna)


# ----------------------------------------------------------------------------- #
# ---                               EXECUTABLE                              --- #
# ----------------------------------------------------------------------------- #

find_package(LLVM REQUIRED CONFIG
	PATHS
		${CMAKE_BINARY_DIR}/../llvm-bin
)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Set additional flags for LLVM
llvm_map_components_to_libnames(LLVM_LIBRARIES all)

find_package(Clang REQUIRED
	PATHS
		${CMAKE_BINARY_DIR}/../llvm-bin
)

# Add path to LLVM modules
set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	${LLVM_CMAKE_DIR}
)

# import LLVM CMake functions
include(AddLLVM)

include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${CLANG_DEFINITIONS})

# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	${CMAKE_SOURCE_DIR}/intern/memalloc
	${CMAKE_SOURCE_DIR}/source/rose/makesdna
	${CMAKE_SOURCE_DIR}/source/rose/roselib
	
	# LLVM Include Directories
	${LLVM_INCLUDE_DIRS}
	
)

# -----------------------------------------------------------------------------
# Declare Source Files

set(SRC
	${DNA_TYPES_HEADERS}
	${DNA_DEFAULTS_HEADERS}
	
	dna_utils.c
	dna_utils.h
	
	llvmdna.cc
	
	${dna_header_include_file}
	${dna_header_string_file}
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	rf::intern::memalloc
	rf::source::roselib
	
	# External Include Directories
	${LLVM_LIBRARIES}

)

# -----------------------------------------------------------------------------
# Add DNA Executable

set(LLVM_LINK_COMPONENTS support)

add_llvm_executable(makesdna
	${SRC}
)
target_link_libraries(makesdna
	PUBLIC
		${LIB}
	PRIVATE
		clangAST
		clangASTMatchers
		clangBasic
		clangFrontend
		clangSerialization
		clangTooling
)

target_include_directories(makesdna PRIVATE "${INC}")

# -----------------------------------------------------------------------------
# Anoying LLVM pojects in ROOT DIRECTORY

set(EXTERNAL_PROJECTS_FILTER "extern")

set_target_properties(acc_gen PROPERTIES FOLDER ${EXTERNAL_PROJECTS_FILTER}/llvm)
set_target_properties(clang-tablegen-targets PROPERTIES FOLDER ${EXTERNAL_PROJECTS_FILTER}/llvm)
set_target_properties(intrinsics_gen PROPERTIES FOLDER ${EXTERNAL_PROJECTS_FILTER}/llvm)
set_target_properties(omp_gen PROPERTIES FOLDER ${EXTERNAL_PROJECTS_FILTER}/llvm)
set_target_properties(RISCVTargetParserTableGen PROPERTIES FOLDER ${EXTERNAL_PROJECTS_FILTER}/llvm)
