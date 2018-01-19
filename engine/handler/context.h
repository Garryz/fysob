#ifndef ENGINE_HANDLER_CONTEXT_H
#define ENGINE_HANDLER_CONTEXT_H

#include <memory>
#include <string>

namespace engine
{

class abstract_handler;
class any;
class pipeline;

class context
{
public:
    context(const context&) = delete;
    context& operator=(const context&) = delete;
    context(pipeline* pipeline, 
            const std::string& name,
            std::shared_ptr<abstract_handler> handler)
        : prev(nullptr)
        , next(nullptr)
        , pipeline_(pipeline)
        , name_(name)
        , handler_(handler)
    {
    }

    virtual ~context()
    {
        handler_    = nullptr;
        next        = nullptr;
        prev        = nullptr;
    }

    virtual void connect();

    void fire_connect()
    {
        if (next) {
            next->connect();
        }
    }

    virtual void read(std::unique_ptr<any> msg);

    void fire_read(std::unique_ptr<any> msg)
    {
        if (next) {
            next->read(std::move(msg));
        }
    }

    virtual void write(std::unique_ptr<any> msg);

    void fire_write(std::unique_ptr<any> msg)
    {
        if (prev) {
            prev->write(std::move(msg));
        }
    }

    virtual void close();

    void fire_close()
    {
        if (prev) {
            prev->close();
        }
    }

    std::size_t session_id();

    void set_user_data(any user_data);

    any get_user_data();

    context*    prev;
    context*    next;
protected:  
    pipeline*   pipeline_;
private:
    std::string name_;
    std::shared_ptr<abstract_handler> handler_;
}; // class context

} // namespace engine

#endif // ENGINE_HANDLER_CONTEXT_H
