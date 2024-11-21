set(FREETYPE_VERSION "2.13.3")
set(FREETYPE_FOUND TRUE)

set(FREETYPE_INCLUDE_DIRS
    "${LIBDIR}/freetype/include"
    "${LIBDIR}/freetype/include/freetype"
)
set(FREETYPE_LIBRARY_STATIC
    "${LIBDIR}/freetype/lib/libfreetype.a"
)
if(NOT TARGET Freetype::Freetype)
    add_library(Freetype::Freetype UNKNOWN IMPORTED)
    set_target_properties(Freetype::Freetype PROPERTIES
        IMPORTED_LOCATION "${FREETYPE_LIBRARY_STATIC}"
        INTERFACE_INCLUDE_DIRECTORIES "${FREETYPE_INCLUDE_DIRS}"
    )
endif()

set(FREETYPE_LIBRARIES Freetype::Freetype)