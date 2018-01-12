#ifndef ENGINE_HANDLE_PIPELINE_H
#define ENGINE_HANDLE_PIPELINE_H

#include <vector>

#include <engine/handle/context.h>
#include <engine/net/asio_buffer.h>

namespace engine
{

class session;

class pipeline
{
public:
    pipeline(const pipeline&) = delete;
    pipeline& operator=(const pipeline&) = delete;
    explicit pipeline(std::shared_ptr<session> session)
        : session_(session)
    {
        head_ = new head_context(this);
        tail_ = new tail_context(this);
        head_->next = tail_;
        tail_->prev = head_;
        ctxs_.push_back(std::unique_ptr<context>(head_));
        ctxs_.push_back(std::unique_ptr<context>(tail_));
    }

    ~pipeline()
    {
        ctxs_.clear();
        tail_ = NULL;
        head_ = NULL;
    }

    std::shared_ptr<asio_buffer> read_buffer();

    std::shared_ptr<asio_buffer> write_buffer();

    void notify_write(std::size_t length);

    void write(std::unique_ptr<any> msg)
    {
        if (write_data_type<data_block>(*msg)) {}
        else if (write_data_type<read_data_ptr>(*msg)) {}
        else if (write_data_type<write_data_ptr>(*msg)) {}
        else if (write_string_type<std::string>(*msg)) {}
        else if (write_string_type<std::string&>(*msg)) {}
        else if (write_string_type<const std::string&>(*msg)) {}
        else if (write_base_type<int8_t>(*msg)) {}
        else if (write_base_type<int16_t>(*msg)) {}
        else if (write_base_type<int32_t>(*msg)) {}
        else if (write_base_type<int64_t>(*msg)) {}
        else if (write_base_type<uint8_t>(*msg)) {}
        else if (write_base_type<uint16_t>(*msg)) {}
        else if (write_base_type<uint32_t>(*msg)) {}
        else if (write_base_type<uint64_t>(*msg)) {}
        else if (write_base_type<float>(*msg)) {}
        else if (write_base_type<double>(*msg)) {}
        else {
            printf("unsupported write type \n");
        }
    }

    template<typename BASE_DATA_TYPE>
    bool write_base_type(any msg)
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

    template<typename DATA_TYPE>
    bool write_data_type(any msg)
    {
        if (msg.type() == typeid(DATA_TYPE)) {
            DATA_TYPE data = any_cast<DATA_TYPE>(msg);   
            write_buffer()->append(data.data, data.len);
            notify_write(data.len);
            return true;
        }
        return false;
    }

    std::size_t session_id();

    void close();

    pipeline& add_handler(std::shared_ptr<abstract_handler> handler)
    {
        context* ctx = new context(this, handler);
        tail_->prev->next = ctx;
        ctx->prev = tail_->prev; 
        ctx->next = tail_;
        tail_->prev = ctx;
        ctxs_.push_back(std::unique_ptr<context>(ctx));
        return *this;
    }

    void fire_connect()
    {
        head_->connect();
    }

    void fire_read()
    {
        auto msg = new any(read_buffer());
        head_->fire_read(std::move(std::unique_ptr<any>(msg)));
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
    class head_context : public context
    {
    public:
        head_context(const head_context&) = delete;
        head_context& operator=(const head_context&) = delete;
        explicit head_context(pipeline* pipeline)
            : context(pipeline, NULL)
        {
        }

        virtual ~head_context()
        {
        }

        virtual void connext()
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
    }; // class head_context

    class tail_context : public context
    {
    public:
        tail_context(const tail_context&) = delete; 
        tail_context& operator=(const tail_context&) = delete;
        tail_context(pipeline* pipeline)
            : context(pipeline, NULL)
        {
        }

        virtual ~tail_context()
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
    }; // class tail_context

    std::shared_ptr<session> session_;
    context* head_;
    context* tail_;
    std::vector<std::unique_ptr<context>> ctxs_;
    any user_data_;
}; // class pipeline

} // namespace engine

#endif // ENGINE_HANDLE_PIPELINE_H
