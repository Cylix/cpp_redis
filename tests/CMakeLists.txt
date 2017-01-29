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
