#include "WorkerThread.h"
#include <string>
#include "EventLoop.h"

WorkerThread::WorkerThread(int index)
{
	m_thread = nullptr;
	m_evLoop = nullptr;
	m_threadID = std::thread::id();
	m_name = "WorkerThread-" + std::to_string(index);
}

WorkerThread::~WorkerThread()
{
	if (m_evLoop != nullptr)
	{
		delete m_thread;
	}
}

void WorkerThread::run()
{
	//创建子线程
	m_thread = new std::thread(&WorkerThread::running, this);

	//阻塞主线程，等待子线程将数据初始化
	std::unique_lock<std::mutex> locker(m_mutex);	//初始化后自动加锁
	while (this->m_evLoop == nullptr)		//访问共享资源
	{
		m_cond.wait(locker);		//数据初始化未完成, 解锁, 阻塞等待数据初始化完毕，被唤醒后自动加锁
	}
	//locker被释放前自动解锁
}

void WorkerThread::running()
{
	m_mutex.lock();
	m_evLoop = new EventLoop(m_name);
	m_mutex.unlock();
	m_cond.notify_one();
	m_evLoop->run();
}
