set(INC
	PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
)

set(INC_SYS
)

# Part of the `rose` binary (sources for data-files are appended).
set(SRC
)

set(LIB
	rose::source::editors::interface
)

set(SVG_FILENAMES_NOEXT
	file_3d
	file_blank
	file_folder
)

set(SVG_CONTENTS_H)
set(SVG_CONTENTS_C)

foreach(SVG ${SVG_FILENAMES_NOEXT})
	data_to_c_simple(${SVG}.svg SRC)
	string(TOUPPER ${SVG} SVG_UPPER)
	list(APPEND SVG_CONTENTS_H
		"extern const char datatoc_${SVG}_svg[]\;\n"
	)
	list(APPEND SVG_CONTENTS_C
		"\t\tcase ICON_${SVG_UPPER}: return datatoc_${SVG}_svg\;\n"
	)
endforeach()	

list(JOIN SVG_CONTENTS_C "" SVG_CONTENTS_C)
list(JOIN SVG_CONTENTS_H "" SVG_CONTENTS_H)

configure_file(
	"svg_icons.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/svg_icons.h"
	ESCAPE_QUOTES
	@ONLY
)
configure_file(
	"svg_icons.cc.in"
	"${CMAKE_CURRENT_BINARY_DIR}/svg_icons.cc"
	ESCAPE_QUOTES
	@ONLY
)

list(APPEND SRC ${CMAKE_CURRENT_BINARY_DIR}/svg_icons.h)
list(APPEND SRC ${CMAKE_CURRENT_BINARY_DIR}/svg_icons.cc)

rose_add_lib(icons "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::resources::icons ALIAS icons)
