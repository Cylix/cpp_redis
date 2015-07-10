###
# compilation options
###
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


###
# includes
###
include_directories(${PROJECT_SOURCE_DIR}/includes
                    ${CPP_REDIS_INCLUDES})


###
# executable
###
add_executable(cpp_redis_example main.cpp)
target_link_libraries(cpp_redis_example cpp_redis)
set_target_properties(cpp_redis_example
                      PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/target")
