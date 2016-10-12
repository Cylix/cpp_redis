###
# config
###
cmake_minimum_required(VERSION 2.8.7)
set(CMAKE_MACOSX_RPATH 1)

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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -W -Wall -Wextra -O3")


###
# variables
###
set(DEPS_FOLDER ${PROJECT_SOURCE_DIR}/deps/build)
set(GTEST_INCLUDES ${DEPS_FOLDER}/gtest/include)
set(GTEST_LIBS ${DEPS_FOLDER}/gtest/lib)
set(CPP_REDIS_INCLUDES ${PROJECT_SOURCE_DIR}/includes)


###
# includes
###
include_directories(${CPP_REDIS_INCLUDES})


###
# link
###
link_directories(${GTEST_LIBS})


###
# sources
###
set(SRC_DIRS "sources" "sources/network" "sources/builders")

IF (WIN32)
  set(SRC_DIRS ${SRC_DIRS} "sources/network/windows")
ELSE ()
  set(SRC_DIRS ${SRC_DIRS} "sources/network/unix")
ENDIF (WIN32)

foreach(dir ${SRC_DIRS})
  # get directory sources
  file(GLOB s_${dir} "${dir}/*.cpp")
  # set sources
  set(SOURCES ${SOURCES} ${s_${dir}})
endforeach()


###
# executable
###
add_library(${PROJECT} SHARED ${SOURCES})
target_link_libraries(${PROJECT} pthread)
set_target_properties(${PROJECT}
                      PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/target")

IF (CPP_REDIS_READ_SIZE)
set_target_properties(${PROJECT}
                      PROPERTIES
                      COMPILE_DEFINITIONS "CPP_REDIS_READ_SIZE=${CPP_REDIS_READ_SIZE}")
ENDIF (CPP_REDIS_READ_SIZE)


###
# install
###
install (TARGETS ${PROJECT} DESTINATION lib)
install (DIRECTORY ${CPP_REDIS_INCLUDES}/ DESTINATION include)


###
# examples
###
IF (BUILD_EXAMPLES)
  add_subdirectory(examples)
ENDIF(BUILD_EXAMPLES)


###
# tests
###
IF (BUILD_TESTS)
  add_subdirectory(tests)
ENDIF(BUILD_TESTS)
