#ifndef ENGINE_HANDLE_CONTEXT_H
#define ENGINE_HANDLE_CONTEXT_H

#include <memory>

#include <engine/common/any.h>

namespace engine
{

class abstract_handler;

class context
{
public:
    context(const context&) = delete;
    context& operator=(const context&) = delete;
    explicit context(std::shared_ptr<abstract_handler> handler)
        : prev(NULL)
        , next(NULL)
        , handler_(handler)
    {
    }

    virtual ~context()
    {
        handler_ = NULL;
        next = NULL;
        prev = NULL;
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

    context* prev;
    context* next;
private:
    std::shared_ptr<abstract_handler> handler_;
}; // class context

} // namespace engine

#endif // ENGINE_HANDLE_CONTEXT_H

