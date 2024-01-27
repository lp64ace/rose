cmake_minimum_required(VERSION 3.10)

# -----------------------------------------------------------------------------
# Early Initialization

# NOTE: We don't allow in-source builds. This causes no end of troubles because
# all out-of-source builds will use the CMakeCache.txt file there and even
# build the libs and objects in it.
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	if(NOT DEFINED WITH_IN_SOURCE_BUILD)
	message(FATAL_ERROR
		"CMake generation for rose is not allowed within the source directory!"
		"\n Remove \"${CMAKE_SOURCE_DIR}/CMakeCache.txt\" and try again from another folder, e.g.:"
		"\n "
		"\n rm -rf CMakeCache.txt CMakeFiles"
		"\n cd .."
		"\n mkdir cmake-make"
		"\n cd cmake-make"
		"\n cmake ../rose"
		"\n "
		"\n Alternately define WITH_IN_SOURCE_BUILD to force this option (not recommended!)"
	)
	endif()
endif()

# -----------------------------------------------------------------------------
# Load Local Macros

include(build_files/cmake/macros.cmake)

# -----------------------------------------------------------------------------
# Initialize Project

project(rose)

# -----------------------------------------------------------------------------
# Redirect Output Files

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL "" FORCE)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib CACHE INTERNAL "" FORCE)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# -----------------------------------------------------------------------------
# Misc

if(WIN32)
	# Use hardcoded paths or find_package to find externals
	option(WITH_WINDOWS_FIND_MODULES "Use find_package to locate libraries" OFF)
	mark_as_advanced(WITH_WINDOWS_FIND_MODULES)

	option(WINDOWS_PYTHON_DEBUG "Include the files needed for debugging python scripts with visual studio 2017+." OFF)
	mark_as_advanced(WINDOWS_PYTHON_DEBUG)

	option(WITH_WINDOWS_BUNDLE_CRT "Bundle the C runtime for install free distribution." ON)
	mark_as_advanced(WITH_WINDOWS_BUNDLE_CRT)

	option(WITH_WINDOWS_SCCACHE "Use sccache to speed up builds (Ninja builder only)" OFF)
	mark_as_advanced(WITH_WINDOWS_SCCACHE)

	option(WITH_WINDOWS_PDB "Generate a pdb file for client side stacktraces" ON)
	mark_as_advanced(WITH_WINDOWS_PDB)

	option(WITH_WINDOWS_STRIPPED_PDB "Use a stripped PDB file" ON)
	mark_as_advanced(WITH_WINDOWS_STRIPPED_PDB)
endif()

if(WIN32 OR XCODE)
	option(IDE_GROUP_SOURCES_IN_FOLDERS "Organize the source files in filters matching the source folders." ON)
	mark_as_advanced(IDE_GROUP_SOURCES_IN_FOLDERS)

	option(IDE_GROUP_PROJECTS_IN_FOLDERS "Organize the projects according to source folder structure." ON)
	mark_as_advanced(IDE_GROUP_PROJECTS_IN_FOLDERS)

	if(IDE_GROUP_PROJECTS_IN_FOLDERS)
		set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	endif()
endif()

if(UNIX)
	# See WITH_WINDOWS_SCCACHE for Windows.
	option(WITH_COMPILER_CCACHE "Use ccache to improve rebuild times (Works with Ninja, Makefiles and Xcode)" OFF)
	mark_as_advanced(WITH_COMPILER_CCACHE)
endif()

# -----------------------------------------------------------------------------
# Extra Compile Flags

# Select C++17 as the standard for C++ projects.
set(CMAKE_CXX_STANDARD 17)
# If C++17 is not available, downgrading to an earlier standard is NOT OK.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Do not enable compiler specific language extensions.
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------------------------------------------------------
# Extra Compilation Definitions

if(WIN32)
	add_compile_definitions(-DWIN32)
endif()

# -----------------------------------------------------------------------------
# Libraries

find_package(OpenGL REQUIRED)

# -----------------------------------------------------------------------------
# Copy clang format files

configure_file(${CMAKE_SOURCE_DIR}/.clang-format ${CMAKE_BINARY_DIR}/.clang-format COPYONLY)

# -----------------------------------------------------------------------------
# Download External Libraries

if(WIN32)
    # Download pthread-win32 from GitHub
    download_dependency_module(pthread https://github.com/GerHobbelt/pthread-win32 master)

    # Set pthread include and libraries
    set(PTHREADS_INC ${CMAKE_BINARY_DIR}/extern/pthread)
    set(PTHREADS_LIB pthreadVCE3 pthreadVSE3 pthreadVC3)

    # Set target properties for organization
    set_target_properties(pthreadVCE3 PROPERTIES FOLDER "extern")
    set_target_properties(pthreadVSE3 PROPERTIES FOLDER "extern")
    set_target_properties(pthreadVC3 PROPERTIES FOLDER "extern")
	
	# Mute warnings for external projects
	target_compile_options(pthreadVCE3 PRIVATE /W0)
	target_compile_options(pthreadVSE3 PRIVATE /W0)
	target_compile_options(pthreadVC3 PRIVATE /W0)
endif()

if(UNIX)
  # On UNIX, you can use the FindThreads module to locate the pthread library
  find_package(Threads REQUIRED)

  # Set pthread include and libraries
  set(PTHREADS_INC ${CMAKE_THREAD_LIBS_INIT})
  set(PTHREADS_LIB ${CMAKE_THREAD_LIBS_INIT})
endif()

# -----------------------------------------------------------------------------
# Add Subdirectories

add_subdirectory(extern)
add_subdirectory(intern)
add_subdirectory(source)
