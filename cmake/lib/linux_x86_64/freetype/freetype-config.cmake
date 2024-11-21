include(ExternalProject)

ExternalProject_Add(
	freetype2a
	GIT_REPOSITORY git://git.savannah.gnu.org/freetype/freetype2.git
	GIT_SHALLOW 1
	CMAKE_ARGS -DCMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DWITH_PNG=Off
	UPDATE_COMMAND ""
)

ExternalProject_Get_Property(freetype2a install_dir)
set(freetype2a_install_dir install_dir)

ExternalProject_Add(
	harfbuzz
	DEPENDS freetype2a
	# PATCH_COMMAND git apply ${CMAKE_CURRENT_LIST_DIR}/harfbuzz_freetype_fix.patch
	# PATCH_COMMAND patch -p1 < ${CMAKE_CURRENT_LIST_DIR}/harfbuzz_freetype_fix.patch
	URL https://github.com/behdad/harfbuzz/releases/download/1.4.7/harfbuzz-1.4.7.tar.bz2
	CMAKE_ARGS -DCMAKE_PREFIX_PATH=${freetype2a_install_dir} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DHB_HAVE_FREETYPE=On 
	UPDATE_COMMAND ""
)

ExternalProject_Add(
	freetype2b
	GIT_REPOSITORY git://git.savannah.gnu.org/freetype/freetype2.git
	GIT_SHALLOW 1
	# PATCH_COMMAND git apply ${CMAKE_CURRENT_LIST_DIR}/freetype_harfbuzz_fix.patch
	CMAKE_ARGS -DCMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DWITH_PNG=Off
	UPDATE_COMMAND ""
)


include_directories(
	${CMAKE_INSTALL_PREFIX}/include/harfbuzz
	${CMAKE_INSTALL_PREFIX}/include/freetype2
)

if (UNIX)
	list(APPEND libs
		${CMAKE_INSTALL_PREFIX}/lib/libfreetype.a
		${CMAKE_INSTALL_PREFIX}/lib/libharfbuzz.a
		-lz
		-lbz2
	)
endif()

if (APPLE)
	find_library(fr_coretext CoreText)
	find_library(fr_corefoundation CoreFoundation)
	find_library(fr_coregraphics CoreGraphics)
	list(APPEND libs
		${fr_coretext}
		${fr_corefoundation}
		${fr_coregraphics}
	)
endif()

if(NOT TARGET Freetype::Freetype)
    add_library(Freetype::Freetype UNKNOWN IMPORTED)
    set_target_properties(Freetype::Freetype PROPERTIES
        IMPORTED_LOCATION "${FREETYPE_LIBRARY_STATIC}"
        INTERFACE_INCLUDE_DIRECTORIES "${FREETYPE_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${libs}"
    )
endif()

set(FREETYPE_LIBRARIES Freetype::Freetype)