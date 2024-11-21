set(BROTLI_VERSION "1.1.0")
set(BROTLI_FOUND TRUE)

set(BROTLI_INCLUDE_DIRS
    "${LIBDIR}/brotli/include"
    "${LIBDIR}/brotli/include/brotli"
)

set(BROTLI_LIBRARY_STATIC
    "${LIBDIR}/brotli/lib/libbrotlicommon.a"
    "${LIBDIR}/brotli/lib/libbrotlidec.a"
    "${LIBDIR}/brotli/lib/libbrotlienc.a"
)
set(BROTLI_LIBRARY_SHARED
    "${LIBDIR}/brotli/lib/libbrotlicommon.so"
    "${LIBDIR}/brotli/lib/libbrotlidec.so"
    "${LIBDIR}/brotli/lib/libbrotlienc.so"
)

add_library(Brotli INTERFACE)

foreach(LIB ${BROTLI_LIBRARY_STATIC})
    target_link_libraries(Brotli INTERFACE ${LIB})
endforeach()

target_include_directories(Brotli INTERFACE ${BROTLI_INCLUDE_DIRS})
target_link_libraries(Brotli INTERFACE ${BROTLI_LIBRARY_SHARED})

set(BROTLI_LIBRARIES Brotli)
