#include "EdoyunPlayerServer.h"

CEdoyunPlayerServer::CEdoyunPlayerServer(unsigned count) : CBusiness()
{
    m_count = count;
}
CEdoyunPlayerServer::~CEdoyunPlayerServer()
{
    if (m_db != NULL) // 释放数据库
    {
        CDatabaseClient *db = m_db;
        m_db = NULL;
        db->Close();
        delete db;
    }
    m_epoll.Close();
    m_pool.Close();
    for (auto it : m_mapClients)
    {
        if (it.second)
        {
            delete it.second;
        }
    }
    m_mapClients.clear();
}

// 业务处理子进程
int CEdoyunPlayerServer::BusinessProcess(CProcess *proc)
{
    // printf("[%s]<%s>:%d %s\n", __FILE__, __FUNCTION__, __LINE__, "客户端子进程启动");
    using namespace std::placeholders;
    int ret = 0;

    // 连接数据库
    m_db = new CMysqlClient();
    if (m_db == NULL)
    {
        TRACEE("no more memory!");
        return -1;
    }
    // printf("[%s]<%s>:%d %s\n", __FILE__, __FUNCTION__, __LINE__, "创建数据库对象");
    KeyValue args;
    args["host"] = "127.0.0.1"; // 数据库创建参数
    args["user"] = "xbj";
    args["password"] = "xbj";
    args["port"] = "3306";
    args["db"] = "edoyun";
    // printf("[%s]<%s>:%d %s\n", __FILE__, __FUNCTION__, __LINE__, "开始连接");
    ret = m_db->Connect(args);
    // printf("[%s]<%s>:%d %s ret=%d\n", __FILE__, __FUNCTION__, __LINE__, "连接数据库", ret);
    ERR_RETURN(ret, -2);
    // 创建表
    edoyunLogin_user_mysql user;
    ret = m_db->Exec(user.Create());

    ERR_RETURN(ret, -3);
    // printf("[%s]<%s>:%d %s\n", __FILE__, __FUNCTION__, __LINE__, "数据库初始化成功");
    //   设置连接通信回调函数
    ret = setConnectCallback(&CEdoyunPlayerServer::Connected, this, _1); // 设置连接回调
    ERR_RETURN(ret, -4);
    ret = setRecvCallback(&CEdoyunPlayerServer::Received, this, _1, _2); // 设置通信回调
    ERR_RETURN(ret, -5);
    // printf("[%s]<%s>:%d %s\n", __FILE__, __FUNCTION__, __LINE__, "回调设置成功");
    ret = m_epoll.Create(m_count); // 创建epoll
    ERR_RETURN(ret, -6);

    m_pool.start(m_count);                 // 开启线程池
    for (unsigned i = 0; i < m_count; i++) // 开启四个线程
    {
        std::future<int> res1 = m_pool.submitTask([this]()
                                                  { return ThreadFunc(); });
    }
    // printf("[%s]<%s>:%d %s\n", __FILE__, __FUNCTION__, __LINE__, "线程启动成功");
    int sock = 0;
    sockaddr_in addrin;
    while (m_epoll != -1) // 循环接收主进程发送客户端套接字
    {
        ret = proc->RecvSocket(sock, &addrin);
        if (ret < 0 || (sock == 0))
            break;

        CSocketBase *pCLient = new CSocket(sock);
        if (pCLient == NULL)
            continue;
        // printf("[%s]<%s>:%d %s\n", __FILE__, __FUNCTION__, __LINE__, "客户端子进程收到套接字");
        pCLient->Init(CSockParam(&addrin, SOCK_ISIP));
        WARN_CONTINUE(ret);

        ret = m_epoll.Add(sock, EPollData((void *)pCLient)); // 客户端套接字挂到树上用于通信
        if (m_connectdcallback)
        {
            (*m_connectdcallback)(pCLient); // 连接回调
        }
        WARN_CONTINUE(ret)
    }
    return 0;
}

// 连接回调
int CEdoyunPlayerServer::Connected(CSocketBase *pCLient)
{
    // 简单打印一下客户端信息
    sockaddr_in *paddr = *pCLient;
    TRACEI("client connected addr %s port:%d", inet_ntoa(paddr->sin_addr), (short)paddr->sin_port);
    return 0;
}

// 通信回调
int CEdoyunPlayerServer::Received(CSocketBase *pCLient, const Buffer &data)
{
    int ret = 0;
    Buffer responce = "";
    // HTTP解析
    ret = HttpParser(data);
    // 验证结果的反馈
    if (ret != 0) // 验证失败
    {
        TRACEE("http parser failed! ret=%d", ret);
    }
    responce = MakeResponse(ret);
    ret = pCLient->Send(responce); // 发送应答给客户端
    if (ret != 0)
    {
        // TRACEE("http response Send failed! ret=%d \n[%s]", ret, (char *)responce);
    }
    else
    {
        printf("http response success!%d", ret);
        TRACEI("http response success!%d", ret);
    }
    return 0;
}

int CEdoyunPlayerServer::HttpParser(const Buffer &data)
{
    // http解析
    CHttpParser parser;
    size_t size = parser.Parser(data);
    if (size == 0 || (parser.Errno() != 0))
    {
        TRACEE("size %llu errno:%u", size, parser.Errno());
        return -1;
    }

    // http解析完成
    if (parser.Method() == HTTP_GET)
    {
        UrlParser url("http://127.0.0.1" + parser.Url());
        int ret = url.Parser();
        if (ret != 0)
        {
            TRACEE("ret=%d url[%s]", ret, "http://127.0.0.1" + parser.Url());
            return -2;
        }
        Buffer uri = url.Uri();
        if (uri == "login")
        {
            // 处理登录
            Buffer time = url["time"];
            Buffer salt = url["salt"];
            Buffer user = url["user"];
            Buffer sign = url["sign"];
            // printf("time:%s salt:%s user:%s sign:%s\n", (char *)time, (char *)salt, (char *)user, (char *)sign);
            //  数据库的查询
            edoyunLogin_user_mysql dbuser;
            Result result;
            Buffer sql = dbuser.Query("user_name=\"" + user + "\"");
            ret = m_db->Exec(sql, result, dbuser);
            if (ret != 0)
            {
                TRACEE("sql=%s ret=%d", (char *)sql, ret);
                return -3;
            }
            if (result.size() == 0) // 查询结果为空
            {
                TRACEE("no result sql=%s ret=%d", (char *)sql, ret);
                return -4;
            }
            if (result.size() != 1) // 不存在多个用户
            {
                TRACEE("more than one sql=%s ret=%d", (char *)sql, ret);
                return -5;
            }
            PTable user1 = result.front();                              // 取第一行数据
            Buffer pwd = *user1->Fields["user_password"]->Value.String; // 取出密码
            // TRACEI("password = %s", (char *)pwd);
            //  登录请求的验证（数字签名是否一致）
            const char *MD5_KEY = "*&^%$#@b.v+h-b*g/h@n!h#n$d^ssx,.kl<kl";
            Buffer md5str = time + MD5_KEY + pwd + salt;
            Buffer md5 = Crypto::MD5(md5str);
            // TRACEI("MD5=%s", (char *)md5);
            if (md5 == sign) // 数字签名是否一致
            {
                return 0;
            }
            return -6;
        }
    }
    else if (parser.Method() == HTTP_POST)
    {
    }
    return -7;
}

Buffer CEdoyunPlayerServer::MakeResponse(int ret)
{
    json js;
    js["status"] = ret;
    if (ret != 0)
    {
        js["message"] = "登录失败,可能是用户名或者密码错误!";
    }
    else
    {
        js["message"] = "success";
    }
    Buffer json = js.dump();

    // 组装应答HTTP报文
    Buffer result = "HTTP/1.1 200 OK\r\n";
    time_t t;
    time(&t);
    tm *ptm = localtime(&t);
    char temp[64] = "";
    strftime(temp, sizeof(temp), "%a, %d %b %G %T GMT\r\n", ptm);
    Buffer Date = Buffer("Date: ") + temp;
    Buffer Server = "Server: Edoyun/1.0\r\nContent-Type: text/html; charset=utf-8\r\nX-Frame-Options: DENY\r\n";
    snprintf(temp, sizeof(temp), "%d", json.size());
    Buffer Length = Buffer("Content-Length: ") + temp + "\r\n";
    Buffer Stub = "X-Content-Type-Options: nosniff\r\nReferrer-Policy: same-origin\r\n\r\n";
    result += Date + Server + Length + Stub + json;
    // TRACEI("response: %s", (char *)result);
    return result;
}

int CEdoyunPlayerServer::ThreadFunc() // 线程函数等待通信事件
{
    int ret = 0;
    EPEvents events(EVENT_SIZE);
    while ((m_epoll != -1))
    {
        ssize_t size = m_epoll.WaitEvents(events); // 等待通信事件
        if (size < 0)
            break;
        if (size > 0)
        {
            for (size_t i = 0; i < size; i++)
            {
                if (events[i].events & EPOLLERR)
                {
                    break;
                }
                else if (events[i].events & EPOLLIN)
                {
                    CSocketBase *pClient = (CSocketBase *)events[i].data.ptr;
                    if (pClient)
                    {
                        Buffer data;
                        ret = pClient->Recv(data); // 接收客户端数据
                        if (ret <= 0)
                        {
                            TRACEW("Recv failded! ret=%d errno=%d msg=[%s]", ret, errno, strerror(errno));
                            m_epoll.Del(*pClient);
                            continue;
                        }
                        if (m_recvcallback)
                        {
                            (*m_recvcallback)(pClient, data); // 调用通信回调函数
                        }
                    }
                }
            }
        }
    }
    return 0;
}