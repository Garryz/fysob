#ifndef ENGINE_NET_ASIO_BUFFER_H
#define ENGINE_NET_ASIO_BUFFER_H

#include <list>
#include <mutex>
#include <third_party/asio.hpp>
#include <engine/common/data_block.h>
#include <engine/net/endian.h>

namespace engine
{

// only thread safe for one thread read/write and ther other write/read
class asio_buffer
{
public:
    static const std::size_t kInitialSize       = 512;
    static const std::size_t kRemainRatio       = 8;
    static const std::size_t kLowUseCeilCount   = 10;

    asio_buffer(const asio_buffer&) = delete;
    asio_buffer operator=(const asio_buffer&) = delete;
    explicit asio_buffer(std::size_t initial_size = kInitialSize);

    asio_buffer& append(const char* /*restrict*/ data, std::size_t len);

    asio_buffer& append(const void* /*restrict*/ data, std::size_t len)
    {
        return append(static_cast<const char*>(data), len);
    }

    template<typename BASE_DATA_TYPE>
    asio_buffer& append(BASE_DATA_TYPE x)
    {
        return append(&adapte_endian(x), sizeof x);    
    }

    asio_buffer& append(const std::string& str)
    {
        return append(str.data(), str.size());
    }

    std::unique_ptr<data_block> peek(std::size_t len);
    
    template<typename BASE_DATA_TYPE>
    BASE_DATA_TYPE peek()
    {
        return get_base_data_type([=](std::size_t len){return peek(len);});
    }
    
    std::unique_ptr<data_block> read(std::size_t len)
    {
        std::unique_ptr<data_block> free_data = peek(len);
        retrieve(len);
        return free_data;
    }

    template<typename BASE_DATA_TYPE>
    BASE_DATA_TYPE read()
    {
        return get_base_data_type([=](std::size_t len){return read(len);});
    }

    void retrieve(std::size_t len);
    void has_written(std::size_t len);

    std::size_t readable_bytes() const
    {
        return readable_bytes_;
    }

    std::size_t writable_bytes() const
    {
        return writable_bytes_;
    }

    std::vector<asio::mutable_buffer>& mutable_buffer();
    const std::vector<asio::const_buffer>& const_buffer();
    std::vector<write_data_ptr>& write_buffer();
    const std::vector<read_data_ptr>& read_buffer();
private:
    typedef std::shared_ptr<data_block>     block_ptr;
    typedef std::list<block_ptr>::iterator  buffer_iter;

    template<typename BASE_DATA_TYPE>
    BASE_DATA_TYPE get_base_data_type(const std::function<std::unique_ptr<data_block>(std::size_t)>& func)
    {
        std::size_t bytes = sizeof(BASE_DATA_TYPE);
        assert(readable_bytes() >= bytes);
        std::unique_ptr<data_block> free_data = func(bytes);
        if (free_data->len != bytes) return 0;
        else return adapte_endian(*reinterpret_cast<BASE_DATA_TYPE*>(free_data->data));
    }

    void check_active();
    // adjust_buffer have to make sure at least 1 byte to write
    void adjust_buffer(std::size_t len);
    void write_bytes(std::size_t len);
    void check_to_add_block(std::size_t len);
    std::size_t add_block();
    
    void set_write_index(const buffer_iter& iter, std::size_t index)
    {
        std::lock_guard<std::mutex> set_guard(write_index_mutex_);
        write_buffer_iter_   = iter;
        write_index_        = index;
    }

    void get_write_index(buffer_iter& iter, std::size_t& index)
    {
        std::lock_guard<std::mutex> get_guard(write_index_mutex_);
        iter    = write_buffer_iter_;
        index   = write_index_;
    }

    void adjust_index(std::size_t len, buffer_iter& iter, std::size_t& index);
private:
    std::list<block_ptr>                buffer_;
    std::vector<asio::mutable_buffer>   mutable_buffer_;
    std::vector<asio::const_buffer>     const_buffer_;
    std::vector<write_data_ptr>         write_buffer_;
    std::vector<read_data_ptr>          read_buffer_;
    buffer_iter                         read_buffer_iter_;
    buffer_iter                         write_buffer_iter_;
    std::size_t                         read_index_;
    std::size_t                         write_index_;
    std::size_t                         block_size_;
    std::size_t                         readable_bytes_;
    std::size_t                         writable_bytes_;
    std::size_t                         total_bytes_;
    std::size_t                         low_use_count_;
    std::mutex                          write_index_mutex_;
    bool                                active_;
}; // class asio_buffer

} // namespace engine

#endif // ENGINE_NET_ASIO_BUFFER_H

