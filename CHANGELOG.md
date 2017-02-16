# Changelog

## [v3.1](https://github.com/Cylix/cpp_redis/releases/tag/3.1)
### Changes
* Fix: compilation for specific windows compilers concerning atomic variables
* Fix: handle correctly array replies with negative size by returning a null reply instead of throwing an invalid format exception
* Fix: Bump tacopie version to retrieve a fix concerning gethostbyname() thread-safety issue on unix
* Fix: compilation for programs based on Qt ('slots' conflict) 

### Additions
* Add some overloads for the Z set functions to support floating point values
* Add an auth method to the subscriber class to allow a subscriber to authenticate on the redis server

### Removals
None


## [v3.0](https://github.com/Cylix/cpp_redis/releases/tag/3.0)
### Changes
* Rewrite the network side of cpp_redis by using the [tacopie library](https://github.com/Cylix/tacopie)

### Additions
* Tacopie is now a submodule of cpp_redis

### Removals
* All network related code


## [v2.2](https://github.com/Cylix/cpp_redis/releases/tag/2.2)
### Changes
* Bug patch
* io_service is no longer a singleton
* `redis_client` and `redis_subscriber` instances can be assigned specific io_service

### Additions
* Integration Tests
* Travis Integration
* [Wiki](https://github.com/Cylix/cpp_redis/wiki)
* Port of the library on Windows
* Support for acknowledgement callbacks for `cpp_redis::redis_subscriber`
* Logging system
* Compilation Customizations

### Removals
None
