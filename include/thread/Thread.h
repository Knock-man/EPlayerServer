#pragma once
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include <map>
#include "function.h"

class CThread
{
public:
    CThread() // 初始化线程
    {
        m_function = NULL; // 线程函数
        m_thread = 0;
        m_bpaused = false; // 是否暂停
    }

    template <typename _FUNCTION_, typename... _ARGS_>
    CThread(_FUNCTION_ func, _ARGS_... args) // 构造直接设置线程执行函数
        : m_function(new CFunction<_FUNCTION_, _ARGS_...>(func, args...))
    {
        m_thread = 0;
        m_bpaused = false;
    }

    // 禁止赋值构造和拷贝构造
    CThread(const CThread &) = delete;
    CThread operator=(const CThread &) = delete;

public:
    // 设置线程的执行函数
    template <typename _FUNCTION_, typename... _ARGS_>
    int SetThreadFunc(_FUNCTION_ func, _ARGS_... args)
    {
        m_function = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
        return 0;
    }

    // 开启线程
    int Start(); // 创建子线程 执行线程函数 回收子线程

    // 暂停线程
    int Pause();

    // 终止线程
    int Stop();

    bool isValid() const { return m_thread != 0; }

private:
    //_stdcall //子线程函数
    static void *ThreadFunction(void *arg);

    //_thiscall 线程逻辑函数 调用业务函数对象
    void EnterThread();

    // 信号处理函数 接收信号 执行暂停线程终止线程操作
    static void Sigaction(int signo, siginfo_t *info, void *context);

private:
    CFunctionBase *m_function;                         // 业务函数对象，在线程中调用
    pthread_t m_thread;                                // 子线程ID
    bool m_bpaused;                                    // true表示暂停  false表示运行中
    static std::map<pthread_t, CThread *> m_mapThread; // 储存线程对象
};