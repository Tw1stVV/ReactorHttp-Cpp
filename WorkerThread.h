#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>

class EventLoop;

class WorkerThread
{
public:
    WorkerThread(int index);
    ~WorkerThread();
    void run();

private:
    void running();
private:
    std::thread*    m_thread;   //线程实例
    std::thread::id m_threadID; // ID
    std::string m_name;
    std::mutex m_mutex;  // 互斥锁
    std::condition_variable m_cond;    // 条件变量
    EventLoop* evLoop;   // 反应堆模型
};

