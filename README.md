<p align="center">
   <img src="https://raw.githubusercontent.com/Cylix/cpp_redis/master/assets/images/cpp_redis_logo.jpg"/>
</p>

# cpp_redis [![Build Status](https://travis-ci.org/Cylix/cpp_redis.svg?branch=master)](https://travis-ci.org/Cylix/cpp_redis)
`cpp_redis` is a C++11 Asynchronous Multi-Platform Lightweight Redis Client, with support for synchronous operations and pipelining.

## Requirement
`cpp_redis` has **no dependency**. Its only requirement is `C++11`.

## Example
### cpp_redis::redis_client
```cpp
cpp_redis::redis_client client;

client.connect();

client.set("hello", "42");
client.get("hello", [](cpp_redis::reply& reply) {
  std::cout << reply << std::endl;
});

client.sync_commit();
# or client.commit(); for synchronous call
```
`cpp_redis::redis_client` [full documentation](https://github.com/Cylix/cpp_redis/wiki/Redis-Client) and [detailed example](https://github.com/Cylix/cpp_redis/wiki/Examples#redis-client).
More about [cpp_redis::reply](https://github.com/Cylix/cpp_redis/wiki/Replies).

### cpp_redis::redis_subscriber
```cpp
cpp_redis::redis_subscriber sub;

sub.connect();

sub.subscribe("some_chan", [](const std::string& chan, const std::string& msg) {
  std::cout << "MESSAGE " << chan << ": " << msg << std::endl;
});
sub.psubscribe("*", [](const std::string& chan, const std::string& msg) {
  std::cout << "PMESSAGE " << chan << ": " << msg << std::endl;
});

sub.sync_commit();
# or sub.commit(); for synchronous call
```
`cpp_redis::redis_subscriber` [full documentation](https://github.com/Cylix/cpp_redis/wiki/Redis-Subscriber) and [detailed example](https://github.com/Cylix/cpp_redis/wiki/Examples#redis-subscriber).

### cpp_redis::future_client
```cpp
cpp_redis::future_client client;

client.connect();

auto set    = client.set("hello", "42");
auto decrby = client.decrby("hello", 12);
auto get    = client.get("hello");

client.sync_commit();
# or client.commit(); for synchronous call

std::cout << "set 'hello' 42: " << set.get() << std::endl;
std::cout << "After 'hello' decrement by 12: " << decrby.get() << std::endl;
std::cout << "get 'hello': " << get.get() << std::endl;
```
`cpp_redis::future_client` [full documentation](https://github.com/Cylix/cpp_redis/wiki/Future-Client) and [detailed example](https://github.com/Cylix/cpp_redis/wiki/Examples#future-client).

## Wiki
A [Wiki](https://github.com/Cylix/cpp_redis/wiki) is available and provides full documentation for the library as well as [installation explanations](https://github.com/Cylix/cpp_redis/wiki/Installation).

## License
`cpp_redis` is under [MIT License](LICENSE).

## Contributing
Please refer to [CONTRIBUTING.md](CONTRIBUTING.md).

## Special Thanks

* [Frank Pagliughi](https://github.com/fpagliughi) for adding support of futures to the library
* [Mike Moening](https://github.com/MikesAracade) for his unexpected and incredible great work aiming to port cpp_redis on Windows!
* [Tobias Gustafsson](https://github.com/tobbe303) for contributing and reporting issues
* Alexis Vasseur for reporting me issues and spending time helping me debugging
* Pawel Lopko	for reporting me issues

## Author
[Simon Ninon](http://simon-ninon.fr)
