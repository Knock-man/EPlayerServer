#include "Socket.h"

/***********************套接字地址类**************************/
CSockParam::CSockParam() // 构造初始化地址 端口 套接字类型
{
    bzero(&addr_in, sizeof(addr_in));
    bzero(&addr_un, sizeof(addr_un));
    port = -1;
    ip = "";
    attr = 0; // 默认客户端-阻塞-TCP
};

// 使用IP port 网络套接字地址初始化
CSockParam::CSockParam(const Buffer &ip, short port, int attr)
{
    this->ip = ip;
    this->port = port;
    this->attr = attr;
    addr_in.sin_family = AF_INET;
    addr_in.sin_addr.s_addr = inet_addr(ip);
    addr_in.sin_port = htons(port);
};

// 使用IP地址 网络套接字地址初始化
CSockParam::CSockParam(const sockaddr_in *addrin, int attr)
{
    this->ip = ip;
    this->port = port;
    this->attr = attr;
    memcpy(&addr_in, addrin, sizeof(addr_in));
};

// 使用路径 本地套接字地址初始化
CSockParam::CSockParam(const Buffer &path, int attr)
{
    ip = path;
    addr_un.sun_family = AF_UNIX;
    strcpy(addr_un.sun_path, path);
    this->attr = attr;
}
CSockParam::~CSockParam(){};

// 直接使用地址结构初始化（浅拷贝构造）
CSockParam::CSockParam(const CSockParam &param)
{
    ip = param.ip;
    port = param.port;
    attr = param.attr;
    memcpy(&addr_in, &param.addr_in, sizeof(addr_in));
    memcpy(&addr_un, &param.addr_un, sizeof(addr_un));
};

// 等号运算符重载
CSockParam &CSockParam::operator=(const CSockParam &param)
{
    if (this != &param)
    {
        ip = param.ip;
        port = param.port;
        attr = param.attr;
        memcpy(&addr_in, &param.addr_in, sizeof(addr_in));
        memcpy(&addr_un, &param.addr_un, sizeof(addr_un));
    }
    return *this;
}

/*********************网络通信类*****************************/
// 网络通信初始化 【服务器】：socket(),bind(),listen(),是否阻塞   【客户端】：socket(),是否阻塞
int CSocket::Init(const CSockParam &param)
{
    int ret = 0;
    if (m_status != 0) // 保证未初始化
        return -1;
    m_param = param; // 缓存地址

    // 创建套接字
    //  服务器客户端都需要创建套接
    int type = m_param.attr & SOCK_ISUDP ? SOCK_DGRAM : SOCK_STREAM; //  确定TCP连接还是UDP连接
    if (m_socket == -1)                                              // 保证套接字未初始化
    {
        if (param.attr & SOCK_ISIP) // 创建网络套接字
        {
            m_socket = socket(PF_INET, type, 0);
        }
        else // 创建本地套接字
        {
            m_socket = socket(PF_LOCAL, type, 0);
        }
        if (m_socket == -1)
            return -2;
    }
    else
    {                 // 已经建立连接的客户端初始化时，改变一下连接状态即可
        m_status = 2; // 已经处于连接状态
    }

    // 设置端口复用
    if (m_param.attr & SOCK_ISREUSE)
    {
        int option = 1;
        ret = setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
        if (ret == -1)
            return -3;
    }

    // 服务器需要bind() listen()
    if (m_param.attr & SOCK_ISSERVER)
    {
        if (param.attr & SOCK_ISIP)
        {
            ret = bind(m_socket, m_param.addrin(), sizeof(sockaddr_un)); // 绑定
        }
        else
        {
            ret = bind(m_socket, m_param.addrun(), sizeof(sockaddr_un)); // 绑定
        }
        if (ret == -1)
            return -4;

        ret = listen(m_socket, 32); // 监听
        if (ret == -1)
            return -5;
    }

    // 套接字是否需要设置非阻塞
    if (m_param.attr & SOCK_ISNOBLOCK)
    {
        // 非阻塞
        int option = fcntl(m_socket, F_GETFL);
        if (option == -1)
            return -6;
        option |= O_NONBLOCK;
        ret = fcntl(m_socket, F_SETFL, option);
        if (ret == -1)
            return -7;
    }

    // 转变套接字状态为已初始化
    if (m_status == 0)
    {
        m_status = 1; // 初始化完成
    }

    return 0;
}

// 连接 [服务器]: accept  [客户端}:connect   对于udp这里可以忽略
int CSocket::Link(CSocketBase **pClient) // pClinet传出参数，返回已连接的客户端套接字对象
{
    // 保证套接字已经初始化完成
    if ((m_status < 1) && (m_socket == -1)) // m_status = 1; 初始化完成
        return -1;

    int ret = 0;

    // 服务器
    if (m_param.attr & SOCK_ISSERVER) //// 需要accept() 接受连接
    {
        if (pClient == NULL)
            return -2;
        CSockParam param; // 默认地址类型为attr=0  客户端\阻塞\TCP\本地套接字
        int fd = -1;
        socklen_t len = 0;
        if (m_param.attr & SOCK_ISIP) // 网络套接字通信
        {
            param.attr |= SOCK_ISIP; // 标志位修改位网络套接字通信
            len = sizeof(sockaddr_in);
            fd = accept(m_socket, param.addrin(), &len); // 接收客户端连接
        }
        else // 本地套接字通信
        {
            len = sizeof(sockaddr_un);
            fd = accept(m_socket, param.addrun(), &len); // 接收客户端连接
        }
        if (fd == -1)
            return -3;

        *pClient = new CSocket(fd); // 创建客户端套接字对象作为传出参数
        // m_socket =fd; ;父类构造将m_status = 0；
        if (*pClient == NULL)
            return -4;
        ret = (*pClient)->Init(param); // 初始化连接成功客户端套接字 做的事情仅仅m_status = 2;//已连接
        if (ret != 0)
        {
            delete (*pClient);
            *pClient = NULL;
            return -5;
        }
    }
    else // 客户端 connect()连接
    {
        if (m_param.attr & SOCK_ISIP) // 网络套接字连接
        {
            ret = connect(m_socket, m_param.addrin(), sizeof(sockaddr_in));
        }
        else // 本地套接字conenct
        {
            ret = connect(m_socket, m_param.addrun(), sizeof(sockaddr_un));
        }

        if (ret != 0)
            return -6;
    }
    m_status = 2; // 状态设为连接完成
    return 0;
}

// 发送数据
int CSocket::Send(const Buffer &data)
{
    // 保证已经连接完成
    if (m_status < 2 || (m_socket == -1))
        return -1;

    ssize_t index = 0;
    while (index < (ssize_t)data.size())
    {

        ssize_t len = write(m_socket, (char *)data + index, data.size() - index);
        if (len == 0)
            return -2;
        if (len < 0)
            return -3;
        index += len;
    }
    return index;
}
// 接收数据 大于零表示接收成功 小于0检查errno，等于0读到文件末尾或者套接字关闭
int CSocket::Recv(Buffer &data)
{
    // 保证已经连接完成
    if (m_status < 2 || (m_socket == -1))
        return -1;
    data.resize(1024 * 1024);
    ssize_t len = read(m_socket, data, data.size());
    if (len > 0)
    {
        data.resize(len);
        return (int)len; // 收到数据
    }

    if (len < 0)
    {
        if (errno == EINTR || (errno == EAGAIN))
        { // 系统调用被中断或非阻塞返回
            data.clear();
            return 0; // 没有收到数据
        }
        return -2; // 发送错误
    }
    return -3; // 套接字被关闭或者读到文件末尾 read()返回0
}
