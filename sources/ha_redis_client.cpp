// The MIT License (MIT)
//
// Copyright (c) 2015-2017 Simon Ninon <simon.ninon@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cpp_redis/ha_redis_client.hpp>
#include <cpp_redis/redis_error.hpp>

namespace cpp_redis {

#ifdef __CPP_REDIS_USE_CUSTOM_TCP_CLIENT
   ha_redis_client::ha_redis_client(std::int32_t max_reconnects, std::uint32_t reconnect_interval_msecs, 
                                    const std::shared_ptr<network::tcp_client_iface>& tcp_client,
                                    ha_connect_callback_t connect_callback)
      : redis_client(tcp_client), m_sentinel(tcp_client), m_connect_callback(connect_callback) {
#else
   ha_redis_client::ha_redis_client(std::int32_t max_reconnects, std::uint32_t reconnect_interval_msecs,
                                    std::uint32_t num_io_workers,
                                    ha_connect_callback_t connect_callback)
      : redis_client(num_io_workers), m_sentinel(num_io_workers), m_connect_callback(connect_callback) {
#endif
      m_max_reconnects = max_reconnects;
      m_reconnect_interval_msecs = reconnect_interval_msecs;
      m_connect_timeout_msecs = 0;
      m_cancel = false;
      m_redis_port = 0;
   }

   ha_redis_client::~ha_redis_client(void) {
      cancel_reconnect();

      //If for some reason sentinel is connected then disconnect now.
      if(m_sentinel.is_connected())
         m_sentinel.disconnect(true);
   }

   void 
   ha_redis_client::add_sentinel(const std::string& host, std::size_t port) {
      m_sentinel.add_sentinel(host, port);
   }

   void 
   ha_redis_client::clear_sentinels() {
      m_sentinel.clear_sentinels();
   }

   //! override to capture the auth password to use in reconnects
   redis_client& 
   ha_redis_client::auth(const std::string& password, const reply_callback_t& reply_callback) {
      m_password = password;  //save the password for reconnect attempts.
      return redis_client::auth(password, reply_callback); //call our parent
   }

   //! override to capture the database index to use in reconnects
   redis_client& 
   ha_redis_client::select(int index, const reply_callback_t& reply_callback) { 
      m_database_index = index;  //save the index of the database for reconnect attempts.
      return redis_client::select(index, reply_callback); //call our parent
   }
   
   void 
   ha_redis_client::connect(std::uint32_t timeout_msecs, const std::string& name) {
      //Save for auto reconnects
      m_master_name = name;
      m_connect_timeout_msecs = timeout_msecs;
      
      //We rely on the sentinel to tell us which redis server is currently the master.
      if(m_sentinel.get_master_addr_by_name(name, m_redis_server, m_redis_port, true)) {
         ha_redis_client::internal_connect(m_redis_server, m_redis_port, timeout_msecs);
      }
      else {
         throw redis_error("ha_redis_client::connect() could not find master for name " + name);
      }
   }

   void
   ha_redis_client::disconnect_handler(network::redis_connection&) {

      __CPP_REDIS_LOG(debug, "ha_redis_client has been disconnected. attempting reconnnect in 1/2 second.");
      if(m_connect_callback)
         m_connect_callback(m_redis_server, m_redis_port, ha_client_connect_dropped);
      
      m_reconnecting = true;
      
      //We wait for 1/2 second to give the redis server time to recover
      //and also time for sentinels to promote another redis server to master
      //before we attempt to connect to the new master
      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      //Lock the callbacks mutex of the base class to prevent more client commands from being issued
      //until our reconnect has completed.
      std::unique_lock<std::mutex> lock_callback(m_callbacks_mutex);

      //Make a copy of any waiting commands and their associated callbacks so that we
      //can re-issue the commands once the reconnect is complete.
      //Hopefully the caller will never know the redis server has been switched out from under them.
      std::queue<std::vector<std::string>> pending_commands = std::move(m_commands);
      std::queue<reply_callback_t> pending_callbacks = std::move(m_callbacks);

      //Now reconnect automatically to a new master redis server
      bool forever=false;
      std::uint32_t attempts=0;
      std::int32_t reconnects = m_max_reconnects;
      if(-1 == reconnects)
         forever=true;

      while(!m_cancel && (forever || reconnects>0))
      {
         if(!forever)
            --reconnects;
         ++attempts;

         //We rely on the sentinel to tell use which redis server is currently the master.
         if(m_sentinel.get_master_addr_by_name(m_master_name, m_redis_server, m_redis_port, true)) {
            __CPP_REDIS_LOG(warn, std::string("ha_redis_client reconnect attempt #" + std::to_string(attempts) + " to master " + m_redis_server).c_str());

            //Try catch block because the redis client throws an error if connection cannot be made.
            try {
               internal_connect(m_redis_server, m_redis_port, m_connect_timeout_msecs);
            }
            catch(...){}

            if(is_connected())
            {
               __CPP_REDIS_LOG(warn, "ha_redis_client reconnected ok");

               //Now re-authorize and select the proper database.
               if(m_password.size() > 0) {
                  //Unlock the lock because it cannot be reentered when sending commands as part of auth()
                  lock_callback.unlock();

                  redis_client::auth(m_password, [&](cpp_redis::reply& reply) {
                     if(reply.is_string() && reply.as_string() == "OK") {
                        __CPP_REDIS_LOG(warn, "ha_redis_client reconnect authenticated ok");
                     }
                     else {
                        __CPP_REDIS_LOG(warn, std::string("ha_redis_client reconnect failed to authenticate. Redis says: " + reply.as_string()).c_str());
                     }
                  });
                  sync_commit(std::chrono::seconds(10)); //Should never take this long but...
               }

               if(m_database_index > 0) {
                  //Unlock the lock because it cannot be reentered when sending commands as part of select()
                  if(lock_callback.owns_lock())
                     lock_callback.unlock();

                  redis_client::select(m_database_index, [&](cpp_redis::reply& reply) {
                     if(reply.is_string() && reply.as_string() == "OK") {
                        __CPP_REDIS_LOG(warn, "ha_redis_client reconnect selected redis database ok");
                     }
                     else {
                        __CPP_REDIS_LOG(warn, std::string("ha_redis_client reconnect failed to select database. Redis says: " + reply.as_string()).c_str());
                     }
                  });
                  sync_commit(std::chrono::seconds(10)); //Should never take this long but...
               }
 
               break;   //Break out of loop. We are good.
            }
            else
            {
               if(m_connect_callback)
                  m_connect_callback(m_redis_server, m_redis_port, ha_client_connect_failed);
            }
         }
         else {
               if(m_connect_callback)
                  m_connect_callback(m_redis_server, m_redis_port, ha_client_connect_lookup_failed);
         }

         if(m_reconnect_interval_msecs>0) {
            if(m_connect_callback)
               m_connect_callback(m_redis_server, m_redis_port, ha_client_connect_sleeping);

            __CPP_REDIS_LOG(warn, "ha_redis_client reconnect failed. sleeping before next attempt");
            std::this_thread::sleep_for(std::chrono::milliseconds(m_reconnect_interval_msecs));
         }
      }

      if(is_connected()) {
          reply_callback_t callback = nullptr;

         //Unlock the lock because it cannot be reentered when sending commands below
         if(lock_callback.owns_lock())
            lock_callback.unlock();

         bool sent=false;
         //If we got reconnected re-issue any pending commands and callbacks.
         while(pending_commands.size() > 0) {
            callback = pending_callbacks.front();
            std::vector<std::string>& commands = pending_commands.front();
            pending_callbacks.pop();
            pending_commands.pop();
            send(commands, callback);  //Reissue the pending command and its callback.
            sent = true;
         }
         if(sent)
            commit();   //Commit the work we just did.
         m_reconnecting = false;
         return;
      }

      //We could not get connected!
      m_reconnecting = false;
      m_callbacks_running = 0;

      //Tell the user we gave up!
      if(m_connect_callback)
         m_connect_callback(m_redis_server, m_redis_port, ha_client_connect_stopped);
   }

   void 
   ha_redis_client::cancel_reconnect() {
      m_cancel=true;
   }

   //Override so we can use our receive handler instead of base class.
   void
   ha_redis_client::internal_connect(const std::string& host, std::size_t port,
                                     std::uint32_t timeout_msecs) {
      if(m_connect_callback)
         m_connect_callback(host, port, ha_client_connect_start);

      __CPP_REDIS_LOG(debug, "cpp_redis::ha_redis_client attempts to connect");

      auto our_disconnect_handler = std::bind(&ha_redis_client::disconnect_handler, this, std::placeholders::_1);
      auto our_receive_handler = std::bind(&ha_redis_client::receive_handler, this, std::placeholders::_1, std::placeholders::_2);
      m_client.connect(host, port, our_disconnect_handler, our_receive_handler, timeout_msecs);

      if(m_connect_callback)
         m_connect_callback(host, port, ha_client_connect_ok);
      __CPP_REDIS_LOG(info, "cpp_redis::ha_redis_client connected");
   }

   redis_client& 
   ha_redis_client::send(const std::vector<std::string>& redis_cmd, const reply_callback_t& callback) {
      //Do the same thing as our base class except we keep the commands in a queue
      //to enable us to replay any that were missed when our connection to redis drops out
      //and we need to reconnect.
      std::lock_guard<std::mutex> lock_callback(m_callbacks_mutex);

      __CPP_REDIS_LOG(info, "cpp_redis::ha_redis_client attemps to store new command in the send buffer");
      m_client.send(redis_cmd);
      m_commands.push(redis_cmd);
      m_callbacks.push(callback);
      __CPP_REDIS_LOG(info, "cpp_redis::ha_redis_client stored new command in the send buffer");

      return *this;
   }

   //Override the base class receive handler so we can maintain our queue of commands that were issued.
   void
   ha_redis_client::receive_handler(network::redis_connection&, reply& reply) {
      reply_callback_t callback = nullptr;

      __CPP_REDIS_LOG(info, "cpp_redis::ha_redis_client received reply");
      {
         std::lock_guard<std::mutex> lock(m_callbacks_mutex);

         if(m_callbacks.size()) {
            callback = m_callbacks.front();
            m_callbacks_running += 1;
            m_commands.pop();
            m_callbacks.pop();
         }
      }

      if(m_callback_runner) {
         __CPP_REDIS_LOG(debug, "cpp_redis::ha_redis_client executes reply callback through custom before callback handler");
         m_callback_runner(reply, callback);
      }
      else if(callback) {
         __CPP_REDIS_LOG(debug, "cpp_redis::ha_redis_client executes reply callback");
         callback(reply);
      }

      {
         std::lock_guard<std::mutex> lock(m_callbacks_mutex);
         m_callbacks_running -= 1;
         m_sync_condvar.notify_all();
      }
   }


} //! cpp_redis
