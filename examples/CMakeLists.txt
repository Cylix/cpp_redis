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
# compilation options
###
IF (NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ENDIF (NOT WIN32)


###
# includes
###
include_directories(${CPP_REDIS_INCLUDES})


###
# libraries
###
link_directories(${DEPS_LIBRARIES})


###
# executable
###
add_executable(cpp_redis_client cpp_redis_client.cpp)
target_link_libraries(cpp_redis_client cpp_redis)

add_executable(cpp_redis_future_client cpp_redis_future_client.cpp)
target_link_libraries(cpp_redis_future_client cpp_redis)

add_executable(cpp_redis_subscriber cpp_redis_subscriber.cpp)
target_link_libraries(cpp_redis_subscriber cpp_redis)

add_executable(cpp_redis_logger cpp_redis_logger.cpp)
target_link_libraries(cpp_redis_logger cpp_redis)

add_executable(cpp_redis_kill cpp_redis_kill.cpp)
target_link_libraries(cpp_redis_kill cpp_redis)

add_executable(cpp_redis_high_availability_client cpp_redis_high_availability_client.cpp)
target_link_libraries(cpp_redis_high_availability_client cpp_redis)

###
# link libs
###
IF (WIN32)
  target_link_libraries(cpp_redis_client ws2_32)
  target_link_libraries(cpp_redis_future_client ws2_32)
  target_link_libraries(cpp_redis_subscriber ws2_32)
  target_link_libraries(cpp_redis_logger ws2_32)
  target_link_libraries(cpp_redis_kill ws2_32)
  target_link_libraries(cpp_redis_high_availability_client ws2_32)
ELSE ()
  target_link_libraries(cpp_redis_client pthread)
  target_link_libraries(cpp_redis_future_client pthread)
  target_link_libraries(cpp_redis_subscriber pthread)
  target_link_libraries(cpp_redis_logger pthread)
  target_link_libraries(cpp_redis_kill pthread)
  target_link_libraries(cpp_redis_high_availability_client pthread)
ENDIF (WIN32)
