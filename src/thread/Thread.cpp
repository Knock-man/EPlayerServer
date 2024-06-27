#include "Thread.h"

std::map<pthread_t, CThread *> CThread::m_mapThread;

// 开启线程
int CThread::Start()
{
    pthread_attr_t attr;
    int ret = 0;
    ret = pthread_attr_init(&attr); // 初始化线程属性对象
    if (ret != 0)
        return -1;
    ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); // 设置为分离状态并创建，无需pthread_join，线程终止自动释放
    if (ret != 0)
        return -2;
    // ret = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
    // if (ret != 0)return -3;
    ret = pthread_create(&m_thread, &attr, &CThread::ThreadFunction, this); // 创建线程
    if (ret != 0)
        return -4;
    m_mapThread[m_thread] = this;      // 储存线程
    ret = pthread_attr_destroy(&attr); // 销毁线程属性对象
    if (ret != 0)
        return -5;
    return 0;
}

// 暂停线程
int CThread::Pause()
{
    if (m_thread != 0)
        return -1;
    if (m_bpaused) // 线程已经被暂停
    {
        m_bpaused = false; // 启动线程
        return 0;
    }
    // 暂停线程
    m_bpaused = true;
    int ret = pthread_kill(m_thread, SIGUSR1);
    if (ret != 0)
    {
        m_bpaused = false;
        return -2;
    }
    return 0;
}

// 终止线程
int CThread::Stop()
{
    if (m_thread != 0)
    {
        pthread_t thread = m_thread;
        m_thread = 0; // 先让信号处理函数Sigaction终止（先结束暂停）

        // 设置终止等待时间
        timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 100 * 1000000;
        // 100ms;
        int ret = pthread_timedjoin_np(thread, NULL, &ts); // 等待10ms线程结束
        if (ret == ETIMEDOUT)                              // 超时线程依然没有结束，终止线程
        {
            pthread_detach(thread);        // 分离线程
            pthread_kill(thread, SIGUSR2); // 向thread线程发送一个SIGUSR2终止信号，终止线程
        }
    }
    return 0;
}

// 线程函数 注册信号处理函数Sigaction  调用线程逻辑函数  业务处理完毕销毁线程
void *CThread::ThreadFunction(void *arg)
{
    CThread *thiz = (CThread *)arg;

    // 注册信号处理函数
    struct sigaction act = {0};
    sigemptyset(&act.sa_mask);              // 清空mask
    act.sa_flags = SA_SIGINFO;              // 使用包含额外信息的信号处理程序
    act.sa_sigaction = &CThread::Sigaction; // 设置信号处理函数
    sigaction(SIGUSR1, &act, NULL);         // 注册两个信号
    sigaction(SIGUSR2, &act, NULL);

    // 进入线程逻辑函数
    thiz->EnterThread();

    // 线程逻辑执行结束，线程自动退出
    if (thiz->m_thread)
        thiz->m_thread = 0;

    pthread_t thread = pthread_self(); // 不是冗余，有可能被stop函数把m_thread清零
    auto it = m_mapThread.find(thread);
    if (it != m_mapThread.end())
    {
        m_mapThread[thread] = NULL; // 线程map中删除
    }
    // 线程退出
    pthread_detach(thread); // 当前线程分离
    pthread_exit(NULL);     // 销毁本线程
}

//_thiscall 线程逻辑函数
void CThread::EnterThread()
{
    if (m_function != NULL)
    {
        int ret = (*m_function)(); // 执行设置的业务函数
        if (ret != 0)
        {
            printf("%s(%d):[%s]ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
        }
    }
}

// 信号处理函数
void CThread::Sigaction(int signo, siginfo_t *info, void *context)
{
    if (signo == SIGUSR1) // 线程休眠信号
    {
        // 留给pause用
        pthread_t thread = pthread_self();
        auto it = m_mapThread.find(thread);
        if (it != m_mapThread.end())
        {
            if (it->second)
            {

                while (it->second->m_bpaused) // 循环阻塞，等待m_bpaused=false
                {
                    if (it->second->m_thread == 0)
                    {
                        pthread_exit(NULL);
                    }
                    usleep(1000); // 1ms
                }
            }
        }
    }
    else if (signo == SIGUSR2) // 线程退出信号
    {
        pthread_exit(NULL);
    }
}