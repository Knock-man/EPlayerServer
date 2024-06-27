#pragma once
// 日志类 日志服务器类
#include "Thread.h"
#include "Epoll.h"
#include "Socket.h"
#include <list>
#include <sys/timeb.h>
#include <sys/time.h>
#include <stdarg.h>
#include <sstream>
#include <sys/stat.h>
#include "Buffer.h"

enum LogLevel // 日志等级
{
    LOG_INFO,    // 信息日志
    LOG_DEBUG,   // 调试日志
    LOG_WARNING, // 警告日志
    LOG_ERROR,   // 错误日志
    LOG_FATAL    // 致命错误日志
};

class CLoggerServer;

// 日志打包类
class LogInfo
{
public:
    // 构造函数在做一件事情  打包好m_buf-日志内容

    //  char buffer[] = "hello edoyun 肖老师"; TRACEI("here is log %d %c %f %g %s 哈哈 嘻嘻 易道云", 10, 'A', 1.0f, 2.0, buffer);
    LogInfo(
        const char *file,      // 文件名
        int line,              // 行数
        const char *func,      // 函数
        pid_t pid,             // 进程号
        pthread_t tid,         // 线程号
        int level,             // 日志等级
        const char *fmt, ...); // 可变参数

    // LOGE << 100 << " " << 'A' << " " << 0.12345f << " " << 1.2345 << " " << buffer << " "<< "易道云编程";
    LogInfo( // 流的方式写日志
        const char *file,
        int line,
        const char *func,
        pid_t pid,
        pthread_t tid,
        int level);

    // char buffer[] = "hello edoyun 肖老师";  DUMPD((void *)buffer, (size_t)sizeof(buffer));
    LogInfo(const char *file,
            int line,
            const char *func,
            pid_t pid,
            pthread_t tid,
            int level,
            void *pData, // 传字符串
            size_t nSize);

    ~LogInfo();
    operator Buffer() const
    {
        return m_buf;
    }

    // 重载流运算符
    template <typename T>
    LogInfo &operator<<(const T &data)
    {
        std::stringstream stream;
        stream << data;
        m_buf += stream.str(); // 流进buff中
        return *this;
    }

private:
    bool bAuto;   // 自动发送 默认false 流式日志，则为true
    Buffer m_buf; // 缓冲区
};

// 日志服务器类
#define _DEBUG
class CLoggerServer
{
public:
    CLoggerServer() : m_thread(&CLoggerServer::ThreadFunc, this) // 绑定线程函数
    {
        m_server = NULL;
        m_path = "./../log/" + GetTimeStr() + ".log"; // 日志文件路径
        // printf("%s(%d):[%s]path=%s\n", __FILE__, __LINE__, __FUNCTION__, (char *)m_path);
    }
    ~CLoggerServer()
    {
        Close();
    }

    // 禁止赋值构造和拷贝构造
    CLoggerServer(const CLoggerServer &) = delete;
    CLoggerServer &operator=(const CLoggerServer &) = delete;

public:
    // 日志服务器的启动
    int Start();

    int Close();

    // 给其他非日志进程和线程使用的
    static void Trace(const LogInfo &info);

    // 获得时间
    static Buffer GetTimeStr();

private:
    // 写日志
    void WriteLog(const Buffer &data);

    // 线程函数
    int ThreadFunc();

private:
    CThread m_thread; // 线程
    CEpoll m_epoll;
    CSocketBase *m_server; // 连接服务器
    Buffer m_path;
    FILE *m_file;
};

// 写日志的宏，提供三种方式
// 方式一  TRACEI("here is log %d %c %f %g %s 哈哈 嘻嘻 易道云", 10, 'A', 1.0f, 2.0, buffer);
#ifndef TRACE
#define TRACEI(...) \
    CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_INFO, __VA_ARGS__))
#define TRACED(...) \
    CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_DEBUG, __VA_ARGS__))
#define TRACEW(...) \
    CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_WARNING, __VA_ARGS__))
#define TRACEE(...) \
    CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_ERROR, __VA_ARGS__))
#define TRACEF(...) \
    CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_FATIAL, __VA_ARGS__))
#endif

// 方式二  DUMPD((void *)buffer, (size_t)sizeof(buffer));
#define DUMPI(data, size) \
    CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_INFO, data, size))
#define DUMPD(data, size) \
    CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_DEBUG, data, size))
#define DUMPW(data, size) \
    CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_WARNING, data, size))
#define DUMPE(data, size) \
    CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_ERROR, data, size))
#define DUMPF(data, size) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_FATIAL, data, size))

// 方式三  LOGE << 100 << " " << 'A' << " " << 0.12345f << " " << 1.2345 << " " << buffer << " "<< "易道云编程";
#define LOGI LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_INFO)
#define LOGD LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_DEBUG)
#define LOGW LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_WARNING)
#define LOGE LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_ERROR)
#define LOGF LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_FATIAL)