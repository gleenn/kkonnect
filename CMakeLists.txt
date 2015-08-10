# This file is part of the KKonnect project.
#
# Copyright (c) 2015 individual KKonnect contributors. See the CONTRIB file
# for details.
#
# This code is licensed to you under the terms of the Apache License, version
# 2.0, or, at your option, the terms of the GNU General Public License,
# version 2.0. See the APACHE20 and GPL2 files for the text of the licenses,
# or the following URLs:
# http://www.apache.org/licenses/LICENSE-2.0
# http://www.gnu.org/licenses/gpl-2.0.txt
#
# If you redistribute this file in source form, modified or unmodified, you
# may:
#   1) Leave this header intact and distribute it under the same terms,
#      accompanying it with the APACHE20 and GPL20 files, or
#   2) Delete the Apache 2.0 clause and accompany it with the GPL2 file, or
#   3) Delete the GPL v2 clause and accompany it with the APACHE20 file
# In all cases you must keep the copyright notice intact and include a copy
# of the CONTRIB file.
#
# Binary distributions must follow the binary distribution requirements of
# either License.

################################################################################
# Project declarations
################################################################################

cmake_minimum_required (VERSION 2.8)
set (PYTHON_EXECUTABLE "python2")

project (libkkonnect)

set (PROJECT_VER_MAJOR 0)
set (PROJECT_VER_MINOR 1)
set (PROJECT_VER_PATCH 0)
set (PROJECT_VER
  "${PROJECT_VER_MAJOR}.${PROJECT_VER_MINOR}.${PROJECT_VER_PATCH}")
set (PROJECT_APIVER
  "${PROJECT_VER_MAJOR}.${PROJECT_VER_MINOR}")

option (BUILD_EXAMPLES "Build example programs" ON)

################################################################################
# External Dependencies
################################################################################

include (ExternalProject)

ExternalProject_Add (project_libfreenect
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/libfreenect
  PREFIX libfreenect
  INSTALL_COMMAND ""
)

ExternalProject_Add (project_libfreenect2
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/libfreenect2/examples/protonect
  PREFIX libfreenect2
  INSTALL_COMMAND ""
)

ExternalProject_Add_Step (project_libfreenect2
    static_lib_make_dir
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_BINARY_DIR}/libfreenect2/src/project_libfreenect2-build/lib/
    COMMENT "Making directory for libfreenect2 static library"
)

ExternalProject_Add_Step (project_libfreenect2
    static_lib_copy
    DEPENDEES static_lib_make_dir
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy external/libfreenect2/examples/protonect/lib/libfreenect2.a
        ${CMAKE_BINARY_DIR}/libfreenect2/src/project_libfreenect2-build/lib/
    COMMENT "Copying libfreenect2 static library"
)

add_library (libfreenect STATIC IMPORTED)
set_target_properties (libfreenect PROPERTIES IMPORTED_LOCATION
    ${CMAKE_BINARY_DIR}/libfreenect/src/project_libfreenect-build/lib/libfreenect.a)
add_dependencies (libfreenect project_libfreenect)

add_library (libfreenect2 STATIC IMPORTED)
set_target_properties (libfreenect2 PROPERTIES IMPORTED_LOCATION
    ${CMAKE_BINARY_DIR}/libfreenect2/src/project_libfreenect2-build/lib/libfreenect2.a)
add_dependencies (libfreenect2 project_libfreenect2)

################################################################################
# CMake
################################################################################

set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

set (C_FLAGS_WARNING "-Wall")

set (C_CXX_FLAGS_DEFAULT "${C_FLAGS_WARNING} -O2")

set (CMAKE_C_FLAGS "${C_CXX_FLAGS_DEFAULT} ${CMAKE_C_FLAGS}")
set (CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS} -O0 -g -DDEBUG=1")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS}")
set (CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} -g")

set (CMAKE_CXX_FLAGS "${C_CXX_FLAGS_DEFAULT} ${CMAKE_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS} -O0 -g -DDEBUG=1")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -g")

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories (${CMAKE_CURRENT_SOURCE_DIR})
#include_directories (${CMAKE_CURRENT_SOURCE_DIR}/external/libfreenect/include)
#include_directories (${CMAKE_CURRENT_SOURCE_DIR}/external/libfreenect2/include)

add_subdirectory (src)

if (BUILD_EXAMPLES)
  #add_subdirectory (examples)
endif()
