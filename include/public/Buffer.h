/*
字符串类
功能：继承string类，对运算符进行重载，提供多个构造，使得字符串操作更加简化
对运算符重载之后，对于各种不同参数类型的函数传参，不需要显示类型转换即可传递
*/
#pragma once
#include <string>
#include <string.h>

// 继承string类
class Buffer : public std::string
{
public:
    Buffer() : std::string() {}
    Buffer(size_t size) : std::string()
    {
        resize(size);
    };
    Buffer(const std::string &str) : std::string(str) {}

    Buffer(const char *str) : std::string(str) {}

    Buffer(const char *str, size_t length) : std::string(str)
    {
        resize(length);
        memcpy((char *)c_str(), str, length);
    }
    Buffer(const char *begin, const char *end) : std::string()
    {
        long int len = end - begin;
        if (len > 0)
        {
            resize(len);
            memcpy((char *)c_str(), begin, len);
        }
    }

    operator char *()
    {
        return (char *)c_str();
    }
    operator char *() const
    {
        return (char *)c_str();
    }
    operator const char *() const
    {
        return c_str();
    }
    operator unsigned char *() const
    {
        return (unsigned char *)c_str();
    }
    operator const void *() const
    {
        return (const void *)c_str();
    }
};