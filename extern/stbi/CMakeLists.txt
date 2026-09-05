set(INC
	PUBLIC .
)

set(SRC
	stb_image.c
	stb_image.h
	stbi_image_write.c
	stbi_image_write.h
)

set(LIB
)

rose_add_lib(stbi "${SRC}" "${INC}" "" "${LIB}")
add_library(rose::extern::stbi ALIAS stbi)
