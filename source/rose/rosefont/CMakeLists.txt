# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PUBLIC .
	
	# Internal Include Directories
	${CMAKE_SOURCE_DIR}/intern/memalloc
	${CMAKE_SOURCE_DIR}/source/rose/roselib
	${CMAKE_SOURCE_DIR}/source/rose/makesdna
	${CMAKE_SOURCE_DIR}/source/rose/gpu
	
)

# -----------------------------------------------------------------------------
# Declare System Include Directories

set(INC_SYS
	# External Include Directories
	${FREETYPE_INC}
	${PTHREADS_INC}
	
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
	
	# External Include Directories
	${FREETYPE_LIB}
	${PTHREADS_LIB}
	
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

rose_add_lib(rf_source_fonts "${FONTS_C}" "" "" "")
add_library(rf::source::fonts ALIAS rf_source_fonts)

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
list(APPEND LIB rf::source::fonts)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(rf_source_rosefont "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rf::source::rosefont ALIAS rf_source_rosefont)
