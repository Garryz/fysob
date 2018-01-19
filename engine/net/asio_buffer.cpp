#include "asio_buffer.h"

namespace engine {

asio_buffer::asio_buffer(std::size_t initial_size)
    : read_index_(0)
    , write_index_(0)
    , active_(false)
    , low_use_count_(0)
    , high_water_mask_(0)
    , notify_behind_high_water_mask_(nullptr)
{
    block_ptr fake_block(new data_block(1));
    buffer_.push_back(fake_block);
    read_buffer_iter_ = buffer_.begin();
    write_buffer_iter_ = buffer_.begin();
    block_size_ = ((initial_size % kInitialSize == 0) 
        ? (initial_size / kInitialSize) 
        : (initial_size / kInitialSize + 1)) * kInitialSize;
    readable_bytes_ = 0;
    writable_bytes_ = 1;
    total_bytes_    = 1;
}

asio_buffer::~asio_buffer()
{
}

void asio_buffer::check_active()
{
    if (!active_) {
        char first_char = (*buffer_.begin())->data[0];
        (*buffer_.begin()).reset(new data_block(block_size_));
        (*buffer_.begin())->data[0] = first_char;
        block_ptr new_block(new data_block(block_size_));
        buffer_.push_back(new_block);
        writable_bytes_ = 2 * block_size_;
        total_bytes_    = 2 * block_size_;
        active_ = true;
    }
}

void asio_buffer::adjust_buffer(std::size_t len)
{
    while (read_buffer_iter_ != buffer_.begin()) {
        block_ptr block = buffer_.front();
        buffer_.pop_front();
        buffer_.push_back(block);
        writable_bytes_ += block->len;
    }

    if (writable_bytes_ >= len) {
        check_to_add_block(len);

        if (buffer_.size() > 3 && (readable_bytes_ + writable_bytes_) / 2 < total_bytes_) {
            low_use_count_ += 1;
        }
        if (low_use_count_ >= kLowUseCeilCount) {
            std::size_t reduce_len = total_bytes_ / 4;
            while (reduce_len >= (*buffer_.rbegin())->len) {
                reduce_len -= (*buffer_.rbegin())->len;
                total_bytes_ -= (*buffer_.rbegin())->len;
                buffer_.pop_back();
            } 
            low_use_count_ = 0;
        }
        return;
    }

    low_use_count_ = 0;

    std::size_t remain_len = len - writable_bytes_;
    while (remain_len > 0) {
        std::size_t block_size = add_block();
        remain_len = remain_len > block_size ? remain_len - block_size : 0;
    }
    check_to_add_block(len);
}

void asio_buffer::write_bytes(std::size_t len)
{
    readable_bytes_ += len;
    writable_bytes_ -= len;
}

void asio_buffer::check_to_add_block(std::size_t len)
{
    assert(len <= writable_bytes_);
    if (len > writable_bytes_) len = writable_bytes_;
    if (writable_bytes_ - len < block_size_ / kRemainRatio) {
        add_block();
    }
}

std::size_t asio_buffer::add_block()
{
    block_ptr new_block(new data_block(block_size_));
    buffer_.push_back(new_block);
    writable_bytes_ += block_size_;
    total_bytes_    += block_size_;
    return block_size_;
}

void asio_buffer::adjust_index(std::size_t len, buffer_iter& iter, std::size_t& index)
{
    std::size_t block_size = (*iter)->len;
    std::size_t block_remain_len = block_size - index;
    if (block_remain_len > len) {
        index += len;
    } else {
        len -= block_remain_len;
        iter++;
        block_size = (*iter)->len;
        while (block_size <= len) {
            len -= block_size;
            iter++;
            block_size = (*iter)->len;
        }
        index = len;
    }
}

asio_buffer& asio_buffer::append(const char* /*restrict*/ data, std::size_t len)
{
    check_active();
    adjust_buffer(len);

    buffer_iter write_buffer_iter;
    std::size_t write_index;
    get_write_index(write_buffer_iter, write_index);

    std::size_t write_block_size = (*write_buffer_iter)->len;
    std::size_t write_block_remain_len = write_block_size - write_index;
    if (write_block_remain_len > len) {
        std::copy(data, data + len, (*write_buffer_iter)->data + write_index);
    } else {
        buffer_iter temp_write_buffer_iter = write_buffer_iter;
        std::copy(data, data + write_block_remain_len, (*temp_write_buffer_iter)->data + write_index);
        data += write_block_remain_len;
        std::size_t remain_len = len - write_block_remain_len;
        temp_write_buffer_iter++;
        write_block_size = (*temp_write_buffer_iter)->len;
        while (write_block_size <= remain_len) {
            std::copy(data, data + write_block_size, (*temp_write_buffer_iter)->data);
            data += write_block_size;
            remain_len -= write_block_size;
            temp_write_buffer_iter++;
            write_block_size = (*temp_write_buffer_iter)->len; 
        }
        std::copy(data, data + remain_len, (*temp_write_buffer_iter)->data);
    }

    adjust_index(len, write_buffer_iter, write_index);
    set_write_index(write_buffer_iter, write_index);
    write_bytes(len);
    return *this;
}

char asio_buffer::peek_index(std::size_t index)
{
    assert(index <= readable_bytes_);

    buffer_iter read_buffer_iter = read_buffer_iter_;
    std::size_t read_index = read_index_;

    std::size_t read_block_size = (*read_buffer_iter)->len;
    std::size_t read_block_remain_len = read_block_size - read_index;
    if (read_block_remain_len > index) {
        return *((*read_buffer_iter)->data + read_index + index);
    } else {
        std::size_t remain_index = index - read_block_remain_len;
        read_buffer_iter++;
        read_block_size = (*read_buffer_iter)->len;
        while (read_block_size <= remain_index) {
            remain_index -= read_block_size;
            read_buffer_iter++;
            read_block_size = (*read_buffer_iter)->len;
        }
        return *((*read_buffer_iter)->data + remain_index); 
    }
}

std::unique_ptr<data_block> asio_buffer::peek(std::size_t len)
{
    assert(len <= readable_bytes_);
    if (len > readable_bytes_) len = readable_bytes_;
    std::unique_ptr<data_block> block(new data_block(len));

    buffer_iter read_buffer_iter = read_buffer_iter_;
    std::size_t read_index = read_index_;

    std::size_t read_block_size = (*read_buffer_iter)->len;
    std::size_t read_block_remain_len = read_block_size - read_index;
    if (read_block_remain_len > len) {
        std::copy((*read_buffer_iter)->data + read_index, 
            (*read_buffer_iter)->data + read_index + len, 
                block->data);
    } else {
        std::copy((*read_buffer_iter)->data + read_index, 
            (*read_buffer_iter)->data + read_index + read_block_remain_len, 
                block->data);
        char* data = block->data + read_block_remain_len;
        std::size_t remain_len = len - read_block_remain_len;
        read_buffer_iter++;
        read_block_size = (*read_buffer_iter)->len;
        while (read_block_size <= remain_len) {
            std::copy((*read_buffer_iter)->data, (*read_buffer_iter)->data + read_block_size, data);
            data += read_block_size;
            remain_len -= read_block_size;
            read_buffer_iter++;
            read_block_size = (*read_buffer_iter)->len;
        }
        std::copy((*read_buffer_iter)->data, (*read_buffer_iter)->data + remain_len, data);
    }
    return block;
}

void asio_buffer::retrieve(std::size_t len)
{
    assert(len <= readable_bytes_);
    if (len > readable_bytes_) len = readable_bytes_;

    adjust_index(len, read_buffer_iter_, read_index_);
    readable_bytes_ -= len;
    if (notify_behind_high_water_mask_ && readable_bytes_ < high_water_mask_) {
        notify_behind_high_water_mask_();
        notify_behind_high_water_mask_ = nullptr;
        high_water_mask_ = 0;
    }
}

void asio_buffer::has_written(std::size_t len)
{
    assert(len <= writable_bytes_);
    if (len > writable_bytes_) len = writable_bytes_;
    check_active();
    adjust_buffer(len);

    buffer_iter write_buffer_iter;
    std::size_t write_index;
    get_write_index(write_buffer_iter, write_index);
    adjust_index(len, write_buffer_iter, write_index);
    set_write_index(write_buffer_iter, write_index);
    write_bytes(len);
}

std::vector<asio::mutable_buffer>& asio_buffer::mutable_buffer()
{
    buffer_iter write_buffer_iter;
    std::size_t write_index;
    get_write_index(write_buffer_iter, write_index);
    mutable_buffer_.clear();
    mutable_buffer_.push_back(
        asio::buffer((*write_buffer_iter)->data + write_index, 
            (*write_buffer_iter)->len - write_index));
    buffer_iter iter = write_buffer_iter;
    iter++;
    for (; iter != buffer_.end(); ++iter) {
        mutable_buffer_.push_back(asio::buffer((*iter)->data, (*iter)->len));
    }
    return mutable_buffer_;
}

const std::vector<asio::const_buffer>& asio_buffer::const_buffer()
{
    buffer_iter write_buffer_iter;
    std::size_t write_index;
    get_write_index(write_buffer_iter, write_index);
    const_buffer_.clear();
    if (read_buffer_iter_ == write_buffer_iter) {
        const_buffer_.push_back(
            asio::buffer((*read_buffer_iter_)->data + read_index_, 
                write_index - read_index_));
    } else {
        const_buffer_.push_back(
            asio::buffer((*read_buffer_iter_)->data + read_index_, 
                (*read_buffer_iter_)->len - read_index_));
        buffer_iter iter = read_buffer_iter_;
        iter++;
        for (; iter != write_buffer_iter; ++iter) {
            const_buffer_.push_back(asio::buffer((*iter)->data, (*iter)->len));
        }
        const_buffer_.push_back(asio::buffer((*iter)->data, write_index));
    }
    return const_buffer_;
}

std::vector<write_data>& asio_buffer::write_buffer()
{
    buffer_iter write_buffer_iter;
    std::size_t write_index;
    get_write_index(write_buffer_iter, write_index);
    write_buffer_.clear();
    write_data first_buffer;
    first_buffer.data = (*write_buffer_iter)->data + write_index;
    first_buffer.len  = (*write_buffer_iter)->len - write_index;
    write_buffer_.push_back(first_buffer);
    buffer_iter iter = write_buffer_iter;
    iter++;
    for (; iter != buffer_.end(); ++iter) {
        write_data new_buffer;
        new_buffer.data = (*iter)->data;
        new_buffer.len  = (*iter)->len;
        write_buffer_.push_back(new_buffer);
    }
    return write_buffer_;
}

const std::vector<read_data>& asio_buffer::read_buffer()
{
    buffer_iter write_buffer_iter;
    std::size_t write_index;
    get_write_index(write_buffer_iter, write_index);
    read_buffer_.clear();
    if (read_buffer_iter_ == write_buffer_iter) {
        read_data new_buffer;
        new_buffer.data = (*read_buffer_iter_)->data + read_index_;
        new_buffer.len  = write_index - read_index_;
        read_buffer_.push_back(new_buffer);
    } else {
        read_data first_buffer;
        first_buffer.data = (*read_buffer_iter_)->data + read_index_;
        first_buffer.len  = (*read_buffer_iter_)->len - read_index_;
        read_buffer_.push_back(first_buffer);
        buffer_iter iter = read_buffer_iter_;
        iter++;
        for (; iter != write_buffer_iter; ++iter) {
            read_data new_buffer;
            new_buffer.data = (*iter)->data;
            new_buffer.len  = (*iter)->len;
            read_buffer_.push_back(new_buffer);
        }
        read_data last_buffer;
        last_buffer.data = (*iter)->data;
        last_buffer.len  = (*iter)->len;
        read_buffer_.push_back(last_buffer);
    }
    return read_buffer_;
}

} // namespace engine
