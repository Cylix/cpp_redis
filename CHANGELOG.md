# Changelog

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
