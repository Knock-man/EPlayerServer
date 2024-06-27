#include "Logger.h"

LogInfo::LogInfo(
    const char *file,
    int line,
    const char *func,
    pid_t pid,
    pthread_t tid,
    int level,
    const char *fmt, ...)
{
    const char sLevel[][8] = {
        "INFO", "DEBUG", "WARNING", "ERROR", "FATAL"};
    char *buf = NULL;
    bAuto = false;

    int count = asprintf(&buf, "%s(%d):[%s][%s]<%d-%ld>(%s) ",
                         file, line, sLevel[level], (char *)CLoggerServer::GetTimeStr(), pid, tid, func);
    if (count > 0)
    {
        m_buf = buf;
        free(buf);
    }
    else
    {
        return;
    }

    va_list ap;
    va_start(ap, fmt);

    count = vasprintf(&buf, fmt, ap);
    if (count >= 0)
    {
        m_buf += buf;
        free(buf);
    }
    m_buf += "\n";
    va_end(ap);
};

LogInfo::LogInfo(
    const char *file,
    int line,
    const char *func,
    pid_t pid,
    pthread_t tid,
    int level)
{ // 自己主动发的 流式的日志
    bAuto = true;
    const char sLevel[][8] = {
        "INFO", "DEBUG", "WARNING", "ERROR", "FATAL"};
    char *buf = NULL;
    int count = asprintf(&buf, "%s(%d):[%s][%s]<%d-%ld>(%s) ",
                         file, line, sLevel[level], (char *)CLoggerServer::GetTimeStr(), pid, tid, func);
    if (count > 0)
    {
        m_buf = buf;
        free(buf);
    }
}

LogInfo::LogInfo(const char *file,
                 int line,
                 const char *func,
                 pid_t pid,
                 pthread_t tid,
                 int level,
                 void *pData,
                 size_t nSize)
{

    const char sLevel[][8] = {
        "INFO", "DEBUG", "WARNING", "ERROR", "FATAL"};
    char *buf = NULL;
    bAuto = false;

    int count = asprintf(&buf, "%s(%d):[%s][%s]<%d-%ld>(%s) \n",
                         file, line, sLevel[level], (char *)CLoggerServer::GetTimeStr(), pid, tid, func);
    if (count > 0)
    {
        m_buf = buf;
        free(buf);
    }
    else
    {
        return;
    }

    Buffer out;
    size_t i = 0;
    char *Data = (char *)pData;
    for (i; i < nSize; i++)
    {
        char buf[16] = "";
        snprintf(buf, sizeof(buf), "%02X ", Data[i] & 0xFF);
        m_buf += buf;
        if (0 == (i + 1) % 16)
        {
            m_buf += "\t";
            char buf[17] = "";
            memcpy(buf, Data + i - 15, 16);
            for (int j = 0; j < 16; j++)
            {
                if ((buf[j] < 32) && (buf[j] >= 0))
                    buf[j] = '.';
            }
            m_buf += '\n';
        }
    }

    // 处理尾巴
    size_t k = i % 16;
    if (k != 0)
    {
        for (size_t j = 0; j < 16 - k; j++)
        {
            m_buf += "   ";
        }
        m_buf += "\t";
        for (size_t j = i - k; j <= i; j++)
        {
            if ((Data[j] & 0xFF) > 31 && ((Data[j] & 0xFF) < 0x7F))
            {
                m_buf += Data[i];
            }
            else
            {
                m_buf += '.';
            }
        }
        m_buf += '\n';
    }
}

LogInfo::~LogInfo() // 流的形式在析构的时候自动写日志
{
    if (bAuto)
    {
        m_buf += "\n";
        CLoggerServer::Trace(*this);
    }
}

// 日志服务器
// 日志服务器的启动
// 创建日志目录，创建日志文件，创建epoll，初始化网络通信socket 挂到epoll上  开启线程
int CLoggerServer::Start()
{
    // printf("%s(%d):[%s]\n", __FILE__, __LINE__, __FUNCTION__);
    if (m_server != NULL)
        return -1;
    if (access("./../log", W_OK | R_OK) != 0) // 日志文件是否正常
    {
        mkdir("./../log", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    }

    m_file = fopen(m_path, "w+"); // 打开日志文件
    if (m_file == NULL)
        return -2;

    // 创建epoll
    int ret = m_epoll.Create(1);
    if (ret != 0)
        return -3;

    // 创建网络通信socket
    m_server = new CSocket();
    if (m_server == NULL)
    {
        Close();
        return -4;
    }
    CSockParam param("./../log/server.sock", (int)SOCK_ISSERVER | SOCK_ISREUSE); // 本地通信套接字文件地址
    ret = m_server->Init(param);                                                 // 初始化服务器
    if (ret != 0)
    {
        Close();
        return -5;
    }

    // 挂到树上
    ret = m_epoll.Add(*m_server, EPollData((void *)m_server), EPOLLIN | EPOLLERR);
    if (ret != 0)
    {
        Close();
        return -6;
    }
    // printf("%s(%d):[%s]\n", __FILE__, __LINE__, __FUNCTION__);
    //  开启线程
    ret = m_thread.Start();
    if (ret != 0)
    {
        Close();
        return -7;
    }
    return 0;
}

int CLoggerServer::Close()
{
    if (m_server != NULL)
    {
        CSocketBase *p = m_server;
        m_server = NULL;
        delete p;
    }
    m_epoll.Close();
    m_thread.Stop();
    return 0;
}

// 日志客户端 向日志服务器发送日志 给其他非日志进程和线程使用的
void CLoggerServer::Trace(const LogInfo &info)
{
    int ret = 0;
    static thread_local CSocket client;
    if (client == -1)
    {

        ret = client.Init(CSockParam("./../log/server.sock", 0)); // 初始化客户端
        if (ret != 0)
        {
            return;
        }
        ret = client.Link(); // 连接服务器
        // printf("%s(%d):[%s]ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    }
    ret = client.Send(info); // 向服务器发送数据
    // printf("%s(%d):[%s]ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
}

// 获得时间
Buffer CLoggerServer::GetTimeStr()
{
    Buffer result(108);
    // timeb tmb;
    // ftime(&tmb);
    timeval tmb;
    gettimeofday(&tmb, NULL);
    tm *pTm = localtime(&tmb.tv_sec);
    int nSize = snprintf(result, result.size(),
                         "%04d-%02d-%02d_%02d-%02d-%02d.%03ld",
                         pTm->tm_year + 1900, pTm->tm_mon + 1, pTm->tm_mday,
                         pTm->tm_hour, pTm->tm_min, pTm->tm_sec,
                         tmb.tv_usec / 1000); // 将微秒转换为毫秒
    result.resize(nSize);
    return result;
}

// 写日志
void CLoggerServer::WriteLog(const Buffer &data)
{
    if (m_file != NULL)
    {
        FILE *pfile = m_file;
        fwrite((char *)data, 1, data.size(), pfile);
        fflush(pfile);
#ifdef _DEBUG
        printf("info=%s\n", (char *)data);
#endif
    }
}

// 日志服务器线程函数 负责epoll_wait 接收客户端连接 接收通知处理日志
int CLoggerServer::ThreadFunc()
{
    // printf("%s(%d):[%s]\n", __FILE__, __LINE__, __FUNCTION__);
    EPEvents events(EVENT_SIZE);
    std::map<int, CSocketBase *> mapClients; // 储存所有和服务器连接的客户端
    while (m_thread.isValid() && (m_epoll != -1) && (m_server != NULL))
    {
        // printf("%s(%d):[%s]\n", __FILE__, __LINE__, __FUNCTION__);
        ssize_t ret = m_epoll.WaitEvents(events, 300); // epoll_wait()
        // printf("%s(%d):[%s]ret=%ld\n", __FILE__, __LINE__, __FUNCTION__, ret);
        if (ret < 0)
        {
            // printf("%s(%d):[%s]ret=%ld\n", __FILE__, __LINE__, __FUNCTION__, ret);
            break;
        }
        if (ret > 0)
        {
            // printf("%s(%d):[%s]ret=%ld\n", __FILE__, __LINE__, __FUNCTION__, ret);
            ssize_t i = 0;
            for (; i < ret; i++) // 遍历所有响应事件
            {
                if (events[i].events & EPOLLERR) // 错误事件
                {
                    printf("%s(%d):[%s]\n", __FILE__, __LINE__, __FUNCTION__);
                    break;
                }
                else if (events[i].events & EPOLLIN) // 读事件
                {
                    // printf("%s(%d):[%s]m_server=%p ptr=%p\n", __FILE__, __LINE__, __FUNCTION__, m_server, events[i].data.ptr);
                    if (events[i].data.ptr == m_server) // 连接事件
                    {
                        CSocketBase *pClient = NULL;      // 接收客户端连接
                        int r = m_server->Link(&pClient); // 客户端连接 客户端消息pClient传出
                        if (r < 0)
                        {
                            printf("%s(%d):[%s]r=%d\n", __FILE__, __LINE__, __FUNCTION__, r);
                            continue;
                        }
                        // printf("%s(%d):[%s]r=%d %s\n", __FILE__, __LINE__, __FUNCTION__, r, "连接日志服务器成功");
                        r = m_epoll.Add(*pClient, EPollData((void *)pClient), EPOLLIN | EPOLLERR); // 客户端挂到树上
                        if (r < 0)
                        {
                            printf("%s(%d):[%s]r=%d\n", __FILE__, __LINE__, __FUNCTION__, r);
                            delete pClient;
                            continue;
                        }
                        // printf("%s(%d):[%s]r=%d\n", __FILE__, __LINE__, __FUNCTION__, r);

                        // 添加进映射表
                        auto it = mapClients.find(*pClient); // 将服务器连接的客户端储存
                        // printf("%s(%d):[%s]\n", __FILE__, __LINE__, __FUNCTION__);
                        if (it != mapClients.end() && it->second != nullptr) // 原来映射表存在
                        {
                            // printf("%s(%d):[%s]\n", __FILE__, __LINE__, __FUNCTION__);
                            delete it->second;
                            // printf("%s(%d):[%s]\n", __FILE__, __LINE__, __FUNCTION__);
                        }
                        // printf("%s(%d):[%s]\n", __FILE__, __LINE__, __FUNCTION__);
                        mapClients[*pClient] = pClient; // 添加进映射表
                        // printf("%s(%d):[%s]\n", __FILE__, __LINE__, __FUNCTION__);
                    }
                    else // 通信事件
                    {
                        // printf("%s(%d):[%s]ret=%ld\n", __FILE__, __LINE__, __FUNCTION__, ret);
                        CSocketBase *pClient = (CSocketBase *)events[i].data.ptr;
                        // printf("%s(%d):[%s]ret=%ld\n", __FILE__, __LINE__, __FUNCTION__, ret);
                        if (pClient != NULL)
                        {
                            // printf("%s(%d):[%s]ret=%ld\n", __FILE__, __LINE__, __FUNCTION__, ret);
                            Buffer data(1024 * 1024);    // 初始化Buffer大小
                            int r = pClient->Recv(data); // 服务器接收数据
                                                         // printf("%s(%d):[%s]ret=%d\n", __FILE__, __LINE__, __FUNCTION__, r);
                            if (r <= 0)
                            {
                                printf("%s(%d):[%s]r=%d\n", __FILE__, __LINE__, __FUNCTION__, r);
                                delete pClient;
                                mapClients[*pClient] = NULL;
                            }
                            else
                            {
                                // printf("%s(%d):[%s]r=%d %s\n", __FILE__, __LINE__, __FUNCTION__, r, "日志服务器接收到一次通信");
                                WriteLog(data); // 将接收到的数据写入文件中
                            }
                        }
                    }
                }
            }
            if (i != ret) // 事件没有处理结束
            {
                printf("%s(%d):[%s] %s\n", __FILE__, __LINE__, __FUNCTION__, "日志服务器事件没有处理结束");
                break;
            }
        }
    }

    // 连接出错
    for (auto it = mapClients.begin(); it != mapClients.end(); it++)
    {
        if (it->second)
        {
            delete it->second;
        }
    }
    mapClients.clear();
    return 0;
}
