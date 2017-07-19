#include <stdio.h>
#include <cstdlib>
#include <memory>
#include <iostream>
#include <thread>
#include <third_party/asio.hpp>

using asio::ip::tcp;

class handler_allocator
{
public:
    handler_allocator()
        : in_uses_(false)
    {
    }

    handler_allocator(const handler_allocator&) = delete;
    handler_allocator& operator=(const handler_allocator&) = delete;

    void* allocate(std::size_t size)
    {
        if (!in_uses_ && size < sizeof(storage_)) {
            in_uses_ = true;
            return &storage_;
        } else {
            return ::operator new(size);
        }
    }

    void deallocate(void* pointer)
    {
        if (pointer == &storage_) {
            in_uses_ = false;
        } else {
            ::operator delete(pointer);
        }
    }
private:
    typename std::aligned_storage<1024>::type storage_;
    bool in_uses_;
};

template <typename Handler>
class custom_alloc_handler
{
public:
    custom_alloc_handler(handler_allocator& a, Handler h)
        : allocator_(a),
          handler_(h)
    {
    }

    template <typename ...Args>
    void operator()(Args&&... args)
    {
        handler_(std::forward<Args>(args)...);
    }

    friend void* asio_handler_allocate(std::size_t size,
            custom_alloc_handler<Handler>* this_handler)
    {
        return this_handler->allocator_.allocate(size);
    }

    friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/,
            custom_alloc_handler<Handler>* this_handler)
    {
        return this_handler->allocator_.deallocate(pointer);
    }
private:
    handler_allocator& allocator_;
    Handler handler_;
};

template <typename Handler>
inline custom_alloc_handler<Handler> make_custom_alloc_handler(
        handler_allocator& a, Handler h)
{
    return custom_alloc_handler<Handler>(a, h);
}

class io_service_pool
{
public:
    io_service_pool(const io_service_pool&) = delete;
    io_service_pool& operator = (const io_service_pool&) = delete;
    explicit io_service_pool(std::size_t pool_size)
        : next_io_service_(0)
    {
        if (pool_size == 0) {
            throw std::runtime_error("io_service_pool size is 0");
        }

        for (std::size_t i = 0; i < pool_size; ++i) {
            io_service_ptr io_service(new asio::io_service);
            work_ptr work(new asio::io_service::work(*io_service));
            io_services_.push_back(io_service);
            work_.push_back(work);
        }
    }

    void run()
    {
        std::vector<std::shared_ptr<std::thread> > threads;
        for (std::size_t i = 0; i < io_services_.size(); ++i) {
            threads.push_back(std::make_shared<std::thread>([=](){io_services_[i]->run();}));
        }
        for (std::size_t i = 0; i < threads.size(); ++i) {
            threads[i]->join();
        }
    }

    void stop()
    {
        for (std::size_t i = 0; i < io_services_.size(); ++i) {
            io_services_[i]->stop();
        }
    }

    asio::io_service& get_io_service()
    {
        asio::io_service& io_service = *io_services_[next_io_service_];
        ++next_io_service_;
        if (next_io_service_ == io_services_.size()) {
            next_io_service_ = 0;
        }
        return io_service;
    }
private:
    typedef std::shared_ptr<asio::io_service> io_service_ptr;
    typedef std::shared_ptr<asio::io_service::work> work_ptr;

    std::vector<io_service_ptr> io_services_;
    std::vector<work_ptr> work_;
    std::size_t next_io_service_;
};

class session
    : public std::enable_shared_from_this<session>
{
public:
    session(asio::io_service& work_service, asio::io_service& io_service)
        : socket_(io_service)
        , io_work_service(work_service)
    {
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    void start()
    {
        auto self(shared_from_this());
        socket_.async_read_some(asio::buffer(data_),
                make_custom_alloc_handler(allocator_,
                    [this, self](std::error_code ec, std::size_t length)
                    {handle_read(ec, length);}));
    }

    void handle_read(std::error_code ec, std::size_t length)
    {
        if (!ec) {
            std::shared_ptr<std::vector<char> > buf(new std::vector<char>);
            buf->resize(length);
            std::copy(data_.begin(), data_.begin() + length, buf->begin());
            auto self(shared_from_this());
            io_work_service.post([this, self, buf, length](){on_receive(buf, length);});

            socket_.async_read_some(asio::buffer(data_),
                    make_custom_alloc_handler(allocator_,
                        [this, self](std::error_code ec, std::size_t length)
                        {handle_read(ec, length);}));
        }
    }

    void handle_write(std::error_code ec)
    {
        if (!ec) {
        }
    }

    void on_receive(std::shared_ptr<std::vector<char> > buffers, size_t length)
    {
        char* data_stream = &(*buffers->begin());
        std::cout << "receive :" << length << " bytes. " <<
            "messages :" << data_stream << std::endl;
    }
private:
    asio::io_service& io_work_service;
    tcp::socket socket_;
    std::array<char, 1024> data_;
    handler_allocator allocator_;
};

typedef std::shared_ptr<session> session_ptr;

class server
{
public:
    server(short port, std::size_t io_service_pool_size)
        : io_service_pool_(io_service_pool_size)
        , io_service_work_pool_(io_service_pool_size)
        , acceptor_(io_service_pool_.get_io_service(), tcp::endpoint(tcp::v4(), port))
    {
        session_ptr new_session(new session(io_service_work_pool_.get_io_service(),
                    io_service_pool_.get_io_service()));
        acceptor_.async_accept(new_session->socket(), 
                [=](std::error_code ec){handle_accept(new_session, ec);});
    }

    void handle_accept(session_ptr new_session, std::error_code ec)
    {
        if (!ec) {
            new_session->start();
            new_session.reset(new session(io_service_work_pool_.get_io_service(),
                        io_service_pool_.get_io_service()));
            acceptor_.async_accept(new_session->socket(),
                    [=](std::error_code ec){handle_accept(new_session, ec);});
        }
    }

    void run()
    {
        io_thread_.reset(new std::thread([this](){io_service_pool_.run();}));
        work_thread_.reset(new std::thread([this](){io_service_work_pool_.run();}));
    }

    void stop()
    {
        io_service_pool_.stop();
        io_service_work_pool_.stop();
        
        io_thread_->join();
        work_thread_->join();
    }
private:
    std::shared_ptr<std::thread> io_thread_;
    std::shared_ptr<std::thread> work_thread_;
    io_service_pool io_service_pool_;
    io_service_pool io_service_work_pool_;
    tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
    try {
        if (argc != 2) {
            std::cerr << "Usage: server <port>\n";
            return 1;
        }

        using namespace std;
        server s(atoi(argv[1]), 10);

        s.run();

        getchar();

        s.stop();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
