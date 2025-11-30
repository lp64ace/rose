# -----------------------------------------------------------------------------
# Misc

set(DNA_HEADER_INC_FILE "${CMAKE_CURRENT_BINARY_DIR}/DNA_include_all.h")
set(DNA_HEADER_STR_FILE "${CMAKE_CURRENT_BINARY_DIR}/DNA_strings_all.h")

set(DNA_INCLUDE_TEXT "/* Do not edit manually, changes will be overwritten. */\n")
set(DNA_STRINGS_TEXT "/* Do not edit manually, changes will be overwritten. */\n")

foreach(HEADER ${DNA_TYPEDEF_HEADERS})
	get_filename_component(DNA_HEADER_FILE ${HEADER} NAME)
	string(APPEND DNA_INCLUDE_TEXT "#include \"${HEADER}\"\n")
	string(APPEND DNA_STRINGS_TEXT "\t\"${DNA_HEADER_FILE}\",\n")
endforeach()

file(GENERATE OUTPUT ${DNA_HEADER_INC_FILE} CONTENT "${DNA_INCLUDE_TEXT}")
file(GENERATE OUTPUT ${DNA_HEADER_STR_FILE} CONTENT "${DNA_STRINGS_TEXT}")

# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PUBLIC ..
	
	# Internal Include Directories
	${CMAKE_CURRENT_BINARY_DIR}
	
)

# -----------------------------------------------------------------------------
# Declare Source Files

set(SRC
	${DNA_TYPEDEF_HEADERS}
	${DNA_DEFAULT_HEADERS}
	
	makedna.c
	
	genfile.c
	genfile.h
	
	${DNA_HEADER_INC_FILE}
	${DNA_HEADER_STR_FILE}
)

# -----------------------------------------------------------------------------
# Declare Library Files

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::source::roselib
	rose::source::translator
	
	# External Library Dependencies
)

# -----------------------------------------------------------------------------
# Declare Executable

add_executable(makedna "${SRC}")

rose_target_link_libraries(makedna "${LIB}")
rose_target_include_dirs(makedna "${INC}")

# -----------------------------------------------------------------------------
# Generate Files

add_custom_command(
	OUTPUT
		${CMAKE_CURRENT_BINARY_DIR}/dna.c
		${CMAKE_CURRENT_BINARY_DIR}/verify.c
	COMMAND
		"$<TARGET_FILE:makedna>"
			"--src" "${CMAKE_SOURCE_DIR}/source/rose/makedna"
			"--bin" "${CMAKE_CURRENT_BINARY_DIR}"
	DEPENDS
		makedna
		${DNA_TYPEDEF_HEADERS}
		${DNA_DEFAULT_HEADERS}
)

# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PUBLIC ..
	
	# Internal Include Directories
	PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
	
)

# -----------------------------------------------------------------------------
# Declare System Include Directories

set(INC_SYS
	# External Include Directories

)

# -----------------------------------------------------------------------------
# Declare Source Files

set(SRC
	${DNA_TYPEDEF_HEADERS}
	${DNA_DEFAULT_HEADERS}
	
	${CMAKE_CURRENT_BINARY_DIR}/dna.c
	${CMAKE_CURRENT_BINARY_DIR}/verify.c
	
	dna.cc
	dna.hh
	genfile.c
	genfile.h
	utils.c
	
	${DNA_HEADER_INC_FILE}
	${DNA_HEADER_STR_FILE}
)

# -----------------------------------------------------------------------------
# Declare Library Files

set(LIB
	# Internal Library Dependencies
	PUBLIC rose::intern::guardedalloc
	PUBLIC rose::source::roselib
	PUBLIC rose::source::translator
	
	# External Library Dependencies
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(dna "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::dna ALIAS dna)
