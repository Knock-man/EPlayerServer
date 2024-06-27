/*
进程控制类
功能：创建子进程，进程通信，创建守护进程
创建进程:fork()
进程通信：使用socketpair()创建一对已连接的套接字对，向struct msghdr 结构体填充消息，向struct cmsghdr辅助信息结构体填充文件描述符，通过sendmsg()和recvmsg()进行通信
守护进程：创建子进程，主进程退出，子进程设为会话组长→创建孙进程，子进程退出→孙进程关闭标准输入输出和标准错误文件描述符，umask(0)设置使用最大权限创建文件，屏蔽子进程退出信号

*/
#pragma once
#include "function.h"
#include <netinet/in.h>

// 进程类
class CProcess
{
public:
    CProcess() : m_func(NULL)
    {
        memset(sockfd, 0, sizeof(sockfd)); // 管道初始化，用来套接字通信
    }
    ~CProcess()
    {
        if (m_func != NULL)
        {
            delete m_func; // 删除函数对象
            m_func = NULL;
        }
    }

    // 设置子进程入口函数
    template <typename _FUNCTION_, typename... _ARGS_> // 可变参模板
    int SetEntryFunction(_FUNCTION_ func, _ARGS_... args)
    {
        m_func = new CFunction<_FUNCTION_, _ARGS_...>(func, args...); // 创建函数对象
        return 0;
    }

    // 创建子进程
    int CreateSubProcess();

    // 发送文件描述符
    int SendFD(int fd);

    // 接收文件描述符 存在fd中
    int RecvFD(int &fd);

    // 发送文件描述符
    int SendSocket(int fd, const sockaddr_in *addrin);

    // 接收文件描述符 存在fd中
    int RecvSocket(int &fd, sockaddr_in *addrin);

    // 转换到守护进程
    static int SwitchDeamon();

private:
    CFunctionBase *m_func; // 多态 指向函数对象
    pid_t m_pid;           // 子进程id
    int sockfd[2];         // 通信套接字对
};