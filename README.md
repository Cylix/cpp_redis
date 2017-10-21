<p align="center">
   <img src="https://raw.githubusercontent.com/Cylix/cpp_redis/master/assets/images/cpp_redis_logo.jpg"/>
</p>

# cpp_redis [![Build Status](https://travis-ci.org/Cylix/cpp_redis.svg?branch=master)](https://travis-ci.org/Cylix/cpp_redis)
`cpp_redis` is a C++11 Asynchronous Multi-Platform Lightweight Redis Client, with support for synchronous operations, pipelining, sentinels and high availability.

## Requirement
`cpp_redis` has **no dependency**. Its only requirement is `C++11`.

It comes with no network module, so you are free to configure your own, or to use the default one ([tacopie](https://github.com/cylix/tacopie))

## Example
### cpp_redis::client
```cpp
cpp_redis::client client;

client.connect();

client.set("hello", "42");
client.get("hello", [](cpp_redis::reply& reply) {
  std::cout << reply << std::endl;
});
//! also support std::future
//! std::future<cpp_redis::reply> get_reply = client.get("hello");

client.sync_commit();
//! or client.commit(); for synchronous call
```
`cpp_redis::client` [full documentation](https://github.com/Cylix/cpp_redis/wiki/Redis-Client) and [detailed example](https://github.com/Cylix/cpp_redis/wiki/Examples#redis-client).
More about [cpp_redis::reply](https://github.com/Cylix/cpp_redis/wiki/Replies).

### cpp_redis::subscriber
```cpp
cpp_redis::subscriber sub;

sub.connect();

sub.subscribe("some_chan", [](const std::string& chan, const std::string& msg) {
  std::cout << "MESSAGE " << chan << ": " << msg << std::endl;
});
sub.psubscribe("*", [](const std::string& chan, const std::string& msg) {
  std::cout << "PMESSAGE " << chan << ": " << msg << std::endl;
});

sub.sync_commit();
//! or sub.commit(); for synchronous call
```
`cpp_redis::subscriber` [full documentation](https://github.com/Cylix/cpp_redis/wiki/Redis-Subscriber) and [detailed example](https://github.com/Cylix/cpp_redis/wiki/Examples#redis-subscriber).

## Wiki
A [Wiki](https://github.com/Cylix/cpp_redis/wiki) is available and provides full documentation for the library as well as [installation explanations](https://github.com/Cylix/cpp_redis/wiki/Installation).

# Doxygen
A [Doxygen documentation](https://cylix.github.io/cpp_redis/html/) is available and provides full API documentation for the library.

## License
`cpp_redis` is under [MIT License](LICENSE).

## Contributing
Please refer to [CONTRIBUTING.md](CONTRIBUTING.md).

## Special Thanks
[Mike Moening](https://github.com/MikesAracade) for his unexpected and incredible great work aiming to port cpp_redis on Windows, provides sentinel support and high availability support!

## Author
[Simon Ninon](http://simon-ninon.fr)
