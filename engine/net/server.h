#ifndef ENGINE_NET_SERVER_H
#define ENGINE_NET_SERVER_H

#include <map>
#include <third_party/asio.hpp>
#include <engine/net/io_service_pool.h>
#include <engine/net/session.h>

namespace engine
{

using asio::ip::tcp;

class server
{
public:
    server(const server&) = delete;
    server& operator=(const server&) = delete;
    explicit server(const char* address, unsigned short port, std::size_t io_service_pool_size)
        : io_service_accept_pool_(1, "accept_pool")
        , io_service_pool_(io_service_pool_size, "io_pool")
        , io_service_work_pool_(io_service_pool_size, "work_pool")
        , acceptor_(io_service_accept_pool_.get_io_service(), 
                tcp::endpoint(asio::ip::address_v4::from_string(address), port),
                true)
    {
        acceptor_.set_option(asio::socket_base::debug(true));
        acceptor_.set_option(asio::socket_base::enable_connection_aborted(true));
        acceptor_.set_option(asio::socket_base::linger(true, 30));
        acceptor_.set_option(tcp::no_delay(true));

        accept();        
    }

    void run()
    {
        io_service_accept_pool_.run();
        io_service_pool_.run();
        io_service_work_pool_.run();
    }

    void stop()
    {
        io_service_accept_pool_.stop();
        io_service_pool_.stop();
        io_service_work_pool_.stop();
    }

    void set_init_handlers(std::function<void(std::shared_ptr<session>)> init_handlers)
    {
        init_handlers_ = init_handlers;
    }
private:
    void accept()
    {
        session_ptr new_session(new session(io_service_work_pool_.get_io_service(),
                io_service_pool_.get_io_service()));
        acceptor_.async_accept(new_session->socket(),
                [=](std::error_code ec){handle_accept(new_session, ec);});
    }

    void handle_accept(session_ptr session, std::error_code& ec)
    {
        if (!ec) {
            session->start(init_handlers_);
        } else {
            std::cout << "accept error: " << ec.message() << std::endl;
        }

        accept();
    }

private:
    typedef std::shared_ptr<std::thread> thread_ptr;

    thread_ptr          accept_thread_;
    thread_ptr          io_thread_;
    thread_ptr          work_thread_;
    io_service_pool     io_service_accept_pool_;
    io_service_pool     io_service_pool_;
    io_service_pool     io_service_work_pool_;
    tcp::acceptor       acceptor_;
    std::function<void(std::shared_ptr<session>)>   init_handlers_;
}; // class server

} // namespace engine

#endif // ENGINE_NET_SERVER_H

