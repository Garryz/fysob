#ifndef ENGINE_COMMON_WFIRST_RW_LOCK_H
#define ENGINE_COMMON_WFIRST_RW_LOCK_H

#include <condition_variable>
#include <mutex>

#include <engine/common/raii.h>

namespace engine
{

class wfirst_rw_lock
{
public:
    wfirst_rw_lock(const wfirst_rw_lock&) = delete;
    wfirst_rw_lock& operator=(const wfirst_rw_lock&) = delete;
    wfirst_rw_lock() = default;
    ~wfirst_rw_lock() = default;

    void read_lock()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_r.wait(lock, [=]()->bool {return write_cnt_ == 0;});
        ++read_cnt_;
    }

    void read_unlock()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (--read_cnt_ == 0 && write_cnt_ > 0) {
            cond_w.notify_one();
        }
    }
    
    void write_lock()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        ++write_cnt_;
        cond_w.wait(lock, [=]()->bool {return read_cnt_ == 0 && !write_flag_;});
        write_flag_ = true;
    }

    void write_unlock()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (--write_cnt_ == 0) {
            cond_r.notify_all();
        }
        else {
            cond_w.notify_one();
        }
        write_flag_ = false;
    }

    raii read_guard() const noexcept
    {
        return make_raii(*this, 
                &wfirst_rw_lock::read_unlock, 
                &wfirst_rw_lock::read_lock);
    }

    raii write_guard() const noexcept
    {
        return make_raii(*this,
                &wfirst_rw_lock::write_unlock,
                &wfirst_rw_lock::write_lock);
    }
private:
    volatile std::size_t    read_cnt_{0};
    volatile std::size_t    write_cnt_{0};
    volatile bool           write_flag_{false};
    std::mutex              mutex_;
    std::condition_variable cond_r;
    std::condition_variable cond_w;
}; // class wfirst_rw_lock

} // namespace engine

#endif // ENGINE_COMMON_WFIRST_RW_LOCK_H
