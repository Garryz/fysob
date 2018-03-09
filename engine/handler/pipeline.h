#ifndef ENGINE_HANDLER_PIPELINE_H
#define ENGINE_HANDLER_PIPELINE_H

#include <third_party/g3log/g3log/g3log.hpp>

#include <engine/common/any.h>
#include <engine/handler/context.h>
#include <engine/net/asio_buffer.h>

namespace engine
{

class session;
class pipeline
{
public:
    pipeline(const pipeline&) = delete;
    pipeline& operator=(const pipeline&) = delete;
    explicit pipeline(session* session)
        : session_(session)
    {
        head_ = new head_context(this);
        tail_ = new tail_context(this);
        head_->next = tail_;
        tail_->prev = head_;
        contexts_.push_back(std::unique_ptr<context>(head_));
        contexts_.push_back(std::unique_ptr<context>(tail_));
    }

    ~pipeline()
    {
        contexts_.clear();
        tail_ = nullptr;
        head_ = nullptr;
    }    

    std::shared_ptr<asio_buffer> read_buffer();

    std::shared_ptr<asio_buffer> write_buffer();

    void notify_write(std::size_t length);

    void write(std::unique_ptr<any> msg)
    {
        if (write_data_struct<read_data>(*msg)) {}
        else if (write_data_struct<write_data>(*msg)) {}
        else if (write_string_type<std::string>(*msg)) {}
        else if (write_string_type<std::string&>(*msg)) {}
        else if (write_string_type<const std::string&>(*msg)) {}
        else if (write_base_data_type<char>(*msg)) {}
        else if (write_base_data_type<int8_t>(*msg)) {}
        else if (write_base_data_type<int16_t>(*msg)) {}
        else if (write_base_data_type<int32_t>(*msg)) {}
        else if (write_base_data_type<int64_t>(*msg)) {}
        else if (write_base_data_type<uint8_t>(*msg)) {}
        else if (write_base_data_type<uint16_t>(*msg)) {}
        else if (write_base_data_type<uint32_t>(*msg)) {}
        else if (write_base_data_type<uint64_t>(*msg)) {}
        else if (write_base_data_type<float>(*msg)) {}
        else if (write_base_data_type<double>(*msg)) {}
        else if (write_base_data_type<long double>(*msg)) {}
        else {
            throw std::bad_cast();
        }
    }

    uint32_t session_id();

    void close();
    
    pipeline& add_handler(const std::string& name, 
            std::shared_ptr<abstract_handler> handler)
    {
        context* ctx = new context(this, name, handler);
        tail_->prev->next = ctx;
        ctx->prev = tail_->prev; 
        ctx->next = tail_;
        tail_->prev = ctx;
        contexts_.push_back(std::unique_ptr<context>(ctx));
        return *this;
    }

    void fire_connect()
    {
        head_->connect();
    }

    void fire_read()
    {
        auto wrap_buffer = new any(read_buffer());
        head_->read(std::move(
                    std::unique_ptr<any>(wrap_buffer)));
    }

    void fire_closed()
    {
        head_->notify_closed();
    }

    void set_user_data(any user_data)
    {
        user_data_ = user_data;
    }

    any get_user_data()
    {
        return user_data_;
    }

private:
    template<typename BASE_DATA_TYPE>
    bool write_base_data_type(any msg)
    {
        if (msg.type() == typeid(BASE_DATA_TYPE)) {
            write_buffer()->append<BASE_DATA_TYPE>(
                    any_cast<BASE_DATA_TYPE>(msg));
            notify_write(sizeof(BASE_DATA_TYPE));
            return true;
        }
        return false;
    }

    template<typename STRING_TYPE>
    bool write_string_type(any msg)
    {
        if (msg.type() == typeid(STRING_TYPE)) {
            const std::string& str = any_cast<STRING_TYPE>(msg);
            write_buffer()->append(str);
            notify_write(str.size());
            return true;
        }
        return false;
    }

    template<typename DATA_STRUCT>
    bool write_data_struct(any msg)
    {
        if (msg.type() == typeid(DATA_STRUCT)) {
            DATA_STRUCT data = any_cast<DATA_STRUCT>(msg);
            write_buffer()->append(data.data, data.len);
            notify_write(data.len);
            return true;
        }
        return false;
    }

    class head_context : public context
    {
    public:
        head_context(const head_context&) = delete;
        head_context& operator=(const head_context&) = delete;
        explicit head_context(pipeline* pipeline)
            : context(pipeline, "head", nullptr)
        {
        }

        virtual ~head_context()
        {
        }

        virtual void connect()
        {
            fire_connect();
        }

        virtual void read(std::unique_ptr<any> msg)
        {
            fire_read(std::move(msg));
        }

        virtual void write(std::unique_ptr<any> msg)
        {
            pipeline_->write(std::move(msg));
        }

        virtual void close()
        {
            pipeline_->close();
        }

        virtual void notify_closed()
        {
            fire_closed();
        }
    private:
    }; // class head_context

    class tail_context : public context
    {
    public:
        tail_context(const tail_context&) = delete;
        tail_context& operator=(const tail_context&) = delete;
        explicit tail_context(pipeline* pipeline)
            : context(pipeline, "tail", nullptr)
        {
        }

        ~tail_context()
        {
        }

        virtual void connect()
        {
        }

        virtual void read(std::unique_ptr<any> msg)
        {
        }

        virtual void write(std::unique_ptr<any> msg)
        {
            fire_write(std::move(msg));
        }

        virtual void close()
        {
            fire_close();
        }

        virtual void notify_closed()
        {
        }
    }; // class tail_context

    session*    session_;
    context*    head_;
    context*    tail_;
    std::vector<std::unique_ptr<context>> contexts_;
    any         user_data_;
}; // class pipeline

} // namespace engine

#endif // ENGINE_HANDLER_PIPELINE_H
