#ifndef ENGINE_NET_SESSION_H
#define ENGINE_NET_SESSION_H

#include <third_party/asio.hpp>
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
    explicit session(asio::io_service& work_service, asio::io_service& io_service)
        : socket_(io_service)
        , io_work_service_(work_service)
    {
        read_buffer_ = std::make_shared<asio_buffer>();
        write_buffer_ = std::make_shared<asio_buffer>();
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    void start()
    {
        read();
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
            io_work_service_.post([this, self]() {on_receive();});
            read();
        } else {
            printf("read error is %s \n", ec.message().c_str());
        }
    }

    void on_receive()
    {
        std::unique_ptr<data_block> free_data = read_buffer_->read(read_buffer_->readable_bytes());
        printf("receive : %i bytes. message : %s \n", free_data->len, free_data->data);
    }

private:
    asio::io_service& io_work_service_;
    tcp::socket socket_;
    std::shared_ptr<asio_buffer> read_buffer_;
    std::shared_ptr<asio_buffer> write_buffer_;
}; // class session

typedef std::shared_ptr<session> session_ptr;

} // namespace engine

#endif // ENGINE_NET_SESSION_H

