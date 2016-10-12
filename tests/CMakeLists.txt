###
# project
###
set(PROJECT cpp_redis_tests)
project(${PROJECT} CXX)


###
# compilation options
###
IF (NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ENDIF (NOT WIN32)


###
# includes
###
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/includes
                    ${CPP_REDIS_INCLUDES}
                    ${GTEST_INCLUDES})


###
# sources
###
set(SOURCES "")
set(DIRS "sources" "sources/spec" "sources/spec/**")
foreach(dir ${DIRS})
  # get directory sources
  file(GLOB s_${dir} "${dir}/*.cpp")
  # set sources
  set(SOURCES ${SOURCES} ${s_${dir}})
endforeach()


###
# executable
###
add_executable(${PROJECT} ${SOURCES})

target_link_libraries(${PROJECT} cpp_redis gtest)

IF (WIN32)
  target_link_libraries(${PROJECT} ws2_32)
ELSE ()
  target_link_libraries(${PROJECT} pthread)
ENDIF (WIN32)
