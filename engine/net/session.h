#ifndef ENGINE_NET_SESSION_H
#define ENGINE_NET_SESSION_H

#include <atomic>
#include <third_party/asio.hpp>
#include <third_party/g3log/g3log/g3log.hpp>
#include <engine/handler/pipeline.h>

namespace engine
{
using asio::ip::tcp;

class session
    : public std::enable_shared_from_this<session>
{
public:
    session(const session&) = delete;
    session& operator=(const session&) = delete;
    session(uint32_t session_id, asio::io_service& work_service, asio::io_service& io_service)
        : id_(session_id)
        , socket_(io_service)
        , io_work_service_(work_service)
        , read_high_water_mask_(0)
        , write_high_water_mask_(0)
        , write_high_water_mask_handler_(nullptr)
        , close_handler_(nullptr)
        , pending_write_len_(0)
        , reading_(false)
        , writing_(false)
        , work_read_count_(0)
        , close_flag_(false)
        , handle_count_(0)
    {
        pipeline_       = std::make_shared<pipeline>(this);
        read_buffer_    = std::make_shared<asio_buffer>();
        write_buffer_   = std::make_shared<asio_buffer>();
        LOGF(DEBUG, "session create id = %d", id());
    }

    ~session()
    {
        pipeline_.reset();
        read_buffer_.reset();
        write_buffer_.reset();
        LOGF(DEBUG, "session destroy id = %d", id());
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    uint32_t id()
    {
        return id_;
    }

    void start(const std::function<void(std::shared_ptr<session>)>& init_handlers)
    {
        auto self(shared_from_this());
        if (init_handlers) {
            init_handlers(self);
        }
        pipeline_->fire_connect();
        read();
    }

    std::shared_ptr<asio_buffer> read_buffer()
    {
        return read_buffer_;
    }

    std::shared_ptr<asio_buffer> write_buffer()
    {
        return write_buffer_;
    }

    std::shared_ptr<session> add_handler(const std::string& name, 
        std::shared_ptr<abstract_handler> handler)
    {
        pipeline_->add_handler(name, handler);
        return shared_from_this();
    }

    std::shared_ptr<session> set_user_data(any user_data)
    {
        pipeline_->set_user_data(user_data);
        return shared_from_this();
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

    void set_close_handler(const std::function<void(uint32_t)>& handler)
    {
        close_handler_ = handler;
    }

    bool check_idle()
    {
        return handle_count_ == 0;
    }

    void reset_handle_count()
    {
        handle_count_ = 0;
    }

    void write(const char* data, std::size_t len)
    {
        auto block = std::make_shared<data_block>(data, len);
        auto self(shared_from_this());
        io_work_service_.post([=,&self](){
            write_buffer_->append(block->data, block->len); 
            notify_write(block->len);       
        });
    }

    template<typename BASE_DATA_TYPE>
    void write_endian(BASE_DATA_TYPE x, bool big_endian)
    {
        auto self(shared_from_this());
        io_work_service_.post([=,&self](){
            write_buffer_->append_endian(x, big_endian);
            notify_write(sizeof(BASE_DATA_TYPE));
        });
    }

    template<typename BASE_DATA_TYPE>
    void write(BASE_DATA_TYPE x)
    {
        auto self(shared_from_this());
        io_work_service_.post([=,&self](){
            write_buffer_->append(x);
            notify_write(sizeof(BASE_DATA_TYPE));
        });
    }

    void write(const char* str)
    {
        auto self(shared_from_this());
        io_work_service_.post([=,&self](){
            write_buffer_->append(str, strlen(str)); 
            notify_write(strlen(str));       
        });
    }

    void write(const std::string& str)
    {
        auto self(shared_from_this());
        io_work_service_.post([=,&self](){
            write_buffer_->append(str);
            notify_write(str.size());
        });
    }

    void notify_write(std::size_t length)
    {
        auto self(shared_from_this());
        pending_write_len_ += length;
        socket_.get_io_service().post([this, &self](){write();});
        if (write_high_water_mask_ != 0 && write_high_water_mask_handler_ 
                && write_buffer_->writable_bytes() > write_high_water_mask_) {
            write_high_water_mask_handler_(self, write_high_water_mask_);
        }
    }

    void close()
    {
        auto self(shared_from_this());
        close_flag_ = true;
        close_if_necessary();
    }
private:
    void read()
    {
        reading_ = true;
        auto self(shared_from_this());
        socket_.async_read_some(read_buffer_->mutable_buffer(),
            [this, &self](std::error_code ec, std::size_t length){handle_read(ec, length);});
    }

    void handle_read(std::error_code& ec, std::size_t length)
    {
        handle_count_++;
        reading_ = false;
        auto close_after_last_read = [this]()->bool{
            if (close_flag_ && !writing_) {
                socket_.shutdown(tcp::socket::shutdown_both);
                socket_.close();
                if (work_read_count_ == 0 && close_handler_) {
                    close_handler_(id());
                }
                return true;
            }
            return false;
        };

        if (!ec) {
            read_buffer_->has_written(length);
            if (close_after_last_read()) {
                return;
            } 

            auto self(shared_from_this());
            if (read_high_water_mask_ == 0 
                    || read_buffer_->readable_bytes() < read_high_water_mask_) {
                read();
            } else {
                read_buffer_->set_notify_behind_high_water_mask(
                        [this, &self](){read();}, read_high_water_mask_);
            }
            work_read_count_++;
            io_work_service_.post([this, &self](){
                pipeline_->fire_read();
                work_read_count_--;
                close_if_necessary(); 
            });
        } else {
            if (ec == asio::error::operation_aborted ||
                    ec == asio::error::eof) {
                LOGF(INFO, "session id = %d, read error = %s", id(), ec.message().c_str());
            } else {
                LOGF(FATAL, "session id = %d, read error = %s", id(), ec.message().c_str());
            }
            close_flag_ = true;
            close_after_last_read();
        }
    }

    void write()
    {
        if (!writing_ && pending_write_len_ > 0) {
            writing_ = true;
            auto self(shared_from_this());
            socket_.async_write_some(write_buffer_->const_buffer(),
                [this, &self](std::error_code ec, std::size_t length){handle_write(ec, length);});
        }
    }

    void handle_write(std::error_code& ec, std::size_t length)
    {
        handle_count_++;
        writing_ = false;
        auto close_after_last_write = [this]()->bool {
            if (close_flag_) {
                socket_.shutdown(tcp::socket::shutdown_both);
                if (!reading_) {
                    socket_.close();
                    if (work_read_count_ == 0 && close_handler_) {
                        close_handler_(id());
                    }
                }
                return true;
            }
            return false;            
        };

        if (!ec) {
            write_buffer_->retrieve(length);
            pending_write_len_ -= length;
            if (pending_write_len_ > 0) {
                write();
            }
            else {
                close_after_last_write();
            }
        } else {
            if (ec == asio::error::operation_aborted) {
                LOGF(INFO, "session id = %d, read error = %s", id(), ec.message().c_str());
            } else {
                LOGF(FATAL, "session id = %d, read error = %s", id(), ec.message().c_str());
            }
            close_flag_ = true;
            close_after_last_write();
        }
    }

    void close_if_necessary()
    {
        auto self(shared_from_this());
        if (close_flag_ && !writing_ && work_read_count_ == 0) {
            if (reading_) {
                socket_.cancel(); 
            }
            else {
                auto self(shared_from_this());
                socket_.get_io_service().post([this, &self](){
                    socket_.shutdown(tcp::socket::shutdown_both);
                    socket_.close();
                    if (close_handler_) {
                        close_handler_(id());
                    }
                });
            }
        }
    }
    
private:
    uint32_t                                        id_;
    asio::io_service&                               io_work_service_;
    tcp::socket                                     socket_;
    std::shared_ptr<asio_buffer>                    read_buffer_;
    std::shared_ptr<asio_buffer>                    write_buffer_;
    std::shared_ptr<pipeline>                       pipeline_;
    std::size_t                                     read_high_water_mask_;
    std::size_t                                     write_high_water_mask_;
    std::function<void(std::shared_ptr<session>, std::size_t)>   
                                                    write_high_water_mask_handler_;
    std::function<void(uint32_t)>                   close_handler_;
    std::atomic_size_t                              pending_write_len_;
    std::atomic_bool                                reading_;
    std::atomic_bool                                writing_;
    std::atomic_size_t                              work_read_count_; 
    std::atomic_bool                                close_flag_;
    std::atomic_size_t                              handle_count_;    
}; // class session

} // namespace engine

#endif // ENGINE_NET_SESSION_H

