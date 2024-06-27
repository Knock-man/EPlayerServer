
/*
函数对象绑定
将函数名和函数参数进行绑定，在合适的时机直接调用 函数名()，即可调用函数
示例：
    int add(int a,int b){return a+b;}
    typedef int(*_FUNCTION_)(int,int);
    CFunction<_FUNCTION_,int,int> func(add,1,2);
    func();
*/
#pragma once
#include <functional>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <memory.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>

class CSocketBase;
class Buffer;

class CFunctionBase // 函数对象抽象类
{
public:
    CFunctionBase(){};
    virtual ~CFunctionBase(){};
    virtual int operator()() { return -1; };
    virtual int operator()(CSocketBase *) { return -1; } // CFunctionBase(CSocketBase *) 会调用
    virtual int operator()(CSocketBase *, const Buffer &) { return -1; };
};

template <typename _FUNCTION_, typename... _ARGS_>
class CFunction : public CFunctionBase // 函数对象类
{
public:
    CFunction(_FUNCTION_ func, _ARGS_... args)
        : m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...) // 绑定函数对象
    {
    }

    virtual ~CFunction(){};
    virtual int operator()() override // 括号运算符重载
    {
        return m_binder(); // 返回绑定对象
    }

private:
    typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder; // 绑定器
};
