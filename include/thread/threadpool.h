#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <future>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <iostream>
#include <unordered_map>

const int TASK_MAX_THRESHHOLD = 1024;   // INT32_MAX;//任务队列阈值
const int THREAD_MAX_THRESHHOLD = 1024; // 线程阈值
const int THREAD_MAX_IDLE_TIME = 60;    // 最大空闲时间

// 线程池支持的模式
enum class PoolMode
{
    MODE_FIXED,  // 固定线程数量的线程池
    MODE_CACHED, // 线程数量动态增长线程池
};

class Thread
{
public:
    // 线程函数对象类型
    using ThreadFunc = std::function<void(int)>;

    Thread(ThreadFunc func)
        : func_(func),
          threadId_(generateId_++)
    {
    }
    ~Thread() = default;

    int getId() const
    {
        return threadId_;
    }
    // 启动线程
    void start()
    {
        // 创建一个线程来执行线程函数
        std::thread t(func_, threadId_);
        t.detach();
    }

private:
    ThreadFunc func_;       // 线程函数
    static int generateId_; // 自增长线程id
    int threadId_;          // 保存线程id
};

// 线程池
class ThreadPool
{
public:
    ThreadPool()
        : initThreadSize_(0),
          curThreadSize_(0),
          threadSizeThreadHold_(THREAD_MAX_THRESHHOLD),
          idleThreadSize_(0),
          taskSize_(0),
          taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD),
          poolMode_(PoolMode::MODE_FIXED),
          isPoolRuning_(false)

    {
    }
    ~ThreadPool()
    {
        isPoolRuning_ = false;
        // 等待线程池里面所有的线程返回 有两种状态：阻塞 & 正在执行任务中
        std::unique_lock<std::mutex> lock(taskQueMtx_);
        notEmpty_.notify_all();
        exitCond_.wait(lock, [&]() -> bool
                       { return threads_.size() == 0; });
    }

    // 禁止拷贝构造和赋值构造
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    int Close()
    {
        isPoolRuning_ = false;
        // 等待线程池里面所有的线程返回 有两种状态：阻塞 & 正在执行任务中
        std::unique_lock<std::mutex> lock(taskQueMtx_);
        notEmpty_.notify_all();
        exitCond_.wait(lock, [&]() -> bool
                       { return threads_.size() == 0; });
        return 0;
    }

    // 设置线程池工作模式
    void setMode(PoolMode mode)
    {
        if (checkRuningState())
            return; // 线程池已启动，不允许设置
        poolMode_ = mode;
    }

    // 设置任务队列上限阈值
    void setTaskQueMaxThreadHold(size_t threadhold)
    {
        taskQueMaxThreshHold_ = threadhold;
    }

    // 设置线程池cached模式下线程阈值
    void setThreadSizeThreshHold(size_t threadhold)
    {
        if (checkRuningState())
            return; // 线程池已启动，不允许设置
        if (poolMode_ == PoolMode::MODE_CACHED)
        {
            threadSizeThreadHold_ = threadhold;
        }
    }

    // 给线程池提交任务
    // 使用可变参模板编程，让submitTask可以接受任意任务函数和任意数量的参数
    // Result submitTask(std::shared_ptr<Task> sp);
    template <typename Func, typename... Args>
    auto submitTask(Func &&func, Args &&...args) -> std::future<decltype(func(args...))>
    {
        // 打包任务，放入任务队列里面
        using Rtype = decltype(func(args...));
        auto task = std::make_shared<std::packaged_task<Rtype()>>(
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
        std::future<Rtype> result = task->get_future();

        // 获取锁
        std::unique_lock<std::mutex> lock(taskQueMtx_);
        // 线程通信 等待等待任务队列有空余
        //  while(taskQue_.size() == taskQueMaxThreshHold_)
        //  {
        //      notFull_.wait(lock);
        //  }
        // 用户提交任务，最长不能阻塞超过1s，否则判断任务提交失败

        bool flag = notFull_.wait_for(lock,
                                      std::chrono::seconds(1),
                                      [&]() -> bool
                                      { return taskQue_.size() < taskQueMaxThreshHold_; });
        if (!flag)
        {
            // 表示notFull_等待1s钟，条件依然没有满足
            std::cerr << "task queue is full, submit task fail." << std::endl;
            auto task = std::make_shared<std::packaged_task<Rtype()>>(
                []() -> Rtype
                { return Rtype(); });
            (*task)();
            return task->get_future();
        }

        // 如果有空余，把任务队列放入任务队列中
        // taskQue_.emplace(sp);
        taskQue_.emplace([task]()
                         { (*task)(); });
        taskSize_++;

        // 因为新放了任务，任务队列肯定不空了,notEmpty_上进行通知,赶快分配线程执行任务
        notEmpty_.notify_all();

        // cached模式，任务处理比较紧急 场景：小而快的任务 需要根据任务数量和空闲线程的数量，判断是否需要创建新的线程出来
        if (poolMode_ == PoolMode::MODE_CACHED && taskSize_ > idleThreadSize_ && curThreadSize_ < (int)threadSizeThreadHold_)
        {
            // std::cout<<"<<<<<<<<<<<<<<<<<<<<<<<<创建了新线程<<<<<<<<<<<<<<<<<<<<<<<<<<"<<std::endl;
            // 创建新线程
            std::unique_ptr<Thread> ptr(new Thread(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1)));
            // threads_.emplace_back(ptr->getId(),std::move(ptr));
            int threadId = ptr->getId();
            threads_.emplace(threadId, std::move(ptr));
            // 启动线程
            threads_[threadId]->start();
            // 相爱线程个数相关的变量
            curThreadSize_++;
            idleThreadSize_++;
        }

        // 返回任务的Result对象
        return result;
    }

    // 开启线程池
    void start(size_t initThreadSize = std::thread::hardware_concurrency()) // 初始线程数量
    {
        // 设置线程池的运行状态
        isPoolRuning_ = true;
        // 初始线程个数
        initThreadSize_ = initThreadSize;
        curThreadSize_ = initThreadSize;

        // 创建线程对象
        for (int i = 0; i < (int)initThreadSize_; i++)
        {

            std::unique_ptr<Thread> ptr(new Thread(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1)));
            // std::unique_ptr<Thread> ptr(new Thread(std::bind(&ThreadPool::threadFunc,this)));
            // threads_.emplace_back(std::move(ptr));//unique_ptr不允许左值引用拷贝构造，可右值引用拷贝构造-
            threads_.emplace(ptr->getId(), std::move(ptr));
        }

        // 启动所有线程
        for (int i = 0; i < (int)initThreadSize_; i++)
        {
            threads_[i]->start(); // 需要去执行一个线程函数
            idleThreadSize_++;    // 记录初始空闲线程数量
        }
    }

private:
    // 定义线程函数
    void threadFunc(int threadid)
    {
        auto lastTime = std::chrono::high_resolution_clock().now();

        for (;;)
        {
            Task task;
            { // 保证取出任务立马释放锁，让别的线程去取任务，而不是等到任务执行结束再释放
                // 先获取锁
                std::unique_lock<std::mutex> lock(taskQueMtx_);
                // std::cout<<"线程"<<std::this_thread::get_id()<<" 尝试获取任务"<<std::endl;
                // cached模式下，有可能已经创建了很多的线程，但是空闲时间超过60s,应该把多余的线程结束回收掉
                //(超过initThreadSize数量的线程要进行回收)
                // 当前时间 - 上一次线程执行的时间 > 60s
                // 每一秒中返回一次 怎么区分：超时返回？还是任务执行返回
                while (taskQue_.size() == 0)
                {
                    if (!isPoolRuning_)
                    {
                        threads_.erase(threadid);
                        exitCond_.notify_all();
                        return; // 线程函数结束线程结束
                    }
                    if (poolMode_ == PoolMode::MODE_CACHED)
                    {
                        // 条件变量，超时返回了
                        if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1)))
                        {
                            auto now = std::chrono::high_resolution_clock().now();
                            auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
                            if (dur.count() >= THREAD_MAX_IDLE_TIME && curThreadSize_ > (int)initThreadSize_) // 空闲队列超时
                            {
                                // 回收当前线程
                                // 记录线程数量相关变量的值修改
                                // 把线程对象从线程列表容器中删除 threadid => thread对象 => 删除
                                threads_.erase(threadid);
                                curThreadSize_--;
                                idleThreadSize_--;
                                return;
                            }
                        }
                    }
                    else if (poolMode_ == PoolMode::MODE_FIXED)
                    {
                        // 等待notEmpty条件
                        notEmpty_.wait(lock);
                    }
                }

                idleThreadSize_--;

                // 从任务队列中取一个任务出来
                task = taskQue_.front();
                taskQue_.pop();
                taskSize_--;
                // std::cout<<"线程"<<std::this_thread::get_id()<<" 获取任务成功"<<std::endl;
            }

            // 如果依然有剩余任务，继续通知其它的线程执行任务
            if (taskQue_.size() > 0)
            {
                notEmpty_.notify_all();
            }
            // 取出一个任务，进行通知，可以继续提交生产任务
            notFull_.notify_all();

            // 当前线程负责执行这个任务
            if (task != nullptr)
            {
                // task->exec();
                task();
            }

            idleThreadSize_++;
            lastTime = std::chrono::high_resolution_clock().now();
        }
    }

    // 检查pool的运行状态
    bool checkRuningState() const
    {
        return isPoolRuning_;
    }

public:
    int Size()
    {
        return initThreadSize_;
    }

private:
    // std::vector<std::unique_ptr<Thread>> threads_; //线程列表
    std::unordered_map<int, std::unique_ptr<Thread>> threads_; // 线程列表
    std::size_t initThreadSize_;                               // 初始的线程数量
    std::atomic_int curThreadSize_;                            // 记录当前线程池里面线程的总数量
    std::size_t threadSizeThreadHold_;                         // 线程数量的上限阈值
    std::atomic_int idleThreadSize_;                           // 记录空闲线程的数量

    // Task任务 =》 函数对象
    using Task = std::function<void()>;
    std::queue<Task> taskQue_;    // 任务队列
    std::atomic_int taskSize_;    // 任务的数量
    size_t taskQueMaxThreshHold_; // 任务队列数量上限阈值

    // 线程通信
    std::mutex taskQueMtx_;            // 保证任务队列线程安全
    std::condition_variable notFull_;  // 表示任务队列不满
    std::condition_variable notEmpty_; // 表示任务队列不空
    std::condition_variable exitCond_; // 等待线程资源全部回收

    PoolMode poolMode_;             // 当前线程池的工作模式
    std::atomic_bool isPoolRuning_; // 表示当前线程池的启动状态
};

#endif