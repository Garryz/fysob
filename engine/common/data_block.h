#ifndef ENGINE_COMMON_DATA_BLOCK_H
#define ENGINE_COMMON_DATA_BLOCK_H

#include <cstddef>

namespace engine
{

struct data_block
{
    char*       data;
    std::size_t len;

    explicit data_block(std::size_t data_len)
    {
        data    = new char[data_len];
        len     = data_len;
    }

    ~data_block()
    {
        delete[] data;
        len     = 0;
    }
};

struct read_data_ptr
{
    const char* data;
    std::size_t len;
};

struct write_data_ptr
{
    char*       data;
    std::size_t len;
};

} // namespace engine

#endif // ENGINE_COMMON_DATA_BLOCK_H
