#include "context.h"

#include <engine/handler/abstract_handler.h>
#include <engine/handler/pipeline.h>

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

std::uint32_t context::session_id()
{
    return pipeline_->session_id();
}

void context::set_user_data(any user_data)
{
    pipeline_->set_user_data(user_data);
}

any context::get_user_data()
{
    return pipeline_->get_user_data();
}

} // namespace engine
