add_definitions(-DLINUX)

# Path to a locally compiled libraries.
set(LIBDIR_NAME ${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR})
string(TOLOWER ${LIBDIR_NAME} LIBDIR_NAME)
set(LIBDIR_NATIVE_ABI ${CMAKE_SOURCE_DIR}/cmake/lib/${LIBDIR_NAME})

if(EXISTS ${LIBDIR_NATIVE_ABI})
    set(LIBDIR ${LIBDIR_NATIVE_ABI})
	message(STATUS "[${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR}] Linux detected.")
else()
	message(FATAL_ERROR "Unable to find LIBDIR_NATIVE_ABI: ${LIBDIR_NATIVE_ABI}.")
endif()

# -----------------------------------------------------------------------------
# Pre-Built Libraries

find_package(X11 REQUIRED)

if (NOT X11_FOUND)
    message(FATAL_ERROR "X11 not found. Install it using `sudo apt-get install libx11-dev`")
endif()

find_package(GLEW REQUIRED)

if (NOT GLEW_FOUND)
    message(FATAL_ERROR "GLEW not found. Install it using `sudo apt-get install libglew-dev`")
endif()

find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(PNG REQUIRED)

find_package(BROTLI REQUIRED
	HINTS ${LIBDIR}/brotli
)
find_package(HARFBUZZ REQUIRED
	HINTS ${LIBDIR}/harfbuzz
)
find_package(FREETYPE REQUIRED
	HINTS ${LIBDIR}/freetype
)

set(EIGEN3_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/extern/Eigen3)
