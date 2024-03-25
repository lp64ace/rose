# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PUBLIC ..
	
	# Internal Include Directories
	
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
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	
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

add_library(rf_source_dna INTERFACE)

target_include_directories(rf_source_dna INTERFACE .)
add_library(rf::source::dna ALIAS rf_source_dna)

# CMake 3.19+ allows one to populate the interface library with
# source files to show in the IDE, for people on older CMake versions
# these headers will be visible in the bf_intern_guardedalloc project
# where they historically have been.
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.19")
	target_sources(rf_source_dna PRIVATE ${SRC})
	rose_source_group(rf_source_dna ${SRC})
endif()

# -----------------------------------------------------------------------------
# Converting Headers To LLVM

find_program(CLANG_EXECUTABLE_PATH clang)

if(CLANG_EXECUTABLE_PATH)
	set(CLANG_EXECUTABLE_PATH "${CLANG_EXECUTABLE_PATH}/clang.exe")
else()
	message(STATUS "You may want to try to install clang manually or you can install the clang extensions through Visual Studio!")
    message(ERROR "Clang not found.")
endif()

get_filename_component(CLANG_DIRECTORY ${CLANG_EXECUTABLE_PATH} DIRECTORY)
get_filename_component(CLANG_EXECUTABLE ${CLANG_EXECUTABLE_PATH} NAME)

set(CLANG_OUTPUT
)

foreach(HEADER ${DNA_TYPES_HEADERS})
	get_filename_component(HEADER_DIRECTORY ${HEADER} DIRECTORY)
	get_filename_component(HEADER_FILENAME ${HEADER} NAME)
	
	set(HEADER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${HEADER_FILENAME}.llvm")
	
	add_custom_command(
		OUTPUT ${HEADER_OUTPUT}
		# Silence warnings because our headers are treated like source files and they have "#pragma once" specified.
		COMMAND "${CLANG_EXECUTABLE}" -w -S -x c "${HEADER}" -DDNA_CLANG -emit-llvm -femit-all-decls -o "${HEADER_OUTPUT}"
		# Mark the llvm output to depend on the DNA header files.
		DEPENDS ${DNA_TYPES_HEADERS}
	)
	
	list(APPEND CLANG_OUTPUT ${HEADER_OUTPUT})
endforeach()

# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
	
	PRIVATE ${CMAKE_SOURCE_DIR}/intern/memalloc
	PRIVATE ${CMAKE_SOURCE_DIR}/source/rose/makesdna
	PRIVATE ${CMAKE_SOURCE_DIR}/source/rose/roselib
	
	# Internal Include Directories
	
)

# -----------------------------------------------------------------------------
# Declare Source Files

set(SRC
	${DNA_TYPES_HEADERS}
	${DNA_DEFAULTS_HEADERS}
	makesdna.c
	
	${dna_header_include_file}
	${dna_header_string_file}
	${CLANG_OUTPUT}
)

set_source_files_properties(
	${CLANG_OUTPUT}
	PROPERTIES GENERATED TRUE
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	PRIVATE rf::intern::memalloc
	PRIVATE rf::source::roselib
	
	# External Include Directories

)

# -----------------------------------------------------------------------------
# Add DNA Executable

add_executable(makesdna ${SRC})
target_include_directories(makesdna ${INC})
target_link_libraries(makesdna ${LIB})
