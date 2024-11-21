set(BROTLI_VERSION "1.1.0")
set(BROTLI_FOUND TRUE)

set(BROTLI_INCLUDE_DIRS
    "${LIBDIR}/brotli/include"
    "${LIBDIR}/brotli/include/brotli"
)
set(BROTLI_LIBRARY_STATIC
    "${LIBDIR}/brotli/lib/libbrotli.a"
)
if(NOT TARGET Brotli::Brotli)
    add_library(Brotli::Brotli UNKNOWN IMPORTED)
    set_target_properties(Brotli::Brotli PROPERTIES
        IMPORTED_LOCATION "${BROTLI_LIBRARY_STATIC}"
        INTERFACE_INCLUDE_DIRECTORIES "${BROTLI_INCLUDE_DIRS}"
    )
endif()

set(BROTLI_LIBRARIES Brotli::Brotli)