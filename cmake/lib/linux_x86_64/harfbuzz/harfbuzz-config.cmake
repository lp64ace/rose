set(HARFBUZZ_VERSION "10.1.0")
set(HARFBUZZ_FOUND TRUE)

set(HARFBUZZ_INCLUDE_DIRS
    "${LIBDIR}/harfbuzz/include"
    "${LIBDIR}/harfbuzz/include/harfbuzz"
)
set(HARFBUZZ_LIBRARY_STATIC
    "${LIBDIR}/harfbuzz/lib/libharfbuzz.a"
    "${LIBDIR}/harfbuzz/lib/libharfbuzz-subset.a"
)

if(NOT TARGET HarfBuzz::HarfBuzz)
    add_library(HarfBuzz::HarfBuzz UNKNOWN IMPORTED)
    set_target_properties(HarfBuzz::HarfBuzz PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${HARFBUZZ_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${HARFBUZZ_LIBRARY_STATIC}"
    )
endif()

set(HARFBUZZ_LIBRARIES HarfBuzz::HarfBuzz)