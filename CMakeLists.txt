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
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /O2")
  add_definitions(-D_UNICODE)
  add_definitions(-DUNICODE)
  add_definitions(-DWIN32_LEAN_AND_MEAN)
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
set(SRC_DIRS "sources" "sources/network" "sources/builders")

IF (WIN32)
  set(SRC_DIRS ${SRC_DIRS} "sources/network/windows_impl")
ELSE ()
  set(SRC_DIRS ${SRC_DIRS} "sources/network/unix_impl")
ENDIF (WIN32)

foreach(dir ${SRC_DIRS})
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

IF (WIN32)
  target_link_libraries(${PROJECT} ws2_32)
ELSE ()
  target_link_libraries(${PROJECT} pthread)
ENDIF (WIN32)

# __CPP_REDIS_READ_SIZE
IF (READ_SIZE)
set_target_properties(${PROJECT} PROPERTIES COMPILE_DEFINITIONS "__CPP_REDIS_READ_SIZE=${READ_SIZE}")
ENDIF (READ_SIZE)

# __CPP_REDIS_LOGGING_ENABLED
IF (LOGGING_ENABLED)
set_target_properties(${PROJECT} PROPERTIES COMPILE_DEFINITIONS "__CPP_REDIS_LOGGING_ENABLED=${LOGGING_ENABLED}")
ENDIF (LOGGING_ENABLED)

# _CPP_REDIS_MAX_NB_FDS
IF (MAX_NB_FDS)
set_target_properties(${PROJECT} PROPERTIES COMPILE_DEFINITIONS "_CPP_REDIS_MAX_NB_FDS=${MAX_NB_FDS}")
ENDIF (MAX_NB_FDS)

# __CPP_REDIS_DEFAULT_NB_IO_SERVICE_WORKERS
IF (DEFAULT_NB_IO_SERVICE_WORKERS)
set_target_properties(${PROJECT} PROPERTIES COMPILE_DEFINITIONS "__CPP_REDIS_DEFAULT_NB_IO_SERVICE_WORKERS=${DEFAULT_NB_IO_SERVICE_WORKERS}")
ENDIF (DEFAULT_NB_IO_SERVICE_WORKERS)


###
# install
###
# ensure lib and bin directories exist
install(DIRECTORY DESTINATION ${CMAKE_BINARY_DIR}/lib/)
install(DIRECTORY DESTINATION ${CMAKE_BINARY_DIR}/bin/)
# install cpp_redis
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
