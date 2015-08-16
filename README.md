# cpp_redis
cpp_redis is C++11 Asynchronous Redis Client.

Network is based on Boost Asio library.

## Requirements
* C++11
* Boost Asio

## Compiling
The library uses `cmake`. In order to build the library, follow these steps:

```bash
git clone https://github.com/Cylix/cpp_redis.git
cd cpp_redis
./install_deps.sh # necessary only for building tests
mkdir build
cd build
cmake .. # only library
cmake .. -DBUILD_TESTING=true # library and tests
cmake .. -DBUILD_EXAMPLES=true # library and examples
cmake .. -DBUILD_TESTING=true -DBUILD_EXAMPLES=true # library, tests and examples
make -j
```

Then, you just have to link the `cpp_redis` library with your project.

## Redis Client

## Redis Subscriber

## Examples
Some examples are provided in this repository:
* [redis_client.cpp](examples/redis_client.cpp) shows how to use the redis client class.
* [redis_subscriber.cpp](examples/redis_subscriber.cpp) shows how to use the redis subscriber class.

## To Do
| Type | Description | Priority | Status |
|------|-------------|----------|--------|
| Improvement | Add test coverage | High | To Do |
| Feature | Synchronous API | Medium | To Do |

## Author
[Simon Ninon](http://simon-ninon.fr)
