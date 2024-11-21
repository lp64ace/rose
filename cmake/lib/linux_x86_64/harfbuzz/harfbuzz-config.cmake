set(HARFBUZZ_VERSION "2.13.3")
set(HARFBUZZ_FOUND TRUE)

set(HARFBUZZ_INCLUDE_DIRS
    "${LIBDIR}/harfbuzz/include"
    "${LIBDIR}/harfbuzz/include/harfbuzz"
)
set(HARFBUZZ_LIBRARY_STATIC
    "${LIBDIR}/freetype/lib/libharfbuzz.a"
    "${LIBDIR}/freetype/lib/libharfbuzz-subset.a"
)
if(NOT TARGET HarfBuzz::HarfBuzz)
    add_library(HarfBuzz::HarfBuzz UNKNOWN IMPORTED)
    set_target_properties(HarfBuzz::HarfBuzz PROPERTIES
        IMPORTED_LOCATION "${HARFBUZZ_LIBRARY_STATIC}"
        INTERFACE_INCLUDE_DIRECTORIES "${HARFBUZZ_INCLUDE_DIRS}"
    )
endif()

set(HARFBUZZ_LIBRARIES HarfBuzz::HarfBuzz)
