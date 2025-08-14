#pragma once
#include <vector>
class EventLoop;
class WorkerThread;

class ThreadPool
{
public:
    ThreadPool(EventLoop* mainLoop, int count);
    void run();
    EventLoop* takeWorkerThreadEventLoop();

private:
    EventLoop* m_mainLoop;    // 主线程的反应堆模型
    bool isStart;           
    int m_threadNum;          //线程个数
    std::vector<WorkerThread*> m_workerThreads;
    int index;          //标记下个取出线程的索引
};

