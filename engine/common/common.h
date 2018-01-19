#ifndef ENGINE_COMMON_COMMON_H
#define ENGINE_COMMON_COMMON_H

#include <atomic>

namespace engine
{

static std::atomic<uint32_t> auto_session_increase_id_{0};

static uint32_t get_session_increase_id()
{
    return ++auto_session_increase_id_;
}

} // namespac engine 

#endif // ENGINE_COMMON_COMMON_H
