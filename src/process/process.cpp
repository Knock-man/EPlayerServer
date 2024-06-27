#include "process.h"

// 创建子进程
int CProcess::CreateSubProcess()
{
    if (m_func == NULL)
    {
        return -1;
    }
    /*
    socketpair 函数用于创建一个已连接的套接字对，它们可以用于进程间通信，其中一个套接字用于读取，另一个用于写入。
        AF_LOCAL：表示使用本地通信域（也称为 Unix 域或本地域）。
        SOCK_STREAM：表示创建的套接字类型为流式套接字，即面向连接的 TCP 套接字。
        0：通常是指定协议，使用 0 表示根据前两个参数自动选择协议。
        pipes：这是一个数组或指针，用于存放创建的套接字对的文件描述符。
    */
    int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd); // 创建套接字对
    if (ret == -1)
    {
        perror("socketpair");
        return -2;
    }

    pid_t pid = fork(); // 创建子进程
    if (pid == -1)
    {
        perror("fork");
        return -3;
    }

    // 子进程
    if (pid == 0)
    {

        close(sockfd[1]); // 关闭掉写
        sockfd[1] = 0;
        ret = (*m_func)(); // 执行子进程入口函数
        exit(0);
    }

    // 主进程
    close(sockfd[0]); // 关闭读
    sockfd[0] = 0;
    m_pid = pid; // 储存子进程ID
    return 0;
}

// 发送文件描述符
int CProcess ::SendFD(int fd)
{
    struct msghdr msg = {0}; // 消息结构体
    struct iovec iov[1];
    char buffer[10] = "edoyun";
    iov[0].iov_base = buffer;
    iov[0].iov_len = sizeof(buffer);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    char control_data[CMSG_SPACE(sizeof(int))]; // 开辟存储辅助数据内存 辅助信息为一个文件描述符 4字节 = sizeof(int)
    msg.msg_control = control_data;
    msg.msg_controllen = sizeof(control_data);
    // 辅助数据结构体
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    *(int *)CMSG_DATA(cmsg) = fd;              // 数据段储存文件描述符
    ssize_t ret = sendmsg(sockfd[1], &msg, 0); // 发送
    if (ret == -1)
    {
        perror("sendmsg");
        return -1;
    }
    close(sockfd[1]); // 关闭写

    // printf("%s(%d):<%s> ret=%zd\n", __FILE__, __LINE__, __FUNCTION__, ret);
    return 0;
}

// 接收文件描述符 存在fd中
int CProcess::RecvFD(int &fd)
{
    struct msghdr msg = {0}; // 消息结构体
    struct iovec iov[1];
    char buffer[10];
    iov[0].iov_base = buffer;
    iov[0].iov_len = sizeof(buffer);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    char control_data[CMSG_SPACE(sizeof(int))]; // 开辟存储辅助数据内存 辅助信息为一个文件描述符 4字节 = sizeof(int)
    msg.msg_control = control_data;
    msg.msg_controllen = sizeof(control_data);
    ssize_t ret = recvmsg(sockfd[0], &msg, 0); // 接收
    if (ret == -1)
    {
        return -2;
    }
    // 从辅助信息中获取文件描述符
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); // 获取消息头的第一个辅助数据块
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));  // 从数据段首地址复制
    close(sockfd[0]);                           // 关闭读
    return 0;
}

// 发送文件描述符
int CProcess::SendSocket(int fd, const sockaddr_in *addrin)
{
    struct msghdr msg = {0}; // 消息结构体
    struct iovec iov[1];
    char buffer[10];
    memcpy(buffer, addrin, sizeof(sockaddr_in));
    iov[0].iov_base = buffer;
    iov[0].iov_len = sizeof(buffer);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    char control_data[CMSG_SPACE(sizeof(int))]; // 开辟存储辅助数据内存 辅助信息为一个文件描述符 4字节 = sizeof(int)
    msg.msg_control = control_data;
    msg.msg_controllen = sizeof(control_data);
    // 辅助数据结构体
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    *(int *)CMSG_DATA(cmsg) = fd;              // 数据段储存文件描述符
    ssize_t ret = sendmsg(sockfd[1], &msg, 0); // 发送
    if (ret == -1)
    {
        perror("sendmsg");
        return -1;
    }
    close(sockfd[1]); // 关闭写

    // printf("%s(%d):<%s> ret=%zd\n", __FILE__, __LINE__, __FUNCTION__, ret);
    return 0;
}

// 接收文件描述符 存在fd中
int CProcess::RecvSocket(int &fd, sockaddr_in *addrin)
{
    struct msghdr msg = {0}; // 消息结构体
    struct iovec iov[1];
    char buffer[10];
    iov[0].iov_base = buffer;
    iov[0].iov_len = sizeof(buffer);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    char control_data[CMSG_SPACE(sizeof(int))]; // 开辟存储辅助数据内存 辅助信息为一个文件描述符 4字节 = sizeof(int)
    msg.msg_control = control_data;
    msg.msg_controllen = sizeof(control_data);
    ssize_t ret = recvmsg(sockfd[0], &msg, 0); // 接收
    if (ret == -1)
    {
        return -2;
    }
    // 从辅助信息中获取文件描述符
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); // 获取消息头的第一个辅助数据块
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));  // 从数据段首地址复制
    memcpy(addrin, buffer, sizeof(sockaddr_in));
    close(sockfd[0]); // 关闭读
    return 0;
}

// 转换到守护进程
int CProcess::SwitchDeamon()
{
    pid_t ret = fork(); // 创建子进程
    if (ret == -1)
        return -1;

    if (ret > 0)
        exit(0); // 主进程退出

    // 子进程
    ret = setsid(); // 子进程设为会话组长
    if (ret == -1)
        return -2;

    ret = fork(); // 再创建孙进程
    if (ret == -1)
        return -3;
    if (ret > 0) // 子进程退出
        exit(0);

    // 孙进程，加入守护状态
    for (int i = 0; i < 3; i++) // 0标准输入 1标准输出 2标准错误
        close(i);

    umask(0);                 // 使用最大权限创建文件
    signal(SIGCHLD, SIG_IGN); // 屏蔽子进程退出通知
    return 0;
}