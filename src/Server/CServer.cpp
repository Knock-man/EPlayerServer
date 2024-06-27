#include "CServer.h"

CServer::CServer()
{
    m_server = NULL;
    m_business = NULL;
}
CServer::~CServer()
{
}
int CServer::Init(CBusiness *business, const Buffer &ip, short port)
{
    // 创建客户端业务子进程
    int ret = 0;
    if (business == NULL)
        return -1;
    m_business = business; // 缓存业务处理对象

    ret = m_process.SetEntryFunction(&CBusiness::BusinessProcess, m_business, &m_process); // 设置服务器入口函数
    if (ret != 0)
        return -2;
    ret = m_process.CreateSubProcess(); // 创建客户端处理子进程
    if (ret != 0)
        return -3;

    // 开启线程池 监听客户端连接请求
    m_pool.start(4);

    ret = m_epoll.Create(4);
    if (ret != 0)
        return -5;

    m_server = new CSocket(); // 创建服务器
    if (m_server == NULL)
        return -6;
    ret = m_server->Init(CSockParam(ip, port, SOCK_ISSERVER | SOCK_ISIP | SOCK_ISREUSE)); // 服务器初始化
    if (ret != 0)
        return -7;
    ret = m_epoll.Add(*m_server, EPollData((void *)m_server)); // 服务器连接监听挂到epoll上
    if (ret != 0)
        return -8;

    for (size_t i = 0; i < m_pool.Size(); i++) // 提交四个线程等待客户端连接
    {
        std::future<int> res1 = m_pool.submitTask([this]()
                                                  { return ThreadFunc(); });
    }
    return 0;
}

int CServer::Run() // 运行服务器
{
    while (m_server != NULL)
    {
        usleep(10);
    }
    return 0;
}

int CServer::Close() // 关闭服务器
{
    if (m_server)
    {
        CSocketBase *sock = m_server;
        m_server = NULL;
        m_epoll.Del(*sock);
        delete sock;
    }
    m_epoll.Close();
    m_process.SendFD(-1);
    m_pool.Close();
    return 0;
}

int CServer::ThreadFunc() // 服务器处理线程 等待客户端连接
{
    // TRACEI("epoll %d server %p", (int)m_epoll, m_server);
    //  printf("ip %s 端口%d 正在监听客户端连接\n", (char *)m_server->m_param.ip, (short)m_server->m_param.port);
    int ret = 0;
    EPEvents events(EVENT_SIZE);
    while ((m_epoll != -1) && (m_server != NULL))
    {
        // printf("[%s]<%s>:%d 服务器正在监听客户端连接\n", __FILE__, __FUNCTION__, __LINE__);
        ssize_t size = m_epoll.WaitEvents(events, 500);
        // printf("服务器正在监听客户端连接 超时 ret=%d\n", ret);
        if (size < 0)
        {
            printf("服务器连接监听失败\n");
            break;
        }

        if (size > 0)
        {
            // TRACEI("size=%d event %08X", size, events[0].events);
            for (size_t i = 0; i < size; i++)
            {
                if (events[i].events & EPOLLERR)
                {
                    break;
                }
                else if (events[i].events & EPOLLIN)
                {
                    if (m_server)
                    {
                        CSocketBase *pClient = NULL;
                        ret = m_server->Link(&pClient); // 连接
                        // printf("[%s]<%s>:%d %s\n", __FILE__, __FUNCTION__, __LINE__, "新客户端连接成功");
                        if (ret != 0)
                            continue;
                        ret = m_process.SendSocket(*pClient, *pClient); // 发生已连接客户端套接字给子进程
                        int s = *pClient;
                        delete pClient;
                        if (ret != 0)
                        {
                            TRACEE("send client %d failed", s);
                            continue;
                        }
                    }
                }
            }
        }
    }
    TRACEI("服务器终止");
    return 0;
}