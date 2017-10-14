# The MIT License (MIT)
#
# Copyright (c) 2015-2017 Simon Ninon <simon.ninon@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

###
# config
###
cmake_minimum_required(VERSION 2.8.7)
set(CMAKE_MACOSX_RPATH 1)
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)


###
# verbose make
###
# set(CMAKE_VERBOSE_MAKEFILE TRUE)


###
# project
###
set(PROJECT cpp_redis)
project(${PROJECT} CXX)


###
# compilation options
###
IF (WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /O2 /bigobj")

  # was causing conflics with gtest build
  string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})

  IF ("${MSVC_RUNTIME_LIBRARY_CONFIG}" STREQUAL "")
    set(MSVC_RUNTIME_LIBRARY_CONFIG "/MT")
  ENDIF()

  foreach (flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE)
    IF ("${MSVC_RUNTIME_LIBRARY_CONFIG}" STREQUAL "/MT")
      string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    ELSEIF ("${MSVC_RUNTIME_LIBRARY_CONFIG}" STREQUAL "/MD")
      string(REPLACE "/MT" "/MD" ${flag_var} "${${flag_var}}")
    ELSE ()
      string(REPLACE "/MD" "${MSVC_RUNTIME_LIBRARY_CONFIG}" ${flag_var} "${${flag_var}}")
      string(REPLACE "/MT" "${MSVC_RUNTIME_LIBRARY_CONFIG}" ${flag_var} "${${flag_var}}")
    ENDIF()
  endforeach()

  add_definitions(-D_UNICODE)
  add_definitions(-DUNICODE)
  add_definitions(-DWIN32_LEAN_AND_MEAN)
ELSE ()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -W -Wall -Wextra -O3")
ENDIF (WIN32)


###
# variables
###
find_path(TACOPIE_INCLUDE_DIR tacopie/tacopie PATHS ${PROJECT_SOURCE_DIR}/tacopie/includes/)
find_library(TACOPIE_LIBRARY tacopie)
set(CPP_REDIS_INCLUDES ${PROJECT_SOURCE_DIR}/includes)
set(DEPS_INCLUDES ${PROJECT_SOURCE_DIR}/deps/include ${TACOPIE_INCLUDE_DIR})
set(DEPS_LIBRARIES ${PROJECT_SOURCE_DIR}/deps/lib)


###
# includes
###
include_directories(${CPP_REDIS_INCLUDES} ${DEPS_INCLUDES})


###
# sources
###
set(SRC_DIRS "sources"
             "sources/builders"
             "sources/core"
             "sources/misc"
             "sources/network"
             "includes/cpp_redis"
             "includes/cpp_redis/builders"
             "includes/cpp_redis/core"
             "includes/cpp_redis/misc"
             "includes/cpp_redis/network")

foreach(dir ${SRC_DIRS})
  # get directory sources and headers
  file(GLOB s_${dir} "${dir}/*.cpp")
  file(GLOB h_${dir} "${dir}/*.hpp")
  file(GLOB i_${dir} "${dir}/*.ipp")

  # set sources
  set(SOURCES ${SOURCES} ${s_${dir}} ${h_${dir}} ${i_${dir}})
endforeach()
# filter tcp_client if no tacopie
IF (USE_CUSTOM_TCP_CLIENT)
  list(REMOVE_ITEM SRC_DIRS "source/network/tcp_client.cpp" "include/cpp_redis/network/tcp_client.hpp")
ENDIF (USE_CUSTOM_TCP_CLIENT)


###
# outputs
###
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_PKGCONFIG_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/pkgconfig)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

###
# pkg-config
###
configure_file("cpp_redis.pc.in" "${CMAKE_PKGCONFIG_OUTPUT_DIRECTORY}/cpp_redis.pc" @ONLY)

###
# executable
###
add_library(${PROJECT} STATIC ${SOURCES})

if (WIN32)
  set_target_properties(${PROJECT}
                        PROPERTIES COMPILE_PDB_NAME ${PROJECT}
                        COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
ENDIF (WIN32)

IF (WIN32)
  target_link_libraries(${PROJECT} ws2_32)
ELSE ()
  target_link_libraries(${PROJECT} pthread)
ENDIF (WIN32)

if (TACOPIE_LIBRARY)
  target_link_libraries(${PROJECT} ${TACOPIE_LIBRARY})
else()
  target_link_libraries(${PROJECT} tacopie)
endif(TACOPIE_LIBRARY)


# __CPP_REDIS_READ_SIZE
IF (READ_SIZE)
  set_target_properties(${PROJECT} PROPERTIES COMPILE_DEFINITIONS "__CPP_REDIS_READ_SIZE=${READ_SIZE}")
ENDIF (READ_SIZE)

# __CPP_REDIS_LOGGING_ENABLED
IF (LOGGING_ENABLED)
  set_target_properties(${PROJECT} PROPERTIES COMPILE_DEFINITIONS "__CPP_REDIS_LOGGING_ENABLED=${LOGGING_ENABLED}")
ENDIF (LOGGING_ENABLED)

# __CPP_REDIS_USE_CUSTOM_TCP_CLIENT
IF (USE_CUSTOM_TCP_CLIENT)
  set_target_properties(${PROJECT} PROPERTIES COMPILE_DEFINITIONS "__CPP_REDIS_USE_CUSTOM_TCP_CLIENT=${USE_CUSTOM_TCP_CLIENT}")
ENDIF (USE_CUSTOM_TCP_CLIENT)


###
# install
###
# ensure lib and bin directories exist
install(DIRECTORY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
install(DIRECTORY DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
# install cpp_redis
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} DESTINATION lib USE_SOURCE_PERMISSIONS)
install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} DESTINATION bin USE_SOURCE_PERMISSIONS)
install(DIRECTORY ${CPP_REDIS_INCLUDES}/ DESTINATION include USE_SOURCE_PERMISSIONS)


###
# examples
###
IF (BUILD_EXAMPLES)
  add_subdirectory(examples)
  # Reset variable to false to ensure tacopie does no build examples
  set (BUILD_EXAMPLES false)
ENDIF(BUILD_EXAMPLES)

###
# tests
###
IF (BUILD_TESTS)
  add_subdirectory(tests)
  ExternalProject_Add("googletest"
                      GIT_REPOSITORY "https://github.com/google/googletest.git"
                      CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${PROJECT_SOURCE_DIR}/deps")
  # Reset variable to false to ensure tacopie does no build tests
  set (BUILD_TESTS false)
ENDIF(BUILD_TESTS)


###
# tacopie
###
IF (NOT TACOPIE_LIBRARY AND NOT USE_CUSTOM_TCP_CLIENT)
  add_subdirectory(tacopie)
ENDIF (NOT TACOPIE_LIBRARY AND NOT USE_CUSTOM_TCP_CLIENT)
