#include <string>
#include <typeindex>

#include <engine/common/any.h>
#include <engine/common/data_block.h>
#include <engine/common/wfirst_rw_lock.h>
#include <engine/handler/context.h>
#include <engine/handler/abstract_handler.h>

using namespace engine;

void write(std::unique_ptr<any> msg)
{
    if (msg->type() == typeid(read_data)) {
        printf("write a read_data \n");
    } else if (msg->type() == typeid(write_data)) {
        printf("write a write_data \n");
    } else if (msg->type() == typeid(char)) {
        printf("write a char \n");
    } else if (msg->type() == typeid(int8_t)) {
        printf("write a int8_t \n");
    } else if (msg->type() == typeid(int16_t)) {
        printf("write a int16_t \n");
    } else if (msg->type() == typeid(int32_t)) {
        printf("write a int32_t \n");
    } else if (msg->type() == typeid(int64_t)) {
        printf("write a int64_t \n");
    } else if (msg->type() == typeid(uint8_t)) {
        printf("write a uint8_t \n");
    } else if (msg->type() == typeid(uint16_t)) {
        printf("write a uint16_t \n");
    } else if (msg->type() == typeid(uint32_t)) {
        printf("write a uint32_t \n");
    } else if (msg->type() == typeid(uint64_t)) {
        printf("write a uint64_t \n");
    } else if (msg->type() == typeid(float)) {
        printf("write a float \n");
    } else if (msg->type() == typeid(double)) {
        printf("write a double \n");
    } else if (msg->type() == typeid(long double)) {
        printf("write a long double \n");
    } else if (msg->type() == typeid(std::string)) {
        printf("write a std::string \n");
    } else if (msg->type() == typeid(std::string&)) {
        printf("write a std::string& \n");
    } else if (msg->type() == typeid(const std::string&)) {
        printf("write a const std::string& \n");
    } else {
        throw std::bad_cast();                
    }
}

class fate_any
{
public:
    fate_any()
        : a_(0)
    {}

    fate_any(int b)
        : a_(b)
    {}

    fate_any(const fate_any& that)
        : a_(that.a_)
    {}

    int get() { return a_; }

private:
    int a_;
};

int main()
{
    using namespace engine;

    any n;
    if (n.empty())
    {
        printf("n.is_null\n");
    }

    std::string s1 = "foo";

    any a1 = s1;

    if (!a1.empty())
    {
        printf("a1.not_null\n");
    }
    if (a1.type() == typeid(std::string))
    {
        printf("a1.is string\n");
    }
    if (a1.type() != typeid(int))
    {
        printf("a1.is not int\n");
    }

    any a2(a1);

    if (!a2.empty())
    {
        printf("a2.not_null\n");
    }
    if (a2.type() == typeid(std::string))
    {
        printf("a2.is string\n");
    }
    if (a2.type() != typeid(int))
    {
        printf("a2.is not int\n");
    }

    std::string s2 = any_cast<std::string>(a2);

    if (s1 == s2)
    {
        printf("s1 == s2\n");
    }
    if (s1.compare(s2) == 0)
    {
        printf("s1 compare s2 equal\n");
    }

    const unsigned write_entered_ = 1U << (sizeof(unsigned)*8 - 1);
    printf("write_entered_ = %u\n", write_entered_); 

    wfirst_rw_lock lock;
    lock.read_lock();
    printf("test rw_lock read lock\n");
    lock.read_unlock(); 

    lock.write_lock();
    printf("test rw_lock write lock\n");
    lock.write_unlock();

    {
        auto read_guard = lock.read_guard();
        printf("test rw_lock read guard\n");
    }

    {
        auto write_guard = lock.write_guard();
        printf("test rw_lock write guard\n");
    }

    read_data data;
    auto response = new any(data);
    write(std::unique_ptr<any>(response));

    write_data wdata;
    auto wresponse = new any(wdata);
    write(std::unique_ptr<any>(wresponse));

    char a;
    auto char_response = new any(a);
    write(std::unique_ptr<any>(char_response));

    std::string s = "3308756894 fool jokers";
    auto s_response = new any(s);
    write(std::unique_ptr<any>(s_response));

    fate_any fate_a(1);

    fate_any fate_b(fate_a);

    printf("fate_any %d ", fate_b.get());

    std::string sb = "1234567890";
    const std::string& sb1 = sb;
    char c = 'a';

    if (std::type_index(typeid(sb1)) == std::type_index(typeid(c)))
    {
        printf("enter there \n");
    }

    return 0;
}
