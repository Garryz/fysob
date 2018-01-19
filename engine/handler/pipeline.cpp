#include "pipeline.h"

#include <engine/net/session.h>

namespace engine
{

std::shared_ptr<asio_buffer> pipeline::read_buffer()
{
    return session_->read_buffer();
}

std::shared_ptr<asio_buffer> pipeline::write_buffer()
{
    return session_->write_buffer();
}

void pipeline::notify_write(std::size_t length)
{
    session_->notify_write(length);
}

uint32_t pipeline::session_id()
{
    return session_->id();
}

void pipeline::close()
{
    session_->close();
}

} // namespace engine
