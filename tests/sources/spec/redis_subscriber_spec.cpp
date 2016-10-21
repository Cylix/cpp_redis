#include <cpp_redis/redis_client.hpp>
#include <cpp_redis/redis_subscriber.hpp>
#include <gtest/gtest.h>

TEST(RedisSubscriber, ValidConnectionDefaultParams) {
  cpp_redis::redis_subscriber client;

  EXPECT_FALSE(client.is_connected());
  //! should connect to 127.0.0.1:6379
  EXPECT_NO_THROW(client.connect());
  EXPECT_TRUE(client.is_connected());
}

TEST(RedisSubscriber, ValidConnectionDefinedHost) {
  cpp_redis::redis_subscriber client;

  EXPECT_FALSE(client.is_connected());
  //! should connect to 127.0.0.1:6379
  EXPECT_NO_THROW(client.connect("127.0.0.1", 6379));
  EXPECT_TRUE(client.is_connected());
}

TEST(RedisSubscriber, InvalidConnection) {
  cpp_redis::redis_subscriber client;

  EXPECT_FALSE(client.is_connected());
  EXPECT_THROW(client.connect("invalid.url", 1234), cpp_redis::redis_error);
  EXPECT_FALSE(client.is_connected());
}

TEST(RedisSubscriber, AlreadyConnected) {
  cpp_redis::redis_subscriber client;

  EXPECT_FALSE(client.is_connected());
  //! should connect to 127.0.0.1:6379
  EXPECT_NO_THROW(client.connect());
  EXPECT_TRUE(client.is_connected());
  EXPECT_THROW(client.connect(), cpp_redis::redis_error);
  EXPECT_TRUE(client.is_connected());
}

TEST(RedisSubscriber, Disconnection) {
  cpp_redis::redis_subscriber client;

  client.connect();
  EXPECT_TRUE(client.is_connected());
  client.disconnect();
  EXPECT_FALSE(client.is_connected());
}

TEST(RedisSubscriber, DisconnectionNotConnected) {
  cpp_redis::redis_subscriber client;

  EXPECT_FALSE(client.is_connected());
  EXPECT_NO_THROW(client.disconnect());
  EXPECT_FALSE(client.is_connected());
}

TEST(RedisSubscriber, CommitConnected) {
  cpp_redis::redis_subscriber client;

  client.connect();
  EXPECT_NO_THROW(client.commit());
}

TEST(RedisSubscriber, CommitNotConnected) {
  cpp_redis::redis_subscriber client;

  EXPECT_THROW(client.commit(), cpp_redis::redis_error);
}

TEST(RedisSubscriber, SubscribeConnected) {
  cpp_redis::redis_subscriber client;

  client.connect();
  EXPECT_NO_THROW(client.subscribe("/chan", [](const std::string&, const std::string&) {}));
}

TEST(RedisSubscriber, PSubscribeConnected) {
  cpp_redis::redis_subscriber client;

  client.connect();
  EXPECT_NO_THROW(client.subscribe("/chan/*", [](const std::string&, const std::string&) {}));
}

TEST(RedisSubscriber, UnsubscribeConnected) {
  cpp_redis::redis_subscriber client;

  client.connect();
  EXPECT_NO_THROW(client.unsubscribe("/chan"));
}

TEST(RedisSubscriber, PUnsubscribeConnected) {
  cpp_redis::redis_subscriber client;

  client.connect();
  EXPECT_NO_THROW(client.punsubscribe("/chan/*"));
}

TEST(RedisSubscriber, SubscribeNotConnected) {
  cpp_redis::redis_subscriber client;

  EXPECT_NO_THROW(client.subscribe("/chan", [](const std::string&, const std::string&) {}));
}

TEST(RedisSubscriber, PSubscribeNotConnected) {
  cpp_redis::redis_subscriber client;

  EXPECT_NO_THROW(client.subscribe("/chan/*", [](const std::string&, const std::string&) {}));
}

TEST(RedisSubscriber, UnsubscribeNotConnected) {
  cpp_redis::redis_subscriber client;

  EXPECT_NO_THROW(client.unsubscribe("/chan"));
}

TEST(RedisSubscriber, PUnsubscribeNotConnected) {
  cpp_redis::redis_subscriber client;

  EXPECT_NO_THROW(client.punsubscribe("/chan/*"));
}

TEST(RedisSubscriber, SubConnectedCommitConnected) {
  cpp_redis::redis_subscriber sub;
  cpp_redis::redis_client client;
  std::condition_variable cv;

  sub.connect();
  client.connect();

  std::atomic_bool callback_run(false);
  sub.subscribe("/chan",
    [&](const std::string&, const std::string&) {
      callback_run = true;
      cv.notify_all();
    },
    [&](int64_t) {
      client.publish("/chan", "hello");
      client.commit();
    });

  sub.commit();

  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait_for(lock, std::chrono::seconds(10), [&]() -> bool { return callback_run; });

  EXPECT_TRUE(callback_run);
}

TEST(RedisSubscriber, SubNotConnectedCommitConnected) {
  cpp_redis::redis_subscriber sub;
  cpp_redis::redis_client client;
  std::condition_variable cv;

  client.connect();

  std::atomic_bool callback_run(false);
  sub.subscribe("/chan",
    [&](const std::string&, const std::string&) {
      callback_run = true;
      cv.notify_all();
    },
    [&](int64_t) {
      client.publish("/chan", "hello");
      client.commit();
    });

  sub.connect();
  sub.commit();

  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait_for(lock, std::chrono::seconds(10), [&]() -> bool { return callback_run; });

  EXPECT_TRUE(callback_run);
}

TEST(RedisSubscriber, SubNotConnectedCommitNotConnectedCommitConnected) {
  cpp_redis::redis_subscriber sub;
  cpp_redis::redis_client client;

  client.connect();

  std::atomic_bool callback_run(false);
  sub.subscribe("/chan",
    [&](const std::string&, const std::string&) {
      callback_run = true;
    },
    [&](int64_t) {
      client.publish("/chan", "hello");
      client.commit();
    });

  EXPECT_THROW(sub.commit(), cpp_redis::redis_error);
  sub.connect();
  sub.commit();

  std::this_thread::sleep_for(std::chrono::seconds(2));

  EXPECT_FALSE(callback_run);
}

TEST(RedisSubscriber, SubscribeSomethingPublished) {
  cpp_redis::redis_subscriber sub;
  cpp_redis::redis_client client;
  std::condition_variable cv;

  sub.connect();
  client.connect();

  std::atomic_bool callback_run(false);
  sub.subscribe("/chan",
    [&](const std::string& channel, const std::string& message) {
      EXPECT_TRUE(channel == "/chan");
      EXPECT_TRUE(message == "hello");
      callback_run = true;
      cv.notify_all();
    },
    [&](int64_t) {
      client.publish("/chan", "hello");
      client.commit();
    });

  sub.commit();

  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait_for(lock, std::chrono::seconds(10), [&]() -> bool { return callback_run; });

  EXPECT_TRUE(callback_run);
}

TEST(RedisSubscriber, SubscribeMultiplePublished) {
  cpp_redis::redis_subscriber sub;
  cpp_redis::redis_client client;
  std::condition_variable cv;

  sub.connect();
  client.connect();

  std::atomic_int number_times_called(0);
  sub.subscribe("/chan",
    [&](const std::string& channel, const std::string& message) {
      EXPECT_TRUE(channel == "/chan");
      if (++number_times_called == 1)
        EXPECT_TRUE(message == "first");
      else
        EXPECT_TRUE(message == "second");

      cv.notify_all();
    },
    [&](int64_t) {
      client.publish("/chan", "first");
      client.publish("/chan", "second");
      client.commit();
    });

  sub.commit();

  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait_for(lock, std::chrono::seconds(10), [&]() -> bool { return number_times_called == 2; });

  EXPECT_TRUE(number_times_called == 2);
}

TEST(RedisSubscriber, SubscribeNothingPublished) {
  cpp_redis::redis_subscriber sub;
  cpp_redis::redis_client client;

  sub.connect();
  client.connect();

  std::atomic_bool callback_run(false);
  sub.subscribe("/chan",
    [&](const std::string&, const std::string&) {
      callback_run = true;
    },
    [&](int64_t) {
      client.publish("/other_chan", "hello");
      client.commit();
    });

  sub.commit();

  std::this_thread::sleep_for(std::chrono::seconds(2));

  EXPECT_FALSE(callback_run);
}

TEST(RedisSubscriber, MultipleSubscribeSomethingPublished) {
  cpp_redis::redis_subscriber sub;
  cpp_redis::redis_client client;
  std::condition_variable cv;

  sub.connect();
  client.connect();

  auto ack_callback = [&](int64_t nb_chans) {
    if (nb_chans == 2) {
      client.publish("/chan_1", "hello");
      client.publish("/chan_2", "world");
      client.commit();
    }
  };

  std::atomic_bool callback_1_run(false);
  std::atomic_bool callback_2_run(false);
  sub.subscribe("/chan_1",
    [&](const std::string& channel, const std::string& message) {
      EXPECT_TRUE(channel == "/chan_1");
      EXPECT_TRUE(message == "hello");
      callback_1_run = true;

      if (callback_2_run)
        cv.notify_all();
    },
    ack_callback);
  sub.subscribe("/chan_2",
    [&](const std::string& channel, const std::string& message) {
      EXPECT_TRUE(channel == "/chan_2");
      EXPECT_TRUE(message == "world");
      callback_2_run = true;

      if (callback_1_run)
        cv.notify_all();
    },
    ack_callback);

  sub.commit();

  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait_for(lock, std::chrono::seconds(10), [&]() -> bool { return callback_1_run && callback_2_run; });

  EXPECT_TRUE(callback_1_run);
  EXPECT_TRUE(callback_2_run);
}

TEST(RedisSubscriber, PSubscribeSomethingPublished) {
  cpp_redis::redis_subscriber sub;
  cpp_redis::redis_client client;
  std::condition_variable cv;

  sub.connect();
  client.connect();

  std::atomic_bool callback_run(false);
  sub.psubscribe("/chan/*",
    [&](const std::string& channel, const std::string& message) {
      EXPECT_TRUE(channel == "/chan/hello");
      EXPECT_TRUE(message == "world");
      callback_run = true;
      cv.notify_all();
    },
    [&](int64_t) {
      client.publish("/chan/hello", "world");
      client.commit();
    });

  sub.commit();

  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait_for(lock, std::chrono::seconds(10), [&]() -> bool { return callback_run; });

  EXPECT_TRUE(callback_run);
}

TEST(RedisSubscriber, PSubscribeMultiplePublished) {
  cpp_redis::redis_subscriber sub;
  cpp_redis::redis_client client;
  std::condition_variable cv;

  sub.connect();
  client.connect();

  std::atomic_int number_times_called(0);
  sub.psubscribe("/chan/*",
    [&](const std::string& channel, const std::string& message) {
      ++number_times_called;

      if (number_times_called == 1)
        EXPECT_TRUE(channel == "/chan/hello");
      else
        EXPECT_TRUE(channel == "/chan/world");

      if (number_times_called == 1)
        EXPECT_TRUE(message == "first");
      else
        EXPECT_TRUE(message == "second");

      if (number_times_called == 2)
        cv.notify_all();
    },
    [&](int64_t) {
      client.publish("/chan/hello", "first");
      client.publish("/chan/world", "second");
      client.commit();
    });

  sub.commit();

  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait_for(lock, std::chrono::seconds(10), [&]() -> bool { return number_times_called == 2; });

  EXPECT_TRUE(number_times_called == 2);
}

TEST(RedisSubscriber, PSubscribeNothingPublished) {
  cpp_redis::redis_subscriber sub;
  cpp_redis::redis_client client;

  sub.connect();
  client.connect();

  std::atomic_bool callback_run(false);
  sub.psubscribe("/chan/*",
    [&](const std::string&, const std::string&) {
      callback_run = true;
    },
    [&](int64_t) {
      client.publish("/other_chan", "hello");
      client.commit();
    });

  sub.commit();

  std::this_thread::sleep_for(std::chrono::seconds(2));

  EXPECT_FALSE(callback_run);
}

TEST(RedisSubscriber, MultiplePSubscribeSomethingPublished) {
  cpp_redis::redis_subscriber sub;
  cpp_redis::redis_client client;
  std::condition_variable cv;

  sub.connect();
  client.connect();

  auto ack_callback = [&](int64_t nb_chans) {
    if (nb_chans == 2) {
      client.publish("/chan/1", "hello");
      client.publish("/other_chan/2", "world");
      client.commit();
    }
  };

  std::atomic_bool callback_1_run(false);
  std::atomic_bool callback_2_run(false);
  sub.psubscribe("/chan/*",
    [&](const std::string& channel, const std::string& message) {
      EXPECT_TRUE(channel == "/chan/1");
      EXPECT_TRUE(message == "hello");
      callback_1_run = true;

      if (callback_2_run)
        cv.notify_all();
    },
    ack_callback);
  sub.psubscribe("/other_chan/*",
    [&](const std::string& channel, const std::string& message) {
      EXPECT_TRUE(channel == "/other_chan/2");
      EXPECT_TRUE(message == "world");
      callback_2_run = true;

      if (callback_1_run)
        cv.notify_all();
    },
    ack_callback);

  sub.commit();

  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait_for(lock, std::chrono::seconds(10), [&]() -> bool { return callback_1_run && callback_2_run; });

  EXPECT_TRUE(callback_1_run);
  EXPECT_TRUE(callback_2_run);
}

TEST(RedisSubscriber, Unsubscribe) {
  cpp_redis::redis_subscriber sub;
  cpp_redis::redis_client client;
  std::condition_variable cv;

  sub.connect();
  client.connect();

  auto ack_callback = [&](int64_t nb_chans) {
    if (nb_chans == 2) {
      client.publish("/chan_1", "hello");
      client.publish("/chan_2", "hello");
      client.commit();
    }
  };

  std::atomic_bool callback_1_run(false);
  std::atomic_bool callback_2_run(false);
  sub.subscribe("/chan_1",
    [&](const std::string&, const std::string&) {
      callback_1_run = true;
    },
    ack_callback);
  sub.subscribe("/chan_2",
    [&](const std::string&, const std::string&) {
      callback_2_run = true;
      cv.notify_all();
    },
    ack_callback);
  sub.unsubscribe("/chan_1");

  sub.commit();

  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait_for(lock, std::chrono::seconds(10), [&]() -> bool { return callback_2_run; });

  EXPECT_FALSE(callback_1_run);
  EXPECT_TRUE(callback_2_run);
}

TEST(RedisSubscriber, PUnsubscribe) {
  cpp_redis::redis_subscriber sub;
  cpp_redis::redis_client client;
  std::condition_variable cv;

  sub.connect();
  client.connect();

  auto ack_callback = [&](int64_t nb_chans) {
    if (nb_chans == 2) {
      client.publish("/chan_1/hello", "hello");
      client.publish("/chan_2/hello", "hello");
      client.commit();
    }
  };

  std::atomic_bool callback_1_run(false);
  std::atomic_bool callback_2_run(false);
  sub.psubscribe("/chan_1/*",
    [&](const std::string&, const std::string&) {
      callback_1_run = true;
    },
    ack_callback);
  sub.psubscribe("/chan_2/*",
    [&](const std::string&, const std::string&) {
      callback_2_run = true;
      cv.notify_all();
    },
    ack_callback);
  sub.punsubscribe("/chan_1/*");

  sub.commit();

  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait_for(lock, std::chrono::seconds(10), [&]() -> bool { return callback_2_run; });

  EXPECT_FALSE(callback_1_run);
  EXPECT_TRUE(callback_2_run);
}
