#ifndef ENGINE_COMMON_ANY_H
#define ENGINE_COMMON_ANY_H

#include <memory>
#include <typeinfo>
#include <type_traits>

namespace engine
{

class any
{
public:
    any() noexcept
        : content(0)
    {
    }

    template<typename ValueType>
    any(const ValueType& value)
        : content(new holder<typename std::remove_cv<
                typename std::decay<const ValueType>
                ::type>::type>(value))
    {
    }

    any(const any& other)
        : content(other.content ? other.content->clone() : 0)
    {
    }

    any(any&& other) noexcept
        : content(other.content)
    {
        other.content = 0;
    }

    template<typename ValueType
        , typename std::enable_if<
            !std::is_same<any&,ValueType>::value &&
            !std::is_const<ValueType>::value, bool>
            ::type = true>
    any(ValueType&& value)
        : content(new holder<typename std::decay<ValueType>
                ::type>(static_cast<ValueType&&>(value)))
    {
    }

    ~any() noexcept
    {
        delete content;
    }

    any& swap(any& rhs) noexcept
    {
        std::swap(content, rhs.content);
        return *this;
    }

    template<typename ValueType>
    any& operator=(const ValueType& rhs)
    {
        any(rhs).swap(*this);
        return *this;
    }

    any& operator=(const any& rhs)
    {
        any(rhs).swap(*this);
        return *this;
    }

    any& operator=(any&& rhs) noexcept
    {
        rhs.swap(*this);
        any().swap(rhs);
        return *this;
    }

    template<class ValueType>
    any& operator=(ValueType&& rhs)
    {
        any(static_cast<ValueType&&>(rhs)).swap(*this);
        return *this;
    }

    bool empty() const noexcept
    {
        return !content;
    }

    void clear() noexcept
    {
        any().swap(*this);
    }

    const std::type_info& type() const noexcept
    {
        return content ? content->type() : typeid(void);
    }
private:
    class placeholder
    {
    public:
        virtual ~placeholder()
        {
        }

        virtual const std::type_info& type() const noexcept = 0;

        virtual placeholder* clone() const = 0;
    }; // class placeholder

    template<typename ValueType>
    class holder : public placeholder
    {
    public:
        holder& operator=(const holder&) = delete;

        holder(const ValueType& value)
            : held(value)
        {
        }

        holder(ValueType&& value)
            : held(static_cast<ValueType&&>(value))
        {
        }

        virtual const std::type_info& type() const noexcept
        {
            return typeid(ValueType);
        }

        virtual placeholder* clone() const
        {
            return new holder(held);
        }

        ValueType held;
    }; // class holder

    template<typename ValueType>
    friend inline ValueType* any_cast(any*) noexcept;
    
    placeholder* content;
}; // class any

inline void swap(any& lhs, any& rhs) noexcept
{
    lhs.swap(rhs);
}

class bad_any_cast : public std::bad_cast
{
public:
    virtual const char* what() const noexcept
    {
        return "bad_any_cast: "
            "failed conversion using any_cast";                
    }
}; // class bad_any_cast

template<typename Tp>
using noncv = typename std::remove_cv<Tp>::type;

template<typename Tp>
using nonref = typename std::remove_reference<Tp>::type;

template<typename Tp>
using noncvref = noncv<nonref<Tp>>;

template<typename ValueType>
inline ValueType* any_cast(any* operand) noexcept
{
    return operand && operand->type() == typeid(ValueType)
        ? std::addressof(static_cast<any::holder<
                noncv<ValueType>>*>(operand->content)->held) 
        : 0;
}

template<typename ValueType>
inline const ValueType* any_cast(const any* operand) noexcept
{
    return any_cast<ValueType>(const_cast<any*>(operand));
}

template<typename Tp>
constexpr bool is_valid_cast()
{
    return std::is_reference<Tp>::value ||
        std::is_copy_constructible<Tp>::value;
}

template<typename ValueType>
inline ValueType any_cast(any& operand)
{
    static_assert(is_valid_cast<ValueType>(), 
            "Template argument must be "
            "a reference or CopyConstructible type");
    
    auto p = any_cast<noncvref<ValueType>>(&operand);
    if (p) {
        return static_cast<ValueType>(*p);
    }
    
    throw bad_any_cast();
}

template<typename ValueType>
inline ValueType any_cast(const any& operand)
{
    return any_cast<const nonref<ValueType>&>(
            const_cast<any&>(operand));
}

template<typename ValueType,
    typename std::enable_if<
        std::is_rvalue_reference<ValueType&&>::value
        || std::is_const<nonref<ValueType>>::value, bool>
        ::type = true>
inline ValueType any_cast(any&& operand)
{
    return any_cast<ValueType>(operand);
}

} // namespace engine

#endif // ENGINE_COMMON_ANY_H

