#pragma once
#include "Socket.h"
#include "Epoll.h"
#include "process.h"
#include "threadpool.h"
#include "EdoyunPlayerServer.h"
#include "Logger.h"
#include <future>
#define PORT 9527
#define IP "0.0.0.0"
class CBusiness;
// 服务器类
class CServer
{

public:
    CServer();
    ~CServer();
    CServer(const CServer &) = delete;
    CServer &operator=(const CServer &) = delete;

    int Init(CBusiness *business, const Buffer &ip = IP, short port = PORT);
    int Run();
    int Close();

private:
    int ThreadFunc(); // 服务器线程函数

private:
    ThreadPool m_pool;     // 线程池
    CSocketBase *m_server; // 服务器
    CEpoll m_epoll;
    CProcess m_process;    // 服务器进程
    CBusiness *m_business; // 业务模块
};
