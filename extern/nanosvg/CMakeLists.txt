set(INC
	PUBLIC .
)

set(SRC
	nanosvg.h
	nanosvg.c
	nanosvgrast.c
	nanosvgrast.h
)

set(LIB
)

rose_add_lib(nanosvg "${SRC}" "${INC}" "" "${LIB}")
add_library(rose::extern::nanosvg ALIAS nanosvg)
