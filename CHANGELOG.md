# Changelog

## [v3.2.0](https://github.com/Cylix/cpp_redis/releases/tag/3.2.0)
### Changes
* tacopie is no longer a mandatory dependency, but just provided by default and can be override if necessary.

### Additions
* add a new interface, `cpp_redis::network::tcp_client_iface` that allows you to use your own tcp_client in place of tacopie.

### Removals
* The `sync_client` has been removed as it was a duplicate of `redis_client::sync_commit` but with a different implementation based on futures. Please use `redis_client` and call `sync_commit` instead.

## [v3.1.2](https://github.com/Cylix/cpp_redis/releases/tag/3.1.2)
### Changes
* rename the `setbit()` function into `setbit_()` in order to avoid conflict with the standard library macro `setbit` causing compilation error.

### Additions
* add `send()` method to the `sync_client` and `future_client`.

### Removals
None

## [v3.1.1](https://github.com/Cylix/cpp_redis/releases/tag/3.1.1)
### Changes
* Fix: subscriber callbacks were sometimes not called due to poll not listening to the appropriate events. Mostly impacted windows as referred in #51, but unix version might also be impacted. Fixed by updating the reference tacopie which contains the fix.

### Additions
None

### Removals
None

## [v3.1.0](https://github.com/Cylix/cpp_redis/releases/tag/3.1.0)
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


## [v3.0.0](https://github.com/Cylix/cpp_redis/releases/tag/3.0.0)
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
