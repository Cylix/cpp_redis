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
# project
###
set(PROJECT cpp_redis_tests)
project(${PROJECT} CXX)


###
# compilation options
###
if(NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif(NOT WIN32)


###
# includes
###
include_directories(${DEPS_INCLUDES} ${CPP_REDIS_INCLUDES})


###
# libraries
###
link_directories(${DEPS_LIBRARIES})


###
# executables
###
set(MAIN "sources/main.cpp")
set(DIRS "sources/spec" "sources/spec/**")
foreach(DIR ${DIRS})
  file(GLOB s_${DIR} "${DIR}/*.cpp")
  foreach(SOURCE ${s_${DIR}})
    get_filename_component(TEST_NAME ${SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${MAIN} ${SOURCE})
    target_link_libraries(${TEST_NAME} cpp_redis gtest)
    if(WIN32)
      target_link_libraries(${TEST_NAME} ws2_32)
    else()
      target_link_libraries(${TEST_NAME} pthread)
    endif(WIN32)
    add_test(NAME ${TEST_NAME}_tests COMMAND ${TEST_NAME})
  endforeach()
endforeach()
