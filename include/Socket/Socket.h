// 套接字地址类 网络通信类
#pragma once
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <fcntl.h>
#include "Buffer.h"

// 套接字类型标记
enum SockAttr
{
    SOCK_ISSERVER = 1,  // 是否服务器 1服务器 0客户端        0001
    SOCK_ISNOBLOCK = 2, // 是否阻塞 1非阻塞 0阻塞           0010
    SOCK_ISUDP = 4,     // 是否为UDP 1UDP 0TCP             0100
    SOCK_ISIP = 8,      // 是否为IP协议 1IP协议 0本地套接字  1000
    SOCK_ISREUSE = 16   // 是否重用地址
};

// socket地址类
class CSockParam
{
public:
    CSockParam(); // 构造初始化IP 端口 套接字类型

    // 使用IP 网络套接字地址初始化
    CSockParam(const Buffer &ip, short port, int attr);

    // 使用IP 网络套接字地址初始化
    CSockParam(const sockaddr_in *addrin, int attr);

    // 使用路径 本地套接字地址初始化
    CSockParam(const Buffer &path, int attr);

    // 直接使用地址类型初始化（浅拷贝构造）
    CSockParam(const CSockParam &param);

    ~CSockParam();

public:
    // 赋值构造
    CSockParam &operator=(const CSockParam &param);

    // 获得地址
    sockaddr *addrin() { return (sockaddr *)&addr_in; }
    sockaddr *addrun() { return (sockaddr *)&addr_un; }

public:
    // 地址
    sockaddr_in addr_in; // 网络套接字地址结构
    sockaddr_un addr_un; // 本地套接字地址结构
    Buffer ip;           // ip地址
    short port;          // 端口
    // 参考SockAttr
    int attr; // 使用SockAttr表示套接字类型
};

// 网络通信抽象基类，方便不同的通信方式继承实现（TCP UDP）
class CSocketBase
{
public:
    CSocketBase()
    {
        m_socket = -1;
        m_status = 0; // 初始化未完成
    }
    virtual ~CSocketBase()
    {
        Close();
    };

public:
    // [服务器]:套接字创建 bind listen [客户端]: 套接字创建
    virtual int Init(const CSockParam &param) = 0;

    // 连接 [服务器]：accept  [客户端]：connect 对于udp这里可以忽略
    virtual int Link(CSocketBase **pClient = NULL) = 0;

    // 发送数据
    virtual int Send(const Buffer &daata) = 0;

    // 接收数据
    virtual int Recv(Buffer &data) = 0;

    // 关闭连接
    virtual int Close()
    {
        m_status = 3; // 设状态已经关闭
        if (m_socket != -1)
        {
            // 是服务器，且不是网络套接字通信
            if ((m_param.attr & SOCK_ISSERVER) &&
                ((m_param.attr & SOCK_ISIP)) == 0)
                unlink(m_param.ip); // 删除文件 本地套接字ip=path 见73行
            int fd = m_socket;      // 关闭套接字
            m_socket = -1;
            close(fd);
        }
        return 0;
    }
    virtual operator int() // 重载括号运算符
    {
        return m_socket;
    }
    virtual operator int() const
    {
        return m_socket;
    }
    virtual operator const sockaddr_in *() const
    {
        return &m_param.addr_in;
    }
    virtual operator sockaddr_in *()
    {
        return &m_param.addr_in;
    }

public:
    // 套接字描述符，默认-1
    int m_socket;
    // [套接字状态] 0:初始化未完成 1:初始化完成 2:连接完成 3:已经关闭
    int m_status;
    // 初始化网络地址参数
    CSockParam m_param;
};

class CSocket : public CSocketBase
{
public:
    CSocket() : CSocketBase(){}; // 默认初始化
    CSocket(int fd) : CSocketBase()
    {
        m_socket = fd; // 已有套接字初始化
    }

    virtual ~CSocket()
    {
        Close(); // 关闭套接字
    };
    // [服务器]：套接字创建 bind listen
    // [客户端]：套接字创建
    int Init(const CSockParam &param);

    // 连接 [服务器]: accept  [客户端}:connect   对于udp这里可以忽略
    int Link(CSocketBase **pClient = NULL); // pClinet传出参数，保存已连接的客户端套接字

    // 发送数据
    int Send(const Buffer &data);

    // 接收数据 大于零表示接收成功 小于0表示失败 等于0表示没有收到数据但没有失败
    int Recv(Buffer &data);

    // 关闭连接
    int Close() { return CSocketBase::Close(); } // 调用父类的关闭函数
};
