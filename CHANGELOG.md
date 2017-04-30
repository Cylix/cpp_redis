# Changelog

## [v3.5.1](https://github.com/Cylix/cpp_redis/releases/tag/3.5.1)
### Tag
`3.5.1`.
### Date
April 30th, 2017
### Changes
* Fix compilations on windows
* Fix reconnection behavior
* Do not clear commands/callback buffer on calling commit or sync_commit while client is disconnected.
### Additions
None
### Removals
None



## [v3.5.0](https://github.com/Cylix/cpp_redis/releases/tag/3.5.0)
### Tag
`3.5.0`.
### Date
April 9th, 2017
### Changes
None
### Additions
* New feature - Update tacopie ref - Provide support for Unix socket. Simply pass in 0 as the port when building a `redis_client`, `redis_subscriber` or `future_client`. Then, the host will automatically be treated as the path to a Unix socket instead of a real host. - More in #67.
### Removals
None



## [v3.4.0](https://github.com/Cylix/cpp_redis/releases/tag/3.4.0)
### Tag
`3.4.0`.
### Changes
* Change: update tacopie ref - IO Service is now based on `select` and not on `poll` anymore to solve some issues encountered on windows due to the buggy implementation of `poll` on windows Systems.
### Additions
None
### Removals
None



## [v3.3.0](https://github.com/Cylix/cpp_redis/releases/tag/3.3.0)
### Tag
`3.3.0`.
### Changes
* Rename `redis_client::before_callback` into `redis_client::set_callback_runner` which is more relevant.
* Before, `future_client` automatically called `.commit` when sending a command, meaning that no pipelining was done for the `future_client`. This has been changed and the `future_client` do not call `.commit` anymore: this is a **breaking** change and you **must** call `.commit` or `.sync_commit` when you wish the commands to be effectively sent. Please refer to the examples if necessary.
### Additions
* Add `commit` and `sync_commit` methodsto the `future_client` for pipelining support.
* documentation for `redis_client::before_callback` has been added
* documentation for `future_client` has been added
### Removals
None



## [v3.2.1](https://github.com/Cylix/cpp_redis/releases/tag/3.2.1)
### Tag
`3.2.1`.
### Changes
* Fix static initialization order fiasco condition
* Change `__CPP_REDIS_USE_TACOPIE` (cmake variable: `USE_TACOPIE`) into `__CPP_REDIS_USE_CUSTOM_TCP_CLIENT` (cmake variable: `USE_CUSTOM_TCP_CLIENT`). Of course, the meaning is now the opposite.
### Additions
None
### Removals
None



## [v3.2.0](https://github.com/Cylix/cpp_redis/releases/tag/3.2.0)
### Tag
`3.2.0`.
### Changes
* tacopie is no longer a mandatory dependency, but just provided by default and can be override if necessary.
### Additions
* add a new interface, `cpp_redis::network::tcp_client_iface` that allows you to use your own tcp_client in place of tacopie.
### Removals
* The `sync_client` has been removed as it was a duplicate of `redis_client::sync_commit` but with a different implementation based on futures. Please use `redis_client` and call `sync_commit` instead.



## [v3.1.2](https://github.com/Cylix/cpp_redis/releases/tag/3.1.2)
### Tag
`3.1.2`.
### Changes
* rename the `setbit()` function into `setbit_()` in order to avoid conflict with the standard library macro `setbit` causing compilation error.
### Additions
* add `send()` method to the `sync_client` and `future_client`.
### Removals
None



## [v3.1.1](https://github.com/Cylix/cpp_redis/releases/tag/3.1.1)
### Tag
`3.1.1`.
### Changes
* Fix: subscriber callbacks were sometimes not called due to poll not listening to the appropriate events. Mostly impacted windows as referred in #51, but unix version might also be impacted. Fixed by updating the reference tacopie which contains the fix.
### Additions
None
### Removals
None



## [v3.1.0](https://github.com/Cylix/cpp_redis/releases/tag/3.1.0)
### Tag
`3.1.0`.
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
### Tag
`3.0.0`.
### Changes
* Rewrite the network side of cpp_redis by using the [tacopie library](https://github.com/Cylix/tacopie)
### Additions
* Tacopie is now a submodule of cpp_redis
### Removals
* All network related code




## [v2.2](https://github.com/Cylix/cpp_redis/releases/tag/2.2)
### Tag
`2.2`.
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
