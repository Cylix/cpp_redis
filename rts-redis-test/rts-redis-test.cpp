// rts-redis-test.cpp : Defines the entry point for the console application.
//
#include "cpp_redis/redis_client.hpp"
#include "cpp_redis/redis_subscriber.hpp"
#include "cpp_redis/network/win_io_service.hpp"
#include <cassert>

#include <signal.h>
#include <iostream>

cpp_redis::redis_client* client;
volatile std::atomic_bool should_exit(false);

void
sigint_handler(int) {
  std::cout << "disconnected (sigint handler)" << std::endl;
  if(client)
    client->disconnect();
  should_exit = true;
}

int
main(void) {

  std::shared_ptr<cpp_redis::network::io_service> m_pIOService = std::shared_ptr<cpp_redis::network::io_service>(new cpp_redis::network::io_service(1));

  //client = new cpp_redis::redis_client(m_pIOService);

  cpp_redis::redis_subscriber sub(m_pIOService);

  sub.connect("127.0.0.1", 6379, [](cpp_redis::redis_subscriber&) {
    printf("sub disconnected (disconnection handler)\n");
    should_exit = true;
  });

  sub.subscribe("some_chan", [](const std::string& chan, const std::string& msg) {
    // a read operation has completed
    printf("MESSAGE chan %s: %s\n", chan.c_str(), msg.c_str());
  });
  sub.psubscribe("*", [](const std::string& chan, const std::string& msg) {
    printf("PMESSAGE chan %s: %s\n", chan.c_str(), msg.c_str());
  });
  sub.commit();

  /*
  cpp_redis::redis_subscriber sub(m_pIOService);
  //cpp_redis::redis_client client;

  client->connect();

  std::atomic_bool callback_run(false);
  sub.subscribe("/chan", [&](const std::string&, const std::string&) 
  {
    callback_run = true;
  });

  sub.connect();
  sub.commit();
  client->publish("/chan", "hello");
  client->commit();
  Sleep(120000);
  //std::this_thread::sleep_for(std::chrono::seconds(1));
  assert(callback_run);
  */

  /*
  std::atomic_bool disconnection_handler_called(false);

  client->connect("127.0.0.1", 6379, [&](cpp_redis::redis_client&) 
    {
      disconnection_handler_called = true;
    }
  );

  client->send({ "QUIT" });
  client->sync_commit();
  std::this_thread::sleep_for(std::chrono::seconds(5));
  assert(true == disconnection_handler_called);
  */


  /*
  client->send({ "SET", "HELLO", "MultipleSend" }, 
    [&](cpp_redis::reply& reply)
    {
      if (!reply.is_string())
        printf("Reply is NOT a string!\n");
      if (!(reply.as_string() == "OK"))
        printf("Reply expected OK!\n");
      else
        printf("Reply received OK");
    }
  );
  client->sync_commit();

  client->send({ "GET", "HELLO" }, 
    [&](cpp_redis::reply& reply)
    {
      if (!reply.is_string())
        printf("Reply is NOT a string!\n");
      if (!(reply.as_string() == "MultipleSend"))
        printf("Reply expected MultipleSend!\n");
      else
        printf("Reply received MultipleSend\n");
    }
  );
  client->sync_commit();
  */

  signal(SIGINT, &sigint_handler);
  while (!should_exit);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  client->disconnect();

  m_pIOService->shutdown();
  m_pIOService.reset();

  return 0;
}
