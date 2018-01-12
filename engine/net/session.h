#ifndef ENGINE_NET_SESSION_H
#define ENGINE_NET_SESSION_H

#include <third_party/asio.hpp>
#include <engine/handle/pipeline.h>
#include <engine/net/asio_buffer.h>

namespace engine
{
using asio::ip::tcp;

class session
    : public std::enable_shared_from_this<session>
{
public:
    session(const session&) = delete;
    session& operator=(const session&) = delete;
    explicit session(std::size_t id, 
            asio::io_service& work_service, 
            asio::io_service& io_service)
        : socket_(io_service)
        , io_work_service_(work_service)
        , write_remain_length_(0)
        , writing_(false)
        , id_(id)
    {
        read_buffer_ = std::make_shared<asio_buffer>();
        write_buffer_ = std::make_shared<asio_buffer>();
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    std::shared_ptr<asio_buffer> read_buffer()
    {
        return read_buffer_;
    }

    std::shared_ptr<asio_buffer> write_buffer()
    {
        return write_buffer_;
    }

    std::size_t id()
    {
        return id_;
    }

    std::shared_ptr<session> add_handler(std::shared_ptr<abstract_handler> handler)
    {
        pipeline_->add_handler(handler);
        return shared_from_this();
    }

    std::shared_ptr<session> set_user_data(any user_data)
    {
        pipeline_->set_user_data(user_data);
        return shared_from_this();
    }
    
    void start(const std::function<void(std::shared_ptr<session>)>& init_handlers)
    {
        auto self(shared_from_this());
        pipeline_ = new pipeline(self);
        init_handlers(self);
        read();
    }

    void notify_write(std::size_t length)
    {
        write_remain_length_ += length;
        if (!writing_ && write_remain_length_ > 0) {
            write();
        }
    }

    void close()
    {
        // TODO
    }

private:
    void read()
    {
        auto self(shared_from_this());
        socket_.async_read_some(read_buffer_->mutable_buffer(),
            [this, self](std::error_code ec, std::size_t length){handle_read(ec, length);});
    }

    void handle_read(std::error_code& ec, std::size_t length)
    {
        if (!ec) {
            read_buffer_->has_written(length);
            auto self(shared_from_this());
            io_work_service_.post([this, self]() {pipeline_->fire_read();});
            read();
        } else {
            printf("read error is %s \n", ec.message().c_str());
        }
    }

    void write()
    {
        writing_ = true;
        auto self(shared_from_this());
        socket_.async_write_some(write_buffer_->const_buffer(),
            [this, self](std::error_code ec, std::size_t length){handle_write(ec, length);});
    }

    void handle_write(std::error_code& ec, std::size_t length)
    {
        if (!ec) {
            write_buffer_->retrieve(length);
            write_remain_length_ -= length;
            writing_ = false;
            if (write_remain_length_ > 0) {
                write();
            }
        } else {
            printf("write error is %s \n", ec.message().c_str());
        }
    }
private:
    asio::io_service& io_work_service_;
    tcp::socket socket_;
    std::shared_ptr<asio_buffer> read_buffer_;
    std::shared_ptr<asio_buffer> write_buffer_;
    std::atomic_size_t write_remain_length_;
    std::atomic_bool writing_;
    std::size_t id_;
    pipeline* pipeline_;
}; // class session

typedef std::shared_ptr<session> session_ptr;

} // namespace engine

#endif // ENGINE_NET_SESSION_H

