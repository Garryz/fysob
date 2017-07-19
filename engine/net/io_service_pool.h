#ifndef ENGINE_NET_IO_SERVICE_POOL_H
#define ENGINE_NET_IO_SERVICE_POOL_H

#include <thread>
#include <third_party/asio.hpp>

namespace engine
{

class io_service_pool
{
public:
    io_service_pool(const io_service_pool&) = delete;
    io_service_pool& operator=(const io_service_pool&) = delete;
    explicit io_service_pool(std::size_t pool_size, std::string pool_name = "")
        : next_io_service_(0)
        , pool_name_(pool_name)
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
        for (std::size_t i = 0; i < io_services_.size(); ++i) {
            std::shared_ptr<std::thread> thread(new std::thread([=]() {io_services_[i]->run(); }));
            printf("%s_thread_%d : %04X\n", pool_name_.c_str(), i, thread->get_id());
            threads_.push_back(thread);
        }
    }

    void stop()
    {
        for (std::size_t i = 0; i < io_services_.size(); ++i) {
            io_services_[i]->stop();
        }

        for (std::size_t i = 0; i < threads_.size(); ++i) {
            threads_[i]->join();
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
    typedef std::shared_ptr<asio::io_service>           io_service_ptr;
    typedef std::shared_ptr<asio::io_service::work>     work_ptr;

    std::string                                 pool_name_;
    std::vector<io_service_ptr>                 io_services_;
    std::vector<work_ptr>                       work_;
    std::vector<std::shared_ptr<std::thread> >  threads_;
    std::size_t                                 next_io_service_;
};

typedef std::shared_ptr<io_service_pool> io_service_pool_ptr;

} // namespace engine

#endif // ENGINE_NET_IO_SERVICE_POOL_H

