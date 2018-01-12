#ifndef ENGINE_COMMON_COMMON_H
#define ENGINE_COMMON_COMMON_H

#include <atomic>

namespace engine
{

static std::atomic_size_t auto_increase_id(0);

static std::size_t get_increase_id()
{
    auto_increase_id++;
    return auto_increase_id;
}

} // namespace engine

#endif // ENGINE_COMMON_COMMON_H
