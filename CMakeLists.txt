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
IF (WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /O2 -D '_UNICODE' -D 'UNICODE' -D 'WIN32_LEAN_AND_MEAN'")
ELSE ()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -W -Wall -Wextra -O3")
ENDIF (WIN32)


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
set(DIRS "sources" "sources/network" "sources/builders")
foreach(dir ${DIRS})
  # get directory sources
  file(GLOB s_${dir} "${dir}/*.cpp")
  # set sources
  set(SOURCES ${SOURCES} ${s_${dir}})
endforeach()


###
# outputs
###
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


###
# executable
###
add_library(${PROJECT} STATIC ${SOURCES})
target_link_libraries(${PROJECT} pthread)

IF (READ_SIZE)
set_target_properties(${PROJECT}
                      PROPERTIES
                      COMPILE_DEFINITIONS "__CPP_REDIS_READ_SIZE=${READ_SIZE}")
ENDIF (READ_SIZE)

IF (NO_LOGGING)
set_target_properties(${PROJECT}
                      PROPERTIES
                      COMPILE_DEFINITIONS "__CPP_REDIS_NO_LOGGING=${NO_LOGGING}")
ENDIF (NO_LOGGING)

###
# install
###
install (DIRECTORY ${CMAKE_BINARY_DIR}/lib/ DESTINATION lib USE_SOURCE_PERMISSIONS)
install (DIRECTORY ${CMAKE_BINARY_DIR}/bin/ DESTINATION bin USE_SOURCE_PERMISSIONS)
install (DIRECTORY ${CPP_REDIS_INCLUDES}/ DESTINATION include USE_SOURCE_PERMISSIONS)


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
