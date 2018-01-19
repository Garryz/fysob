#ifndef ENGINE_COMMON_DATA_BLOCK_H
#define ENGINE_COMMON_DATA_BLOCK_H

#include <cstddef>
#include <cstring>

namespace engine
{

struct data_block
{
    char*       data;
    std::size_t len;
    bool        is_owner = true;

    data_block(const data_block&) = delete;
    data_block& operator=(const data_block&) = delete;
    explicit data_block(std::size_t data_len)
        : is_owner(true)
    {
        data    = new char[data_len];
        len     = data_len;
    }

    data_block(const char* block, std::size_t block_len)
        : is_owner(true)
    {
        data    = new char[len];
        memcpy(data, block, block_len);
        len     = block_len;
    }

    data_block(data_block&& that)
        : data(that.data)
        , len(that.len)
        , is_owner(true)
    {
        that.is_owner = false;
    }

    data_block& operator=(data_block&& that)
    {
        data            = that.data;
        len             = that.len;
        is_owner        = true;
        that.is_owner   = false;
        return *this;
    }

    ~data_block()
    {
        if (is_owner) {
            delete[] data;
        }
        len = 0;
    }
};

struct read_data
{
    const char* data;
    std::size_t len;

    read_data() = default;

    read_data(const data_block* that)
        : data(that->data)
        , len(that->len)
    {}

    read_data(const data_block& that)
        : data(that.data)
        , len(that.len)
    {}
};

struct write_data
{
    char*       data;
    std::size_t len;
    
    write_data() = default;

    write_data(const data_block* that)
        : data(that->data)
        , len(that->len)
    {}

    write_data(const data_block& that)
        : data(that.data)
        , len(that.len)
    {}
};

} // namespace engine

#endif // ENGINE_COMMON_DATA_BLOCK_H
