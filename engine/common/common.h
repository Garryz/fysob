#ifndef ENGINE_COMMON_COMMON_H
#define ENGINE_COMMON_COMMON_H

#include <atomic>

namespace engine
{

enum class AppType {
    ROUTE   = 1,
    GAME    = 2,
    ARENA   = 3,
};

static std::size_t MAX_MESSAGE_SIZE = 1024;

static std::atomic<uint32_t> auto_session_increase_id_{0};

static uint32_t get_session_increase_id()
{
    return ++auto_session_increase_id_;
}

} // namespac engine 

#endif // ENGINE_COMMON_COMMON_H
