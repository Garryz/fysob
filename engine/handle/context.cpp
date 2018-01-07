#include "context.h"

#include <engine/handle/abstract_handler.h>

namespace engine
{

void context::connect()
{
    handler_->connect(this);
}

void context::read(std::unique_ptr<any> msg)
{
    handler_->decode(this, std::move(msg));
}

void context::write(std::unique_ptr<any> msg)
{
    handler_->encode(this, std::move(msg));
}

void context::close()
{
    handler_->close(this);
}

} // namespace engine
