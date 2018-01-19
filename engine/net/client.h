#ifndef ENGINE_NET_CLIENT_H
#define ENGINE_NET_CLIENT_H

#include <third_party/asio.hpp>
#include <third_party/g3log/g3log/g3log.hpp>
#include <engine/common/common.h>
#include <engine/net/io_service_pool.h>
#include <engine/net/session.h>

namespace engine
{

using asio::ip::tcp;

class client
{
public:
    client(const client&) = delete;
    client& operator=(const client&) = delete;
    client(const char* address, unsigned short port)
        : io_service_client_pool_(1, "client_pool")
        , address_(address)
        , port_(port)
        , read_high_water_mask_(0)
        , write_high_water_mask_(0)
        , write_high_water_mask_handler_(nullptr)
        , init_handlers_(nullptr)
    {
        session_ = std::make_shared<session>(get_session_increase_id(),
                io_service_client_pool_.get_io_service(),
                io_service_client_pool_.get_io_service());
    
        connect();    
    }

    ~client()
    {
    }

    void run()
    {
        io_service_client_pool_.run();
    }

    void stop()
    {
        io_service_client_pool_.stop();
    }

    void set_read_high_water_mask(std::size_t mask)
    {
        read_high_water_mask_ = mask;
    }

    void set_write_high_water_mask_handler(
            const std::function<void(std::shared_ptr<session>, std::size_t)>& handler, 
            std::size_t mask)
    {
        write_high_water_mask_          = mask;
        write_high_water_mask_handler_  = handler;
    }

    void set_init_handlers(const std::function<void(std::shared_ptr<session>)>& handler)
    {
        init_handlers_ = handler;
    }

    std::shared_ptr<session> get_session()
    {
        return session_;
    }

private:
    void connect()
    {
        tcp::resolver resolver(io_service_client_pool_.get_io_service());
        tcp::resolver::query query(address_, std::to_string(port_));

        std::error_code ec;
        tcp::resolver::iterator iter = resolver.resolve(query, ec);
        tcp::resolver::iterator end;

        if (iter != end && !ec) {
            session_->socket().async_connect(*iter, 
                    [=](std::error_code ec){handle_connect(ec);});
        } else {
            LOGF(WARNING, "client connect address = %s, port = %d, error = %s", 
                    address_, port_, ec.message().c_str());
        }
    }

    void handle_connect(std::error_code& ec)
    {
        if (!ec) {
            session_->socket().set_option(asio::socket_base::debug(true));
            session_->socket().set_option(asio::socket_base::enable_connection_aborted(true));
            session_->socket().set_option(asio::socket_base::linger(true, 30));
            session_->socket().set_option(tcp::no_delay(true));
            if (read_high_water_mask_ != 0) {
                session_->set_read_high_water_mask(read_high_water_mask_);
            }
            if (write_high_water_mask_ != 0 && write_high_water_mask_handler_ != nullptr) {
                session_->set_write_high_water_mask_handler(
                    write_high_water_mask_handler_, write_high_water_mask_);
            }
            session_->start(init_handlers_);
        } else {
            LOGF(WARNING, "client handle connect address = %s, port = %d, error = %s", 
                    address_, port_, ec.message().c_str());
        }
    }
    
    io_service_pool                                 io_service_client_pool_;
    const char*                                     address_;
    unsigned short                                  port_;
    std::size_t                                     read_high_water_mask_;
    std::size_t                                     write_high_water_mask_;
    std::function<void(std::shared_ptr<session>, std::size_t)>
                                                    write_high_water_mask_handler_;
    std::function<void(std::shared_ptr<session>)>   init_handlers_;
    std::shared_ptr<session>                        session_;
}; // class client

} // namespace engine

#endif // ENGINE_NET_CLIENT_H
