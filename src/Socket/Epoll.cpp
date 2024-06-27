#include "Epoll.h"
#include <algorithm>

// 创建epoll
int CEpoll::Create(unsigned count)
{
    if (m_epoll != -1)
        return -1;
    m_epoll = epoll_create(count); // 创建根节点
    if (m_epoll == -1)
        return -2;
    return 0;
}

// epoll_wait
ssize_t CEpoll::WaitEvents(EPEvents &events, int timeout)
{
    if (m_epoll == -1)
        return -1;
    EPEvents evs(EVENT_SIZE); // epoll_event[event_size] 初始化临时事件数组
    // epoll_wait()的第二个参数是一个传出参数
    int ret = epoll_wait(m_epoll, evs.data(), (int)evs.size(), timeout); // evs传出发生的事件

    // printf("%s(%d):[%s]ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    //     vector的data()运算 返回数组首元素的指针
    if (ret == -1)
    {
        if ((errno == EINTR) || (errno == EAGAIN)) // EINTR系统被信号中断  EAGAIN超时没有事件发生
        {
            printf("%s(%d):[%s]ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
            return 0;
        }
        return -2;
    }
    // printf("%s(%d):[%s]ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    if (ret > (int)evs.size()) // 临时数组开辟不够
    {
        evs.resize(ret);
    }

    //  拷贝到epoll_event[]进行传出
    memcpy(events.data(), evs.data(), sizeof(epoll_event) * ret);

    return ret; // 返回事件发生的个数
}

// 添加事件
int CEpoll::Add(int fd, const EPollData &data, uint32_t event)
{
    if (m_epoll == -1)
        return -1;
    /*
    struct epoll_event
    {
        uint32_t events;
        epoll_data_t data;
    }
    */
    epoll_event ev = {event, data};                       // EPollData有运算符重载
    int ret = epoll_ctl(m_epoll, EPOLL_CTL_ADD, fd, &ev); // 挂到树上
    if (ret == -1)
        return -2;
    return 0;
}

// 修改事件
int CEpoll::Modify(int fd, uint32_t event, const EPollData &data)
{
    if (m_epoll == -1)
        return -1;
    epoll_event ev = {event, data};
    int ret = epoll_ctl(m_epoll, EPOLL_CTL_MOD, fd, &ev);
    if (ret == -1)
        return -2;
    return 0;
}

// 删除监听描述符
int CEpoll::Del(int fd)
{
    if (m_epoll == -1)
        return -1;
    int ret = epoll_ctl(m_epoll, EPOLL_CTL_DEL, fd, NULL);
    if (ret == -1)
        return -2;
    return 0;
}

// 关闭epoll
void CEpoll::Close()
{
    if (m_epoll != -1)
    {
        int fd = m_epoll;
        m_epoll = -1;
        close(fd);
    }
}