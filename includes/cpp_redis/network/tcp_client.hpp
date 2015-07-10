#pragma once

#include <mutex>
#include <thread>
#include <atomic>
#include <stdexcept>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

namespace cpp_redis {

namespace network {

//! tcp_client
//! async tcp client based on boost asio
class tcp_client {
public:
    //! ctor & dtor
    tcp_client(void);
    ~tcp_client(void);

    //! assignment operator & copy ctor
    tcp_client(const tcp_client&) = delete;
    tcp_client& operator=(const tcp_client&) = delete;

    //! returns whether the client is connected or not
    bool is_connected(void);

    //! handle connection & disconnection
    bool connect(const std::string& host, unsigned int port);
    void disconnect(void);

    //! send data
    void send(const std::string& buffer);
    void send(const std::vector<char>& buffer);

    //! set receive callback
    //! called each time some data has been received
    typedef std::function<void(tcp_client&, const std::vector<char>& buffer)> receive_handler;
    void set_receive_handler(const receive_handler& handler);

    //! set disconnection callback
    //! called each time a disconnection has been detected
    typedef std::function<void(tcp_client&)> disconnection_handler;
    void set_disconnection_handler(const disconnection_handler& handler);

public:
    //! exception handling
    class tcp_client_error : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
        using std::runtime_error::what;
    };

private:
    //! make boost asio async read and write operations
    void async_read(void);
    void async_write(void);

    //! close socket and call disconnect callback
    //! called in case of error
    void process_disconnection(void);

private:
    //! io service
    static boost::asio::io_service m_io_service;
    std::thread m_io_thread;
    tcp::socket m_socket;

    //! is connected
    std::atomic_bool m_is_connected;

    //! buffers
    static const unsigned int READ_SIZE = 2048;
    std::vector<char> m_read_buffer;
    std::vector<char> m_write_buffer;

    //! handlers
    receive_handler m_receive_handler;
    disconnection_handler m_disconnection_handler;

    //! thread safety
    std::mutex m_handler_mutex;
    std::mutex m_write_buffer_mutex;
    std::mutex m_receive_handler_mutex;
    std::mutex m_disconnection_handler_mutex;
};

} //! network

} //! cpp_redis
