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

# This needs to be updated, this is really ugly!
find_package(HARFBUZZ REQUIRED
	PATHS ${LIBDIR}/harfbuzz
)
# This needs to be updated, this is really ugly!
find_package(FREETYPE REQUIRED
	PATHS ${LIBDIR}/freetype
)
