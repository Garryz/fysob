#ifndef ENGINE_COMMON_RAII_H
#define ENGINE_COMMON_RAII_H

#include <cassert>
#include <functional>
#include <type_traits>

namespace engine
{

template<typename T>
struct no_const {
    using type=typename std::conditional<std::is_const<T>::value, 
                typename std::remove_const<T>::type, T>::type;
};

class raii 
{
public:
    using fun_type = std::function<void()>;

    explicit raii(fun_type release, fun_type acquire = [] {}, bool default_com = true) noexcept 
        : release_(release)
        , commit_(default_com)
    {
        acquire();
    }

    ~raii() noexcept
    {
        if (commit_) {
            release_();
        }
    }

    raii(raii&& rv) noexcept
        : release_(rv.release_)
        , commit_(rv.commit_)
    {
        rv.commit_ = false;
    }

    raii(const raii&) = delete;
    raii& operator=(const raii&) = delete;

    raii& commit(bool c = true) noexcept
    {
        commit_ = c;
        return *this;
    }
protected:
    std::function<void()> release_;
private:
    bool commit_;
}; // class raii

template<typename T>
class raii_var
{
public:
    using _Self     = raii_var<T>;
    using acq_type  = std::function<T()>;
    using rel_type  = std::function<void(T&)>;

    explicit raii_var(acq_type acquire, rel_type release) noexcept
        : resource_(acquire())
        , release_(release) 
    {    
    }

    raii_var(raii_var&& rv)
        : resource_(std::move(rv.resource_))
        , release_(std::move(rv.release_))
    {
        rv.commit_ = false;
    }

    ~raii_var() noexcept
    {
        if (commit_) {
            release_(resource_);
        }
    }

    _Self& commit(bool c = true) noexcept
    {
        commit_ = c;
        return *this;
    }

    T& get() noexcept
    {
        return resource_;
    }

    T& operator*() noexcept
    {
        return get();
    }

    template<typename _T = T>
    typename std::enable_if<std::is_pointer<_T>::value, _T>::type operator->() const noexcept
    {
        return resource_;
    }

    template<typename _T = T>
    typename std::enable_if<std::is_class<_T>::value, _T*>::type operator->() const noexcept
    {
        return std::addressof(resource_);
    }
private:
    bool        commit_ = true;
    T           resource_;
    rel_type    release_;
}; // class raii_var

template<typename RES, typename M_REL, typename M_ACQ>
raii make_raii(RES& res, M_REL rel, M_ACQ acq, bool default_com = true) noexcept
{
    static_assert(std::is_class<RES>::value, "RES is not a class or struct type.");
    static_assert(std::is_member_function_pointer<M_REL>::value, "M_REL is not a member function.");
    static_assert(std::is_member_function_pointer<M_ACQ>::value, "M_ACQ is not a member function.");
    assert(nullptr != rel && nullptr != acq);
    auto p_res = std::addressof(const_cast<typename no_const<RES>::type&>(res));
    return raii(std::bind(rel, p_res), std::bind(acq, p_res), default_com);
}

template<typename RES, typename M_REL>
raii make_raii(RES& res, M_REL rel, bool default_com = true) noexcept
{
    static_assert(std::is_class<RES>::value, "RES is not a class or struct type.");
    static_assert(std::is_member_function_pointer<M_REL>::value, "M_REL is not a member function.");
    assert(nullptr != rel);
    auto p_res = std::addressof(const_cast<typename no_const<RES>::type&>(res));
    return raii(std::bind(rel, p_res), [] {}, default_com);
}

} // namespace engine

#endif // ENGINE_COMMON_RAII_H
