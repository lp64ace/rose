set(ZSTD_VERSION "10.1.0")
set(ZSTD_FOUND TRUE)

set(ZSTD_INCLUDE_DIRS
    "${LIBDIR}/zstd/include"
)
set(ZSTD_LIBRARY_STATIC
    "${LIBDIR}/zstd/lib/libzstd.a"
)
if(NOT TARGET zstd::libzstd)
    add_library(zstd::libzstd UNKNOWN IMPORTED)
    set_target_properties(zstd::libzstd PROPERTIES
        IMPORTED_LOCATION "${ZSTD_LIBRARY_STATIC}"
        INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_INCLUDE_DIRS}"
    )
endif()

set(ZSTD_LIBRARIES zstd::libzstd)