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
        socket_.async_read_some(buffer_.mutable_buffer(),
            [this, self](std::error_code ec, std::size_t length){handle_read(ec, length);});
    }

    void handle_read(std::error_code& ec, std::size_t length)
    {
        if (!ec) {
            buffer_.has_written(length);
            std::shared_ptr<std::vector<char> > buf(new std::vector<char>);
            buf->resize(length);
            std::unique_ptr<data_block> free_data = buffer_.read(length);
            std::copy(free_data->data, free_data->data + free_data->len, buf->begin());
            auto self(shared_from_this());
            io_work_service_.post([this, self, buf, length]() {on_receive(buf, length);});
            read();
        } else {
            std::cout << "read error is " << ec.message() << std::endl;
        }
    }

    void on_receive(std::shared_ptr<std::vector<char> > buffers, std::size_t length)
    {
        char* data_stream = &(*buffers->begin());
        std::cout << "receive :" << length << " bytes. "
            << "message :" << data_stream << std::endl;
    }

private:
    asio::io_service& io_work_service_;
    tcp::socket socket_;
    asio_buffer buffer_;
}; // class session

typedef std::shared_ptr<session> session_ptr;

} // namespace engine

#endif // ENGINE_NET_SESSION_H

