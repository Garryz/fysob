#ifndef ENGINE_HANDLE_ABSTRACT_HANDLER_H
#define ENGINE_HANDLE_ABSTRACT_HANDLER_H

#include <engine/handle/context.h>

namespace engine
{

class abstract_handler
{
public:
    abstract_handler(const abstract_handler&) = delete;
    abstract_handler& operator=(const abstract_handler&) = delete;
    abstract_handler()
    {
    }

    virtual ~abstract_handler()
    {
    }

    virtual void connect(context* ctx)
    {
        ctx->fire_connect(); 
    }

    virtual void decode(context* ctx, std::unique_ptr<any> msg)
    {
        ctx->fire_read(std::move(msg));
    }

    virtual void encode(context* ctx, std::unique_ptr<any> msg)
    {
        ctx->fire_write(std::move(msg));
    }

    virtual void close(context* ctx)
    {
        ctx->fire_close();
    }
}; // class abstract_handler

} // namespace engine

#endif // ENGINE_HANDLE_ABSTRACT_HANDLER_H
