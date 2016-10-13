#include <cpp_redis/redis_client.hpp>
#include <gtest/gtest.h>

TEST(RedisClient, ValidConnectionDefaultParams) {
  cpp_redis::redis_client client;

  EXPECT_FALSE(client.is_connected());
  //! should connect to 127.0.0.1:6379
  EXPECT_NO_THROW(client.connect());
  EXPECT_TRUE(client.is_connected());
}

TEST(RedisClient, ValidConnectionDefinedHost) {
  cpp_redis::redis_client client;

  EXPECT_FALSE(client.is_connected());
  //! should connect to 127.0.0.1:6379
  EXPECT_NO_THROW(client.connect("127.0.0.1", 6379));
  EXPECT_TRUE(client.is_connected());
}

TEST(RedisClient, InvalidConnection) {
  cpp_redis::redis_client client;

  EXPECT_FALSE(client.is_connected());
  EXPECT_THROW(client.connect("invalid.url", 1234), cpp_redis::redis_error);
  EXPECT_FALSE(client.is_connected());
}

TEST(RedisClient, AlreadyConnected) {
  cpp_redis::redis_client client;

  EXPECT_FALSE(client.is_connected());
  //! should connect to 127.0.0.1:6379
  EXPECT_NO_THROW(client.connect());
  EXPECT_TRUE(client.is_connected());
  EXPECT_THROW(client.connect(), cpp_redis::redis_error);
  EXPECT_TRUE(client.is_connected());
}

TEST(RedisClient, Disconnection) {
  cpp_redis::redis_client client;

  client.connect();
  EXPECT_TRUE(client.is_connected());
  client.disconnect();
  EXPECT_FALSE(client.is_connected());
}

TEST(RedisClient, DisconnectionNotConnected) {
  cpp_redis::redis_client client;

  EXPECT_FALSE(client.is_connected());
  EXPECT_NO_THROW(client.disconnect());
  EXPECT_FALSE(client.is_connected());
}

TEST(RedisClient, CommitConnected) {
  cpp_redis::redis_client client;

  client.connect();
  EXPECT_NO_THROW(client.commit());
}

TEST(RedisClient, CommitNotConnected) {
  cpp_redis::redis_client client;

  EXPECT_THROW(client.commit(), cpp_redis::redis_error);
}

TEST(RedisClient, SyncCommitConnected) {
  cpp_redis::redis_client client;

  client.connect();
  EXPECT_NO_THROW(client.sync_commit());
}

TEST(RedisClient, SyncCommitNotConnected) {
  cpp_redis::redis_client client;

  EXPECT_THROW(client.sync_commit(), cpp_redis::redis_error);
}

TEST(RedisClient, SyncCommitTimeoutConnected) {
  cpp_redis::redis_client client;

  client.connect();
  EXPECT_NO_THROW(client.sync_commit(std::chrono::milliseconds(100)));
}

TEST(RedisClient, SyncCommitTimeoutNotConnected) {
  cpp_redis::redis_client client;

  EXPECT_THROW(client.sync_commit(std::chrono::milliseconds(100)), cpp_redis::redis_error);
}

TEST(RedisClient, SyncCommitTimeout) {
  cpp_redis::redis_client client;

  client.connect();
  volatile std::atomic_bool callback_exit(false);
  client.send({"GET", "HELLO"}, [&](cpp_redis::reply&) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    callback_exit = true;
  });
  EXPECT_NO_THROW(client.sync_commit(std::chrono::milliseconds(100)));
  EXPECT_FALSE(callback_exit);
  while (!callback_exit)
    ;
}

TEST(RedisClient, SyncCommitNoTimeout) {
  cpp_redis::redis_client client;

  client.connect();
  std::atomic_bool callback_exit(false);
  client.send({"GET", "HELLO"}, [&](cpp_redis::reply&) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    callback_exit = true;
  });
  EXPECT_NO_THROW(client.sync_commit());
  EXPECT_TRUE(callback_exit);
}

TEST(RedisClient, SendConnected) {
  cpp_redis::redis_client client;

  client.connect();
  EXPECT_NO_THROW(client.send({"GET", "HELLO"}));
}

TEST(RedisClient, SendNotConnected) {
  cpp_redis::redis_client client;

  EXPECT_NO_THROW(client.send({"GET", "HELLO"}));
}

TEST(RedisClient, SendConnectedSyncCommitConnected) {
  cpp_redis::redis_client client;

  client.connect();

  std::atomic_bool callback_run(false);
  client.send({"GET", "HELLO"}, [&](cpp_redis::reply&) {
    callback_run = true;
  });

  client.sync_commit();
  EXPECT_TRUE(callback_run);
}

TEST(RedisClient, SendNotConnectedSyncCommitConnected) {
  cpp_redis::redis_client client;

  std::atomic_bool callback_run(false);
  client.send({"GET", "HELLO"}, [&](cpp_redis::reply&) {
    callback_run = true;
  });

  client.connect();
  client.sync_commit();
  EXPECT_TRUE(callback_run);
}

TEST(RedisClient, SendNotConnectedSyncCommitNotConnectedSyncCommitConnected) {
  cpp_redis::redis_client client;

  std::atomic_bool callback_run(false);
  client.send({"GET", "HELLO"}, [&](cpp_redis::reply&) {
    callback_run = true;
  });

  EXPECT_THROW(client.sync_commit(), cpp_redis::redis_error);
  client.connect();
  client.sync_commit();
  //! should have cleared commands in the buffer
  EXPECT_FALSE(callback_run);
}

TEST(RedisClient, Send) {
  cpp_redis::redis_client client;

  client.connect();
  client.send({"PING"}, [&](cpp_redis::reply& reply) {
    EXPECT_TRUE(reply.is_string());
    EXPECT_TRUE(reply.as_string() == "PONG");
  });
  client.sync_commit();
}

TEST(RedisClient, MultipleSend) {
  cpp_redis::redis_client client;

  client.connect();

  client.send({"PING"}, [&](cpp_redis::reply& reply) {
    EXPECT_TRUE(reply.is_string());
    EXPECT_TRUE(reply.as_string() == "PONG");
  });
  client.sync_commit();

  client.send({"SET", "HELLO", "MultipleSend"}, [&](cpp_redis::reply& reply) {
    EXPECT_TRUE(reply.is_string());
    EXPECT_TRUE(reply.as_string() == "OK");
  });
  client.sync_commit();

  client.send({"GET", "HELLO"}, [&](cpp_redis::reply& reply) {
    EXPECT_TRUE(reply.is_string());
    EXPECT_TRUE(reply.as_string() == "MultipleSend");
  });
  client.sync_commit();
}

TEST(RedisClient, MultipleSendPipeline) {
  cpp_redis::redis_client client;

  client.connect();

  client.send({"PING"}, [&](cpp_redis::reply& reply) {
    EXPECT_TRUE(reply.is_string());
    EXPECT_TRUE(reply.as_string() == "PONG");
  });
  client.send({"SET", "HELLO", "MultipleSendPipeline"}, [&](cpp_redis::reply& reply) {
    EXPECT_TRUE(reply.is_string());
    EXPECT_TRUE(reply.as_string() == "OK");
  });
  client.send({"GET", "HELLO"}, [&](cpp_redis::reply& reply) {
    EXPECT_TRUE(reply.is_string());
    EXPECT_TRUE(reply.as_string() == "MultipleSendPipeline");
  });
  client.sync_commit();
}

TEST(RedisClient, DisconnectionHandlerWithQuit) {
  cpp_redis::redis_client client;
  std::condition_variable cv;

  std::atomic_bool disconnection_handler_called(false);
  client.connect("127.0.0.1", 6379, [&](cpp_redis::redis_client&) {
    disconnection_handler_called = true;
    cv.notify_all();
  });

  client.send({"QUIT"});
  client.sync_commit();

  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait_for(lock, std::chrono::seconds(2));

  EXPECT_TRUE(disconnection_handler_called);
}

TEST(RedisClient, DisconnectionHandlerWithoutQuit) {
  cpp_redis::redis_client client;
  std::condition_variable cv;

  std::atomic_bool disconnection_handler_called(false);
  client.connect("127.0.0.1", 6379, [&](cpp_redis::redis_client&) {
    disconnection_handler_called = true;
    cv.notify_all();
  });

  client.sync_commit();

  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait_for(lock, std::chrono::seconds(2));

  EXPECT_FALSE(disconnection_handler_called);
}

TEST(RedisClient, ClearBufferOnError) {
  cpp_redis::redis_client client;

  client.connect();
  client.send({"SET", "HELLO", "BEFORE"});
  client.sync_commit();
  client.disconnect();

  client.send({"SET", "HELLO", "AFTER"});
  EXPECT_THROW(client.sync_commit(), cpp_redis::redis_error);
  client.connect();
  client.send({"GET", "HELLO"}, [&](cpp_redis::reply& reply) {
    EXPECT_TRUE(reply.is_string());
    EXPECT_TRUE(reply.as_string() == "BEFORE");
  });
  client.sync_commit();
}
