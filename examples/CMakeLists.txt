###
# compilation options
###
IF (NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ENDIF (NOT WIN32)


###
# includes
###
include_directories(${PROJECT_SOURCE_DIR}/includes
                    ${CPP_REDIS_INCLUDES})


###
# executable
###
add_executable(redis_client redis_client.cpp)
target_link_libraries(redis_client cpp_redis)

add_executable(future_client future_client.cpp)
target_link_libraries(future_client cpp_redis)

add_executable(sync_client sync_client.cpp)
target_link_libraries(sync_client cpp_redis)

add_executable(redis_subscriber redis_subscriber.cpp)
target_link_libraries(redis_subscriber cpp_redis)

add_executable(logger logger.cpp)
target_link_libraries(logger cpp_redis)
