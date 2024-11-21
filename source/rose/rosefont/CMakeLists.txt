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
	${PTHREADS_INCLUDE_DIRS}
	${FREETYPE_INCLUDE_DIRS}
	
)

# -----------------------------------------------------------------------------
# Declare Source Files

set(SRC
	RFT_api.h
	
	intern/rft.c
	intern/rft_default.c
	intern/rft_dir.c
	intern/rft_font.c
	intern/rft_glyph.c
	intern/rft_internal.c
	intern/rft_internal.h
	intern/rft_internal_types.h
	
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	rose::intern::guardedalloc
	rose::source::gpu
	rose::source::roselib
	
	# External Include Directories
	${PTHREADS_LIBRARIES}
	PUBLIC ${FREETYPE_LIBRARIES}
	
)

# -----------------------------------------------------------------------------
# Internal Fonts

set(FONTS_SRC
	fonts/DejaVuSans.ttf
	fonts/DejaVuSansMono.ttf
)

set(FONTS_C
)
foreach(FONT_FILE ${FONTS_SRC})
	data_to_c_simple(${FONT_FILE} FONTS_C)
endforeach()

rose_add_lib(fonts "${FONTS_C}" "" "" "")
add_library(rose::source::fonts ALIAS fonts)

set(FONT_SOURCE_CONTENT
	""
)
foreach(FONT_FILE ${FONTS_SRC})
  get_filename_component(FONT_FILE_NAME ${FONT_FILE} NAME)
  string(REPLACE "." "_" FONT_FILE_NAME_UNDERSCORES ${FONT_FILE_NAME})
  string(APPEND FONT_SOURCE_CONTENT "FONT_SOURCE\(datatoc_${FONT_FILE_NAME_UNDERSCORES}, \"${FONT_FILE_NAME}\", \"${FONT_FILE}\"\)\n")
endforeach()

set(font_source_list_file
	"${CMAKE_CURRENT_BINARY_DIR}/rft_font_source_list.h"
)
file(GENERATE OUTPUT ${font_source_list_file} CONTENT "${FONT_SOURCE_CONTENT}")

list(APPEND INC ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND LIB rose::source::fonts)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(rosefont "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::rosefont ALIAS rosefont)
