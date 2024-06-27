#pragma once
#include <unistd.h>
#include <sys/epoll.h>
#include <vector>
#include <errno.h>
#include <sys/signal.h>
#include <memory.h>
#include <stdio.h>
#define EVENT_SIZE 128

using EPEvents = std::vector<epoll_event>; // events数组
// epoll_event events[EVENT_SIZE];

/*
typedef union epoll_data
{
  void *ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
} epoll_data_t;

struct epoll_event
{
  uint32_t events;//事件类型
  epoll_data_t data;
}
*/

// epoll_data
class EPollData // epoll_data结构体中的成员封装
{
public:
    // 初始化epoll_data_t
    EPollData()
    {
        m_data.u64 = 0; // 联合体 相当于整个结构体初始化
    }
    EPollData(void *ptr) { m_data.ptr = ptr; }
    explicit EPollData(int fd) { m_data.fd = fd; }
    explicit EPollData(uint32_t u32) { m_data.u32 = u32; }
    explicit EPollData(uint64_t u64) { m_data.u64 = u64; }
    EPollData(const EPollData &data) { m_data.u64 = data.m_data.u64; }

public:
    // 重载运算
    EPollData &operator=(const EPollData &data)
    {
        if (this != &data)
        {
            m_data.u64 = data.m_data.u64; // 联合体
        }
        return *this;
    }

    EPollData &operator=(void *data)
    {
        m_data.ptr = data;
        return *this;
    }

    EPollData &operator=(int data)
    {
        m_data.fd = data;
        return *this;
    }
    EPollData &operator=(uint32_t data)
    {
        m_data.u32 = data;
        return *this;
    }
    EPollData &operator=(uint64_t data)
    {
        m_data.u64 = data;
        return *this;
    }

    operator epoll_data_t() { return m_data; }                // epoll_data_t epolldata = EPollData;
    operator epoll_data_t() const { return m_data; }          // epoll_data_t epolldata = const EPollData;
    operator epoll_data_t *() { return &m_data; }             // epoll_data_t* epolldata = EPollData;
    operator const epoll_data_t *() const { return &m_data; } // const epoll_data_t* epolldata = EPollData;

private:
    epoll_data_t m_data;
};

// 封装epoll
class CEpoll
{
public:
    CEpoll()
    {
        m_epoll = -1;
    }
    ~CEpoll()
    {
        Close();
    }

    // 禁止赋值构造拷贝构造
    CEpoll(const CEpoll &) = delete;
    CEpoll &operator=(const CEpoll &) = delete;

    operator int() const { return m_epoll; };

public:
    int Create(unsigned count);

    ssize_t WaitEvents(EPEvents &events, int timeout = 10);

    int Add(int fd, const EPollData &data = EPollData((void *)0), uint32_t event = EPOLLIN | EPOLLERR);

    int Modify(int fd, uint32_t event, const EPollData &data = EPollData((void *)0));

    int Del(int fd);

    void Close();

private:
    int m_epoll; // 根节点
};