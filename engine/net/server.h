#ifndef ENGINE_NET_SERVER_H
#define ENGINE_NET_SERVER_H

#include <map>
#include <third_party/asio.hpp>
#include <third_party/asio/steady_timer.hpp>
#include <third_party/g3log/g3log/g3log.hpp>
#include <engine/common/common.h>
#include <engine/common/wfirst_rw_lock.h>
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
    server(const char* address, unsigned short port, std::size_t io_service_pool_size)
        : io_service_accept_pool_(1, "accept_pool")
        , io_service_pool_(io_service_pool_size, "io_pool")
        , io_service_work_pool_(io_service_pool_size, "work_pool")
        , acceptor_(io_service_accept_pool_.get_io_service(), 
                tcp::endpoint(asio::ip::address_v4::from_string(address), port),
                true)
        , read_high_water_mask_(0)
        , write_high_water_mask_(0)
        , write_high_water_mask_handler_(nullptr)
        , init_handlers_(nullptr)
        , timer_(io_service_accept_pool_.get_io_service())
    {
        acceptor_.set_option(asio::socket_base::debug(true));
        acceptor_.set_option(asio::socket_base::enable_connection_aborted(true));
        acceptor_.set_option(asio::socket_base::linger(true, 30));
        acceptor_.set_option(tcp::no_delay(true));

        accept();
        check_idle();        
    }

    ~server()
    {
        session_map_.clear();
        wait_remove_session_map_.clear();
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

    void close_session(uint32_t session_id)
    {
        auto write_guard = lock_.write_guard();
        if (!remove_session(wait_remove_session_map_, session_id)) {
            remove_session(session_map_, session_id);
        }
    }

    static std::shared_ptr<session> remove_session(
            std::map<uint32_t, std::shared_ptr<session>>& map, 
            uint32_t id)
    {
        std::shared_ptr<session> result = nullptr;
        for (auto it = map.begin(); it != map.end();) {
            if (it->first == id) {
                result = it->second;
                it = map.erase(it);
                break;
            }
            else {
                it++;
            }
        }
        return result;
    }
private:
    void accept()
    {
        std::shared_ptr<session> new_session(new session(get_session_increase_id(),
                io_service_work_pool_.get_io_service(),
                io_service_pool_.get_io_service()));
        acceptor_.async_accept(new_session->socket(),
                [=](std::error_code ec){handle_accept(new_session, ec);});
    }

    void handle_accept(std::shared_ptr<session> session, std::error_code& ec)
    {
        if (!ec) {
            if (read_high_water_mask_ != 0) {
                session->set_read_high_water_mask(read_high_water_mask_);
            }
            if (write_high_water_mask_ != 0 && write_high_water_mask_handler_ != nullptr) {
                session->set_write_high_water_mask_handler(
                        write_high_water_mask_handler_, write_high_water_mask_);
            }
            session->set_close_handler([this](uint32_t session_id){close_session(session_id);});
            {
                auto write_guard = lock_.write_guard();
                session_map_[session->id()] = session;
            }
            session->start(init_handlers_);
        } else {
            LOGF(FATAL, "accept error = %s", ec.message().c_str());
        }

        accept();
    }

    void check_idle()
    {
        timer_.expires_from_now(std::chrono::seconds(30));
        timer_.async_wait([=](std::error_code ec){handle_check_idle(ec);});
    }

    void handle_check_idle(std::error_code& ec)
    {
        if (!ec) {
            auto write_guard = lock_.write_guard();
            for (auto it = session_map_.begin(); it != session_map_.end();) {
                if (it->second->check_idle()) {
                    wait_remove_session_map_[it->first] = it->second;
                    LOGF(INFO, "check idle session id = %d", it->second->id());
                    it->second->close();
                    it = session_map_.erase(it); 
                }
                else {
                    it++;
                }
            }
        } else {
            LOGF(FATAL, "check idle error = %s", ec.message().c_str());
        }

        check_idle();
    }
private:
    typedef std::shared_ptr<std::thread>            thread_ptr;

    thread_ptr                                      accept_thread_;
    thread_ptr                                      io_thread_;
    thread_ptr                                      work_thread_;
    io_service_pool                                 io_service_accept_pool_;
    io_service_pool                                 io_service_pool_;
    io_service_pool                                 io_service_work_pool_;
    tcp::acceptor                                   acceptor_;
    std::size_t                                     read_high_water_mask_;
    std::size_t                                     write_high_water_mask_;
    std::function<void(std::shared_ptr<session>, std::size_t)>   
                                                    write_high_water_mask_handler_;
    std::function<void(std::shared_ptr<session>)>   init_handlers_;
    std::map<uint32_t, std::shared_ptr<session>>    session_map_;
    std::map<uint32_t, std::shared_ptr<session>>    wait_remove_session_map_;
    wfirst_rw_lock                                  lock_;
    asio::steady_timer                              timer_;
}; // class server

} // namespace engine
#endif // ENGINE_NET_SERVER_H

