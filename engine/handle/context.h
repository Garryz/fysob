#ifndef ENGINE_HANDLE_CONTEXT_H
#define ENGINE_HANDLE_CONTEXT_H

#include <memory>

#include <engine/common/any.h>

namespace engine
{

class context
{
public:
    context(const context&) = delete;
    context& operator=(const context&) = delete;
    context()
    {
    }

    virtual ~context();

    virtual void connect();

    void fire_connect();

    virtual void read(std::unique_ptr<any> msg);

    void fire_read(std::unique_ptr<any> msg);

    virtual void write(std::unique_ptr<any> msg);

    void fire_write(std::unique_ptr<any> msg);

    virtual void close();

    void fire_close();
}; // class context

} // namespace engine

#endif // ENGINE_HANDLE_CONTEXT_H

